#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "common.h"
#include "cta2045_pack.h"
#include "cta2045_types.h"
#include "cta2045_uart.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define CTA_THREAD_STACK_SIZE 2048
#define CTA_THREAD_PRIO 7

K_THREAD_STACK_DEFINE(cta_stack, CTA_THREAD_STACK_SIZE);
static struct k_thread cta_thr;

static void send_demo_frames(void) {
  uint8_t buf[64];
  size_t n;

  n = cta2045_datalink_pack_max_payload_req(buf, sizeof(buf));
  if (n) {
    LOG_INF("Send MaxPayloadReq");
    send_response(buf, n);
  }
  k_sleep(K_MSEC(50));

  n = cta2045_basic_pack(OPER_STATE_REQ, 0x00, buf, sizeof(buf));
  if (n) {
    LOG_INF("Send OperStateReq");
    send_response(buf, n);
  }
  k_sleep(K_MSEC(50));

  n = cta2045_intermediate_pack_get_utc_time_req(buf, sizeof(buf));
  if (n) {
    LOG_INF("Send GetUTCTime");
    send_response(buf, n);
  }
}

void main(void) {
  LOG_INF("CTA-2045 demo starting…");

  if (cta2045_uart_init() != 0) {
    LOG_ERR("UART init failed");
    return;
  }

  k_thread_create(&cta_thr, cta_stack, K_THREAD_STACK_SIZEOF(cta_stack),
                  cta2045_uart_thread, NULL, NULL, NULL, CTA_THREAD_PRIO, 0,
                  K_NO_WAIT);

  /* Optional: give RX thread a moment */
  k_sleep(K_MSEC(100));

  send_demo_frames();

  while (1) {
    k_sleep(K_MSEC(10));
  }
}
