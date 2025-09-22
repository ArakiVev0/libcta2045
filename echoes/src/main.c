#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "common.h"
#include "cta2045_pack.h"
#include "cta2045_resp.h"
#include "cta2045_types.h"
#include "cta2045_uart.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define CTA_THREAD_STACK_SIZE 4096
#define CTA_THREAD_PRIO 7

K_THREAD_STACK_DEFINE(cta_stack, CTA_THREAD_STACK_SIZE);
static struct k_thread cta_thr;

static void send_demo_frames(void) {
  uint8_t buf[64];
  size_t n;

  n = intermediate_pack_get_device_info_req(buf, sizeof(buf));
  if (n) {
    LOG_INF("Send Get Get Device Request");
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

  send_demo_frames();

  while (1) {
    k_sleep(K_MSEC(10));
  }
}
