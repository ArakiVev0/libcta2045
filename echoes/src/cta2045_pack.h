
#ifndef CTA2045_PACK_H
#define CTA2045_PACK_H

#include <stddef.h>
#include <stdint.h>

/* Returns bytes written (0 on error). */
size_t cta2045_basic_pack(uint8_t op1, uint8_t op2, uint8_t *out,
                          size_t out_cap);
size_t cta2045_datalink_pack_max_payload_req(uint8_t *out, size_t out_cap);
size_t cta2045_intermediate_pack_get_utc_time_req(uint8_t *out, size_t out_cap);

#endif /* CTA2045_PACK_H */
