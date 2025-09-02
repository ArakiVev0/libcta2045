
#include "cta2045_pack.h"
#include "common.h"
#include "cta2045_types.h"
#include <string.h>
#include <zephyr/sys/byteorder.h>

/* local checksum */
static uint16_t cta2045_checksum(const uint8_t *buf, size_t len) {
  int c1 = 0xaa, c2 = 0;
  for (size_t i = 0; i < len; i++) {
    c1 = (c1 + buf[i]) % 0xff;
    c2 = (c2 + c1) % 0xff;
  }
  uint8_t b0 = 255 - ((c1 + c2) % 255);
  uint8_t b1 = 255 - ((c1 + b0) % 255);
  return (uint16_t)(((uint16_t)b0 << 8) | b1);
}

size_t cta2045_basic_pack(uint8_t op1, uint8_t op2, uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct BasicMessage))
    return 0;

  struct BasicMessage msg = {
      .msgType1 = BASIC_MSG_TYP1,
      .msgType2 = BASIC_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = op1,
      .opCode2 = op2,
  };
  uint16_t cs = cta2045_checksum((const uint8_t *)&msg,
                                 sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t cta2045_datalink_pack_max_payload_req(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct DataLinkMessage))
    return 0;

  struct DataLinkMessage msg = {
      .msgType1 = DATALINK_MSG_TYP1,
      .msgType2 = DATALINK_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = MAXPAYLOAD_REQ_OP_CODE1,
      .opCode2 = CLEAR_OP_CODE2,
  };
  uint16_t cs = cta2045_checksum((const uint8_t *)&msg,
                                 sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t cta2045_intermediate_pack_get_utc_time_req(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct IntermediateMessage))
    return 0;

  struct IntermediateMessage msg = {
      .msgType1 = INTERMEDIATE_MSG_TYP1,
      .msgType2 = INTERMEDIATE_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = GET_UTC_TIME,
      .opCode2 = 0x00,
  };
  uint16_t cs = cta2045_checksum((const uint8_t *)&msg,
                                 sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}
