#include "cta2045_uart.h"
#include "common.h"
#include "cta2045_pack.h"
#include "cta2045_resp.h"
#include "cta2045_types.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>

#define CTA_MAX_PAYLOAD 256u
#define CTA_HDR_ONLY_SIZE 2u
#define CTA_LEN_SIZE 2u // BE payload length on-wire
#define CTA_CSUM_SIZE 2u
#define CTA_MIN_FRAME (CTA_HDR_ONLY_SIZE + CTA_LEN_SIZE + CTA_CSUM_SIZE) // 6
#define CTA_RX_BUF_SZ 512u // fits 4096 payload + header/len/csum, with margin

// Offsets relative to start of a frame (MessageHeader*)
#define CTA_OFF_LEN (CTA_HDR_ONLY_SIZE)                    // 2..3
#define CTA_OFF_PAYLOAD (CTA_HDR_ONLY_SIZE + CTA_LEN_SIZE) // 4..(4+len-1)

LOG_MODULE_REGISTER(cta2045_uart, LOG_LEVEL_INF);

/* UART device (adjust DT node label as needed) */
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

/* Ring buffers */
static uint8_t rx_ring_buf_data[RING_BUF_SIZE];
static struct ring_buf rx_rb;

static uint8_t tx_ring_buf_data[TX_RING_BUF_SIZE];
static struct ring_buf tx_rb;

uint8_t rx_assemble_buf[CTA_RX_BUF_SZ];
size_t rx_assemble_len = 0;

static inline bool is_ll_ack(const struct MessageHeader *h) {
  return h->msgType1 == LL_ACK_MSG_TYP1 && h->msgType2 == LL_ACK_MSG_TYP2;
}
static inline bool is_ll_nak(const struct MessageHeader *h) {
  return h->msgType1 == LL_NAK_MSG_TYP1;
}
/* Simple RX semaphore (signal from ISR) */
K_SEM_DEFINE(uart_sem, 0, 1);

/* -------- TX queue -------- */
int send_response(const uint8_t *data, size_t len) {
  if (!uart_dev || !device_is_ready(uart_dev))
    return -ENODEV;
  if (!data || !len)
    return -EINVAL;

  unsigned int key = irq_lock();
  uint32_t put = ring_buf_put(&tx_rb, data, len);
  irq_unlock(key);

  if (put == 0)
    return -ENOBUFS;
  if (put < len) {
    /* best-effort; enable TX anyway */
  }
  uart_irq_tx_enable(uart_dev);
  return 0;
}

/* -------- ISR -------- */
static void uart_isr(const struct device *dev, void *user_data) {
  ARG_UNUSED(user_data);
  if (!uart_irq_update(dev))
    return;

  while (uart_irq_rx_ready(dev)) {
    uint8_t rx_buf[32];
    int rd = uart_fifo_read(dev, rx_buf, sizeof(rx_buf));
    if (rd <= 0)
      break;
    uint32_t wrote = ring_buf_put(&rx_rb, rx_buf, rd);
    if (wrote > 0)
      k_sem_give(&uart_sem);
  }

  while (uart_irq_tx_ready(dev)) {
    uint8_t tx_buf[32];
    uint32_t n = ring_buf_get(&tx_rb, tx_buf, sizeof(tx_buf));
    if (n == 0) {
      uart_irq_tx_disable(dev);
      break;
    }
    (void)uart_fifo_fill(dev, tx_buf, n);
  }
}

/* -------- Init & thread -------- */
int cta2045_uart_init(void) {
  if (!device_is_ready(uart_dev)) {
    LOG_ERR("UART device not ready");
    return -ENODEV;
  }

  ring_buf_init(&rx_rb, sizeof(rx_ring_buf_data), rx_ring_buf_data);
  ring_buf_init(&tx_rb, sizeof(tx_ring_buf_data), tx_ring_buf_data);

  struct uart_config cfg = {
      .baudrate = 115200,
      .parity = UART_CFG_PARITY_NONE,
      .stop_bits = UART_CFG_STOP_BITS_1,
      .data_bits = UART_CFG_DATA_BITS_8,
      .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
  };
  (void)uart_configure(uart_dev, &cfg);

  uart_irq_callback_set(uart_dev, uart_isr);
  uart_irq_rx_enable(uart_dev);
  return 0;
}

