#include "cta2045_uart.h"
#include "common.h"
#include "cta2045_types.h"

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_REGISTER(cta2045_uart, LOG_LEVEL_INF);

/* UART device (adjust DT node label as needed) */
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

/* Ring buffers */
static uint8_t rx_ring_buf_data[RING_BUF_SIZE];
static struct ring_buf rx_rb;

static uint8_t tx_ring_buf_data[TX_RING_BUF_SIZE];
static struct ring_buf tx_rb;

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

    /* In your full implementation, drain rx_rb here,
       assemble frames, verify checksum, and dispatch handlers. */
    uint8_t tmp[64];
    while (ring_buf_get(&rx_rb, tmp, sizeof(tmp)) != 0) {
      /* placeholder: you can parse/process here or in another module */
      /* LOG_HEXDUMP_DBG(tmp, got, "RX chunk"); */
    }
  }
}
