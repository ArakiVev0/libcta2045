// src/cta2045_uart.c
#include "cta2045_uart.h"
#include "common.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_REGISTER(cta2045_uart, LOG_LEVEL_INF);

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

static uint8_t rx_ring_buf_data[RING_BUF_SIZE];
static struct ring_buf rx_rb;

static uint8_t tx_ring_buf_data[TX_RING_BUF_SIZE];
static struct ring_buf tx_rb;

K_SEM_DEFINE(uart_sem, 0, 1);

uint16_t checksum_calc(const uint8_t *buf, size_t len) {
  int checksum1 = 0xaa;
  int checksum2 = 0;

  for (size_t i = 0; i < len; i++) {
    checksum1 += buf[i];
    checksum1 = checksum1 % 0xff;

    checksum2 += checksum1;
    checksum2 = checksum2 % 0xff;
  }

  uint8_t value[2];

  value[0] = 255 - ((checksum1 + checksum2) % 255);
  value[1] = 255 - ((checksum1 + value[0]) % 255);

  return *((uint16_t *)(&value));
}

void cta2045_init(struct cta2045_ctx *ctx) {
  ctx->state = STATE_WAIT_START;
  ctx->payload_idx = 0;
  ctx->calc_checksum = 0;
}

int send_link_layer_ACK() {
  struct LinkMessage resp;
  resp.msgType1 = LL_ACK_MSG_TYP1;
  resp.msgType2 = LL_ACK_MSG_TYP2;
  return send_response((uint8_t *)&resp, sizeof(resp));
}

int send_link_layer_NAK(LinkLayerNakCode error_code) {
  struct LinkMessage resp;
  resp.msgType1 = LL_NAK_MSG_TYP1;
  resp.msgType2 = (uint8_t)error_code;
  return send_response((uint8_t *)&resp, sizeof(resp));
}

IntermediateTypeCode ConvertIntermediate(uint16_t intermediateType) {
  uint16_t host = sys_be16_to_cpu(intermediateType);
  switch (host) {
  case IT_INFO_REQUEST:
    return IT_INFO_REQUEST;
  case IT_INFO_RESPONSE:
    return IT_INFO_RESPONSE;
  case IT_GET_UTC_TIME_REQUEST:
    return IT_GET_UTC_TIME_REQUEST;
  case IT_COMMODITY_REQUEST:
    return IT_COMMODITY_REQUEST;
  case IT_COMMODITY_RESPONSE:
    return IT_COMMODITY_RESPONSE;
  case IT_SET_ENERGY_PRICE_RESPONSE:
    return IT_SET_ENERGY_PRICE_RESPONSE;
  case IT_GET_SET_TEMPERATURE_OFFSET_REQUEST:
    return IT_GET_SET_TEMPERATURE_OFFSET_REQUEST;
  case IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE:
    return IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE;
  case IT_GET_SET_SETPOINT_RESPONSE:
    return IT_GET_SET_SETPOINT_RESPONSE;
  case IT_GET_PRESENT_TEMPERATURE_RESPONSE:
    return IT_GET_PRESENT_TEMPERATURE_RESPONSE;
  case IT_START_CYCLING_RESPONSE:
    return IT_START_CYCLING_RESPONSE;
  case IT_TERMINATE_CYCLING_RESPONSE:
    return IT_TERMINATE_CYCLING_RESPONSE;
  default:
    return IT_INVALID;
  }
}

DataLinkTypeCode ConvertDataLinkType(uint8_t datalinkType) {
  switch (datalinkType) {
  case DLT_MAX_PAYLOAD_REQUEST:
    return DLT_MAX_PAYLOAD_REQUEST;
  case DLT_MAX_PAYLOAD_RESPONSE:
    return DLT_MAX_PAYLOAD_RESPONSE;
  case DLT_SEND_NEXT_COMMAND_TO_SLOT:
    return DLT_SEND_NEXT_COMMAND_TO_SLOT;
  default:
    return DLT_INVALID;
  }
}

void process_basic_message(struct BasicMessage *msg) {
  switch (msg->opCode1) {
  case APP_ACK:
    send_link_layer_ACK();
    break;

  case APP_NAK:
    send_link_layer_ACK();
    break;

  case OPER_STATE_RESP:
    send_link_layer_ACK();
    break;

  case CUST_OVERRIDE:
    send_link_layer_ACK();

    struct BasicMessage resp;
    resp.msgType1 = BASIC_MSG_TYP1;
    resp.msgType2 = BASIC_MSG_TYP2;
    resp.length = sys_cpu_to_be16(2);
    resp.opCode1 = APP_ACK;
    resp.opCode2 = CUST_OVERRIDE;
    resp.checksum = checksum_calc((uint8_t *)&resp, sizeof(resp) - 2);
    send_response((uint8_t *)&resp, sizeof(resp));

    break;

  default:
    send_link_layer_NAK(LLN_UNSUPPORTED_MESSAGE_TYPE);
    break;
  }
}

