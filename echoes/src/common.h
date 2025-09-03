#ifndef CTA2045_COMMON_H
#define CTA2045_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

/* ---------- Build-time constants ---------- */
#ifndef RING_BUF_SIZE
#define RING_BUF_SIZE 1024
#endif

#ifndef TX_RING_BUF_SIZE
#define TX_RING_BUF_SIZE 1024
#endif

#ifndef MAX_PAYLOAD_LEN
#define MAX_PAYLOAD_LEN 4096
#endif

/* ---------- Utilities ---------- */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Generic raw message wrapper (optional helper) */
struct cta2045_msg {
  uint16_t length; /* big-endian on wire */
  uint8_t payload[MAX_PAYLOAD_LEN];
  uint16_t checksum; /* big-endian on wire */
};

#endif /* CTA2045_COMMON_H */
