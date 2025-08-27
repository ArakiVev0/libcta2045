#include "cta2045_uart.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

K_THREAD_STACK_DEFINE(cta2045_stack, CTA2045_STACK_SIZE);
static struct k_thread cta2045_tid;

int main(void) {
  int err = cta2045_uart_init();
  if (err) {
    LOG_ERR("cta2045_uart_init failed: %d", err);
  }

  k_tid_t tid = k_thread_create(
      &cta2045_tid, cta2045_stack, K_THREAD_STACK_SIZEOF(cta2045_stack),
      cta2045_uart_thread, NULL, NULL, NULL,
      K_PRIO_PREEMPT(CTA2045_THREAD_PRIO), 0, K_NO_WAIT);

  k_thread_name_set(tid, "cta2045_rx");

  LOG_INF("APP started");
  return 0;
}
