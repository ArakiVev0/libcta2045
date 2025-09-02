#ifndef CTA2045_UART_H
#define CTA2045_UART_H

#include "common.h"
#include "cta2045_types.h"
#include <stddef.h>
#include <stdint.h>

/* Init UART/rings and RX worker */
void cta2045_init(void);
int cta2045_uart_init(void);
void cta2045_uart_thread(void *p1, void *p2, void *p3);

/* Non-blocking TX (queues to ISR) */
int send_response(const uint8_t *response, size_t len);

#endif /* CTA2045_UART_H */
