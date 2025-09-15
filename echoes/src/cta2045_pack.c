
#include "cta2045_pack.h"
#include "cta2045_types.h"
#include "zephyr/sys/byteorder.h"
#include <stdint.h>

uint16_t checksum_calc(const uint8_t *buf, size_t len) {
  uint32_t c1 = 0xAA, c2 = 0;
  for (size_t i = 0; i < len; i++) {
    c1 = (c1 + buf[i]) % 0xFF;
    c2 = (c2 + c1) % 0xFF;
  }
  uint8_t hi = (uint8_t)(0xFF - ((c1 + c2) % 0xFF));
  uint8_t lo = (uint8_t)(0xFF - ((c1 + hi) % 0xFF));
  return (uint16_t)((hi << 8) | lo); // BE: HI first
}

size_t nak_pack(LinkLayerNakCode NakCode, uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct LinkLayerMessage))
    return 0;

  struct LinkLayerMessage msg = {
      .msgType1 = LL_NAK_MSG_TYP1,
      .msgType2 = NakCode,
  };

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t ack_pack(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct LinkLayerMessage))
    return 0;

  struct LinkLayerMessage msg = {
      .msgType1 = LL_ACK_MSG_TYP1,
      .msgType2 = LL_ACK_MSG_TYP2,
  };

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t datalink_pack(uint8_t op1, uint8_t op2, uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct DataLinkMessage))
    return 0;

  struct DataLinkMessage msg = {
      .msgType1 = DATALINK_MSG_TYP1,
      .msgType2 = DATALINK_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = op1,
      .opCode2 = op2,
  };

  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);
  return sizeof(msg);
}

size_t basic_pack(uint8_t op1, uint8_t op2, uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct BasicMessage))
    return 0;

  struct BasicMessage msg = {
      .msgType1 = BASIC_MSG_TYP1,
      .msgType2 = BASIC_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = op1,
      .opCode2 = op2,
  };
  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t datalink_pack_max_payload_req(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct DataLinkMessage))
    return 0;

  struct DataLinkMessage msg = {
      .msgType1 = DATALINK_MSG_TYP1,
      .msgType2 = DATALINK_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = MAXPAYLOAD_REQ_OP_CODE1,
      .opCode2 = CLEAR_OP_CODE2,
  };
  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t intermediate_pack_get_info_req(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct IntermediateMessage))
    return 0;

  struct IntermediateMessage msg = {
      .msgType1 = INTERMEDIATE_MSG_TYP1,
      .msgType2 = INTERMEDIATE_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = GET_INFO_OP_CODE1,
      .opCode2 = GET_INFO_OP_CODE2,
  };
  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t intermediate_pack_get_utc_time_req(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct IntermediateMessage))
    return 0;

  struct IntermediateMessage msg = {
      .msgType1 = INTERMEDIATE_MSG_TYP1,
      .msgType2 = INTERMEDIATE_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = GET_UTC_TIME,
      .opCode2 = 0x00,
  };
  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t datalink_pack_maxPayload_resp(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct DataLinkMessage))
    return 0;

  struct DataLinkMessage msg = {
      .msgType1 = DATALINK_MSG_TYP1,
      .msgType2 = DATALINK_MSG_TYP2,
      .length = sys_cpu_to_be16(2),
      .opCode1 = MAXPAYLOAD_RESP,
      .opCode2 = 0,
  };

  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}

size_t intermediate_pack_get_utc_time_resp(uint8_t *out, size_t cap) {
  if (!out || cap < sizeof(struct GetUTCTimeResponse))
    return 0;
  struct GetUTCTimeResponse msg = {
      .msgType1 = INTERMEDIATE_MSG_TYP1,
      .msgType2 = INTERMEDIATE_MSG_TYP2,
      .length = sys_be16_to_cpu(sizeof(struct GetUTCTimeResponse) - 6),
      .opCode1 = GET_UTC_TIME,
      .opCode2 = OP_CODE2_REPLY,
      .responseCode = OP_CODE2_REPLY,
      .utcSeconds = 0,
      .timezoneOffsetQuarterHours = 0,
      .dstOffsetQuarterHours = 0,
  };

  uint16_t cs =
      checksum_calc((const uint8_t *)&msg, sizeof(msg) - sizeof(msg.checksum));
  msg.checksum = sys_cpu_to_be16(cs);

  memcpy(out, &msg, sizeof(msg));
  return sizeof(msg);
}
