#ifndef CTA2045_COMMON_H
#define CTA2045_COMMON_H

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define RING_BUF_SIZE 256
#define TX_RING_BUF_SIZE 256

#define MAX_PAYLOAD_LEN 256

#define CTA2045_START_BYTE 0xAA

#define CTA2045_STACK_SIZE (1024 * 8)

#define CTA2045_THREAD_PRIO 8

struct cta2045_msg {
  uint8_t length;
  uint8_t payload[MAX_PAYLOAD_LEN];
  uint8_t checksum;
};

/* ---- Externals provided elsewhere ---- */
extern struct k_thread cta2045_thread;
extern void cta2045_uart_thread(void *p1, void *p2, void *p3);

#endif /* CTA2045_COMMON_H */