MaxPayloadLengthCode get_max_payload_length() { return MPLL_LENGTH4096; }

MaxPayloadLengthCode convert_max_payload_length(uint8_t maxPayloadLength) {
  switch (maxPayloadLength) {
  case MPLL_LENGTH2:
    return MPLL_LENGTH2;
  case MPLL_LENGTH4:
    return MPLL_LENGTH4;
  case MPLL_LENGTH8:
    return MPLL_LENGTH8;
  case MPLL_LENGTH32:
    return MPLL_LENGTH32;
  case MPLL_LENGTH64:
    return MPLL_LENGTH64;
  case MPLL_LENGTH128:
    return MPLL_LENGTH128;
  case MPLL_LENGTH256:
    return MPLL_LENGTH256;
  case MPLL_LENGTH512:
    return MPLL_LENGTH512;
  case MPLL_LENGTH1024:
    return MPLL_LENGTH1024;
  case MPLL_LENGTH1280:
    return MPLL_LENGTH1280;
  case MPLL_LENGTH1500:
    return MPLL_LENGTH1500;
  case MPLL_LENGTH2048:
    return MPLL_LENGTH2048;
  case MPLL_LENGTH4096:
    return MPLL_LENGTH4096;
  default:
    return MPLL_INVALID;
    break;
  }
}

void process_datalink_message(struct DataLinkMessage *msg) {
  DataLinkTypeCode messageType = ConvertDataLinkType(msg->opCode1);
  switch (messageType) {
  case DLT_MAX_PAYLOAD_REQUEST:
    send_link_layer_ACK();

    struct DataLinkMessage resp;
    resp.msgType1 = DATALINK_MSG_TYP1;
    resp.msgType2 = DATALINK_MSG_TYP2;
    resp.length = sys_cpu_to_be16(2);
    resp.opCode1 = MAXPAYLOAD_RESP;
    resp.opCode2 = (uint8_t)get_max_payload_length();
    resp.checksum = checksum_calc((uint8_t *)&resp, sizeof(resp) - 2);

    send_response((uint8_t *)&resp, sizeof(resp));
    break;
  case DLT_MAX_PAYLOAD_RESPONSE:
    // MaxPayloadLengthCode maxPayloadLength =
    // convert_max_payload_length(msg->opCode2);
    send_link_layer_ACK();
    break;
  default:
    send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
    break;
  }
}

void process_intermediate_message(struct IntermediateMessage *msg) {
  uint16_t intermediateType = *((uint16_t *)(&(msg->opCode1)));

  switch (intermediateType) {
  case IT_INFO_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_UTC_TIME_REQUEST:
    if (sys_cpu_to_be16(msg->length) == 2) {
      send_link_layer_ACK();
      struct GetUTCTimeResponse resp;

      resp.utcSeconds = 0;
      resp.timezoneOffsetQuarterHours = 0;
      resp.dstOffsetQuarterHours = 0;

      resp.msgType1 = INTERMEDIATE_MSG_TYP1;
      resp.msgType2 = INTERMEDIATE_MSG_TYP2;
      resp.length = sys_be16_to_cpu(sizeof(msg->length) - 6);
      resp.opCode1 = GET_UTC_TIME;
      resp.opCode2 = OP_CODE2_REPLY;
      resp.responseCode = OP_CODE2_REPLY;
      resp.checksum = checksum_calc((uint8_t *)&resp, sizeof(resp) - 2);

      send_response((uint8_t *)&resp, sizeof(resp));
    } else {
      send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
    }
    break;

  case IT_COMMODITY_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_SET_ENERGY_PRICE_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_SET_SETPOINT_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_PRESENT_TEMPERATURE_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_START_CYCLING_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_TERMINATE_CYCLING_RESPONSE:
    send_link_layer_ACK();
    break;

  default:
    send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
    break;
  }
}

// check a message response from the SGD
void process_response(struct MessageHeader *msg_header) {
  if (msg_header->msgType1 == BASIC_MSG_TYP1 ||
      msg_header->msgType2 == BASIC_MSG_TYP2) {
    struct BasicMessage *basic = (struct BasicMessage *)msg_header;
    process_basic_message(basic);
  } else if (msg_header->msgType1 == DATALINK_MSG_TYP1 ||
             msg_header->msgType2 == DATALINK_MSG_TYP2) {
    struct DataLinkMessage *datalink = (struct DataLinkMessage *)msg_header;
    process_datalink_message(datalink);
  } else if (msg_header->msgType1 == INTERMEDIATE_MSG_TYP1 ||
             msg_header->msgType2 == INTERMEDIATE_MSG_TYP2) {
    struct IntermediateMessage *intermediate =
        (struct IntermediateMessage *)msg_header;
    process_intermediate_message(intermediate);
  } else {
    send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
  }
}

