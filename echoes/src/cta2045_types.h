
#ifndef CTA2045_TYPES_H
#define CTA2045_TYPES_H

#include "common.h"
#include <stdint.h>

/* ---------- Message type bytes ---------- */
#define BASIC_MSG_TYP1 0x08
#define BASIC_MSG_TYP2 0x01

#define DATALINK_MSG_TYP1 0x08
#define DATALINK_MSG_TYP2 0x03

#define INTERMEDIATE_MSG_TYP1 0x08
#define INTERMEDIATE_MSG_TYP2 0x02

#define LL_ACK_MSG_TYP1 0x06
#define LL_ACK_MSG_TYP2 0x00

#define LL_NAK_MSG_TYP1 0x15

// Basic
#define APP_ACK 0x03
#define APP_NAK 0x04
#define OPER_STATE_REQ 0x12
#define OPER_STATE_RESP 0x13
#define CUST_OVERRIDE 0x11

// Data Link
#define MAXPAYLOAD_REQ_OP_CODE1 0x18
#define CLEAR_OP_CODE2 0x00
#define MAXPAYLOAD_RESP 0x19

// Intermediate
#define GET_INFO_OP_CODE1 0x01
#define GET_INFO_OP_CODE2 0x01

#define GET_UTC_TIME 0x02
#define OP_CODE2_REPLY 0x80

struct MessageHeader {
  uint8_t msgType1;
  uint8_t msgType2;
} __attribute__((packed));

struct LinkLayerMessage {
  uint8_t msgType1;
  uint8_t msgType2;
} __attribute__((packed));

struct BasicMessage {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length;
  uint8_t opCode1;
  uint8_t opCode2;
  uint16_t checksum;
} __attribute__((packed));

struct DataLinkMessage {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length;
  uint8_t opCode1;
  uint8_t opCode2;
  uint16_t checksum;
} __attribute__((packed));

struct IntermediateMessage {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length;
  uint8_t opCode1;
  uint8_t opCode2;
  uint16_t checksum;
} __attribute__((packed));

struct GetUTCTimeResponse {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length;
  uint8_t opCode1;
  uint8_t opCode2;
  uint8_t responseCode;
  uint16_t utcSeconds;
  uint8_t timezoneOffsetQuarterHours;
  uint8_t dstOffsetQuarterHours;
  uint16_t checksum;
} __attribute__((packed));

typedef enum {
  LLN_NO_REASON,
  LLN_INVALID_BYTE,
  LLN_INVALID_LENGTH,
  LLN_CHECKSUM_ERROR,
  LLN_RESERVED,
  LLN_MESSAGE_TIMEOUT,
  LLN_UNSUPPORTED_MESSAGE_TYPE,
  LLN_REQUEST_NOT_SUPPORTED,
  LLN_NONE
} LinkLayerNakCode;

typedef enum {
  DLT_INVALID,
  DLT_MAX_PAYLOAD_REQUEST = 0x18,
  DLT_MAX_PAYLOAD_RESPONSE = 0x19,
  DLT_SEND_NEXT_COMMAND_TO_SLOT = 0x1e
} DataLinkTypeCode;

typedef enum {
  IT_INVALID,
  IT_INFO_REQUEST = 0x0101,
  IT_INFO_RESPONSE = 0x0181,
  IT_GET_UTC_TIME_REQUEST = 0x0200,
  IT_COMMODITY_REQUEST = 0x0600,
  IT_COMMODITY_RESPONSE = 0x0680,
  IT_SET_ENERGY_PRICE_RESPONSE = 0x0380,
  IT_GET_SET_TEMPERATURE_OFFSET_REQUEST = 0x0302,
  IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE = 0x0382,
  IT_GET_SET_SETPOINT_RESPONSE = 0x0383,
  IT_GET_PRESENT_TEMPERATURE_RESPONSE = 0X0384,
  IT_START_CYCLING_RESPONSE = 0x0480,
  IT_TERMINATE_CYCLING_RESPONSE = 0x0481
} IntermediateTypeCode;

#endif /* CTA2045_TYPES_H */
