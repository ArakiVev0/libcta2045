
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

/* ---------- Basic opcodes (subset) ---------- */
#define APP_ACK 0x03
#define APP_NAK 0x04
#define OPER_STATE_REQ 0x12
#define OPER_STATE_RESP 0x13
#define CUST_OVERRIDE 0x11

/* ---------- Data-link ---------- */
#define MAXPAYLOAD_REQ_OP_CODE1 0x18
#define CLEAR_OP_CODE2 0x00
#define MAXPAYLOAD_RESP 0x19

/* ---------- Intermediate ---------- */
#define GET_UTC_TIME 0x02
#define OP_CODE2_REPLY 0x80

/* ---------- Wire structs ---------- */
struct BasicMessage {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length; /* be16 */
  uint8_t opCode1;
  uint8_t opCode2;
  uint16_t checksum; /* be16 */
} __attribute__((packed));

struct DataLinkMessage {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length; /* be16 */
  uint8_t opCode1;
  uint8_t opCode2;
  uint16_t checksum; /* be16 */
} __attribute__((packed));

struct IntermediateMessage {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length; /* be16 */
  uint8_t opCode1;
  uint8_t opCode2;
  uint16_t checksum; /* be16 */
} __attribute__((packed));

#endif /* CTA2045_TYPES_H */