void cta2045_process_byte(struct cta2045_ctx *ctx, uint8_t byte) {
  if (byte == CTA2045_START_BYTE && ctx->state != STATE_WAIT_START) {
    LOG_DBG("Unexpected start byte; reset");
    cta2045_init(ctx);
  }

  switch (ctx->state) {
  case STATE_WAIT_START:
    if (byte == CTA2045_START_BYTE) {
      ctx->state = STATE_WAIT_LENGTH;
      LOG_DBG("Start byte");
    }
    break;

  case STATE_WAIT_LENGTH:
    ctx->msg.length = byte;
    ctx->calc_checksum = byte;
    ctx->payload_idx = 0;

    if (ctx->msg.length == 0) {
      ctx->state = STATE_WAIT_CHECKSUM;
    } else {
      ctx->state = STATE_WAIT_PAYLOAD;
    }
    break;

  case STATE_WAIT_PAYLOAD:
    if (ctx->payload_idx < ctx->msg.length) {
      ctx->msg.payload[ctx->payload_idx++] = byte;
      ctx->calc_checksum ^= byte;

      if (ctx->payload_idx >= ctx->msg.length) {
        ctx->state = STATE_WAIT_CHECKSUM;
      }
    } else {
      LOG_WRN("Payload overflow; reset");
      cta2045_init(ctx);
    }
    break;

  case STATE_WAIT_CHECKSUM: {
    ctx->msg.checksum = byte;
    bool ok = (ctx->calc_checksum == byte);

    if (ok) {
      LOG_INF("Valid CTA-2045 message len=%u", ctx->msg.length);
      LOG_HEXDUMP_INF(ctx->msg.payload, ctx->msg.length, "Payload");
    } else {
      LOG_WRN("Checksum mismatch calc=0x%02x recv=0x%02x", ctx->calc_checksum,
              byte);
    }

    // cta2045_send_ack(ok);
    cta2045_init(ctx);
    break;
  }
  }
}

static void uart_isr(const struct device *dev, void *user_data) {
  ARG_UNUSED(user_data);

  if (!uart_irq_update(dev)) {
    return;
  }

  while (uart_irq_rx_ready(dev)) {
    uint8_t rx_buf[32];
    int rd = uart_fifo_read(dev, rx_buf, sizeof(rx_buf));
    if (rd <= 0) {
      break;
    }
    uint32_t wrote = ring_buf_put(&rx_rb, rx_buf, rd);
    if (wrote > 0) {
      k_sem_give(&uart_sem);
    }
  }

  while (uart_irq_tx_ready(dev)) {
    uint8_t tx_buf[32];
    uint32_t n = ring_buf_get(&tx_rb, tx_buf, sizeof(tx_buf));
    if (n == 0) {
      uart_irq_tx_disable(dev);
      break;
    }
    int sent = uart_fifo_fill(dev, tx_buf, n);
    if (sent < n) {
      (void)ring_buf_put(&tx_rb, &tx_buf[sent], n - sent);
      break;
    }
  }
}

int cta2045_uart_init(void) {
  if (!device_is_ready(uart_dev)) {
    LOG_ERR("UART device not ready");
    return -ENODEV;
  }

  ring_buf_init(&rx_rb, sizeof(rx_ring_buf_data), rx_ring_buf_data);
  ring_buf_init(&tx_rb, sizeof(tx_ring_buf_data), tx_ring_buf_data);

  /* Optional: configure UART if not fixed by DTS*/
  struct uart_config cfg = {
      .baudrate = 115200,
      .parity = UART_CFG_PARITY_NONE,
      .stop_bits = UART_CFG_STOP_BITS_1,
      .data_bits = UART_CFG_DATA_BITS_8,
      .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
  };
  (void)uart_configure(uart_dev, &cfg);

  uart_irq_callback_set(uart_dev, uart_isr);
  uart_irq_rx_enable(uart_dev); /* start RX; TX enabled on demand */

  LOG_INF("CTA2045 UART init done");
  return 0;
}

/* Non-blocking TX API (queues data, ISR drains) */
int send_response(const uint8_t *data, size_t len) {
  if (!uart_dev || !device_is_ready(uart_dev)) {
    return -ENODEV;
  }

  uint32_t put;
  unsigned int key = irq_lock();
  put = ring_buf_put(&tx_rb, data, len);
  irq_unlock(key);

  if (put == 0) {
    return -ENOBUFS;
  }
  if (put < len) {
    LOG_WRN("TX ring partial: %u/%u bytes", put, (unsigned)len);
  }
  uart_irq_tx_enable(uart_dev);
  return 0;
}

void cta2045_uart_thread(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  struct cta2045_ctx ctx = {0};
  cta2045_init(&ctx);

  LOG_INF("CTA-2045 UART thread started");
  for (;;) {
    k_sem_take(&uart_sem, K_FOREVER);

    uint8_t byte;
    while (ring_buf_get(&rx_rb, &byte, 1) == 1) {
      cta2045_process_byte(&ctx, byte);
    }
  }
}
