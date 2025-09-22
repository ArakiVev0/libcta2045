
#ifndef CTA2045_PACK_H
#define CTA2045_PACK_H

#include "common.h"
#include "cta2045_types.h"
#include <stddef.h>
#include <stdint.h>

/* Returns bytes written (0 on error). */
uint16_t checksum_calc(const uint8_t *buf, size_t len);

size_t ack_pack(uint8_t *out, size_t cap);
size_t nak_pack(LinkLayerNakCode NakCode, uint8_t *out, size_t cap);

size_t basic_pack(uint8_t op1, uint8_t op2, uint8_t *out, size_t out_cap);
size_t datalink_pack(uint8_t op1, uint8_t op2, uint8_t *out, size_t out_cap);

size_t intermediate_pack_get_device_info_req(uint8_t *out, size_t cap);
size_t intermediate_pack_get_info_req(uint8_t *out, size_t cap);
size_t intermediate_pack_get_utc_time_req(uint8_t *out, size_t out_cap);

size_t datalink_pack_maxPayload_resp(uint8_t *out, size_t cap);
size_t intermediate_pack_get_utc_time_resp(uint8_t *out, size_t cap);

#endif /* CTA2045_PACK_H */