void cta2045_init(void) { /* reserved for future state */ }

void cta2045_uart_thread(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  for (;;) {
    k_sem_take(&uart_sem, K_FOREVER);

    uint8_t chunk[64];
    size_t bytes_read;
    while ((bytes_read = ring_buf_get(&rx_rb, chunk, sizeof(chunk))) != 0) {
      LOG_HEXDUMP_DBG(chunk, bytes_read, "RX chunk");

      // Ensure assembler has room
      if (bytes_read > (CTA_RX_BUF_SZ - rx_assemble_len)) {
        size_t need = bytes_read - (CTA_RX_BUF_SZ - rx_assemble_len);
        if (need > rx_assemble_len)
          need = rx_assemble_len;
        if (need) {
          memmove(rx_assemble_buf, rx_assemble_buf + need,
                  rx_assemble_len - need);
          rx_assemble_len -= need;
          LOG_WRN("CTA2045: assembler overflow, dropped %zu byte(s)", need);
        }
      }

      // Append new data
      memcpy(rx_assemble_buf + rx_assemble_len, chunk, bytes_read);
      rx_assemble_len += bytes_read;

      for (;;) {

        // --- Fast-path: header-only LL frames (2 bytes total) ---

        if (rx_assemble_len >= 2) {
          struct MessageHeader *h2 = (struct MessageHeader *)rx_assemble_buf;
          if (is_ll_ack(h2)) {
            LOG_INF("LL-ACK (header-only) received");
            // TODO: k_sem_give(&ll_ack_sem); // if you track TX acks
            // consume 2 bytes
            if (2 < rx_assemble_len) {
              memmove(rx_assemble_buf, rx_assemble_buf + 2,
                      rx_assemble_len - 2);
            }
            rx_assemble_len -= 2;
            continue; // check if another frame is ready
          }
          if (is_ll_nak(h2)) {
            LOG_INF("LL-NAK (header-only) received");
            // TODO: record NAK, trigger retry/backoff, etc.
            if (2 < rx_assemble_len) {
              memmove(rx_assemble_buf, rx_assemble_buf + 2,
                      rx_assemble_len - 2);
            }
            rx_assemble_len -= 2;
            continue;
          }
        }

        // --- Normal path: full frames need hdr+len+csum (>= 6 bytes) ---
        if (rx_assemble_len < CTA_MIN_FRAME)
          break;

        // (unchanged) read length at offset +2, sanity, checksum, etc...
        uint16_t payload_len = sys_get_be16(rx_assemble_buf + CTA_OFF_LEN);
        if ((payload_len > CTA_MAX_PAYLOAD) ||
            (payload_len > (CTA_RX_BUF_SZ - CTA_MIN_FRAME))) {
          memmove(rx_assemble_buf, rx_assemble_buf + 1, rx_assemble_len - 1);
          rx_assemble_len -= 1;
          LOG_WRN("CTA2045: bad length %u, resync by 1 byte", payload_len);
          continue;
        }

        size_t total_len =
            CTA_HDR_ONLY_SIZE + CTA_LEN_SIZE + payload_len + CTA_CSUM_SIZE;
        if (rx_assemble_len < total_len)
          break;

        uint16_t calc =
            checksum_calc(rx_assemble_buf, total_len - CTA_CSUM_SIZE);
        uint16_t got =
            sys_get_be16(rx_assemble_buf + (total_len - CTA_CSUM_SIZE));
        if (calc != got) {
          LOG_WRN(
              "CTA2045: checksum mismatch (calc=0x%04x got=0x%04x) — resync",
              calc, got);
          memmove(rx_assemble_buf, rx_assemble_buf + 1, rx_assemble_len - 1);
          rx_assemble_len -= 1;
          continue;
        }

        struct MessageHeader *hdr = (struct MessageHeader *)rx_assemble_buf;
        process_response(hdr);

        // consume full frame
        if (total_len < rx_assemble_len) {
          memmove(rx_assemble_buf, rx_assemble_buf + total_len,
                  rx_assemble_len - total_len);
        }
        rx_assemble_len -= total_len;
      }
    }
  }
}
