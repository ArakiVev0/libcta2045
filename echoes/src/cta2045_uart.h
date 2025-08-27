#ifndef CTA2045_UART_H
#define CTA2045_UART_H

#include "common.h"
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define BASIC_MSG_TYP1 0x08
#define BASIC_MSG_TYP2 0x01

#define DATALINK_MSG_TYP1 0x08
#define DATALINK_MSG_TYP2 0x03

#define INTERMEDIATE_MSG_TYP1 0x08
#define INTERMEDIATE_MSG_TYP2 0x02

#define LL_ACK_MSG_TYP1 0x06
#define LL_ACK_MSG_TYP2 0x00

#define LL_NAK_MSG_TYP1 0x15

#define SHED 0x01
#define END_SHED 0x02
#define APP_ACK 0x03
#define APP_NAK 0x04
#define POWER_LEVEL 0x06
#define PRESENT_RELATIVE_PRICE 0x07
#define NEXT_RELATIVE_PRICE 0x08
#define TIME_REMAINING_PRICE 0x09
#define CPP 0x0a
#define GRID_EMERGENCY 0x0b
#define GRID_GUIDANCE 0x0c
#define COMM_STATUS 0x0e
#define CUST_OVERRIDE 0x11
#define OPER_STATE_REQ 0x12
#define OPER_STATE_RESP 0x13
#define SLEEP_STATE 0x14
#define WAKE_STATE 0x15
#define SIMPLE_TIME_SYNC 0x16
#define LOADUP 0x17
#define INFO_REQ 0x01
#define SET_UTC_TIME 0x02
#define GET_UTC_TIME 0x02
#define MSG_TYPE_SUP 0xFE
#define ECHONET_LT 0x08
#define AUTO_CYCLE 0x04
#define AC_START 0x00
#define AC_STOP 0x01
#define GET_SET 0x03
#define ENERGY_PRICE 0x00
#define TIER 0x01
#define TEMP_OFFSET 0x02
#define TEMP_SETPOINT 0x03
#define PRESENT_TEMPERATURE 0x04

#define MAXPAYLOAD_REQ_OP_CODE1 0x18
#define CLEAR_OP_CODE2 0x00
#define MAXPAYLOAD_RESP 0x19

#define OP_CODE2_REPLY_RESPONSE 0x81
#define OP_CODE2_REPLY 0x80

/* CTA-2045 parser state */
enum cta2045_state {
  STATE_WAIT_START,
  STATE_WAIT_LENGTH,
  STATE_WAIT_PAYLOAD,
  STATE_WAIT_CHECKSUM,
};

typedef enum {
  MPLL_LENGTH2,
  MPLL_LENGTH4,
  MPLL_LENGTH8,
  MPLL_LENGTH16,
  MPLL_LENGTH32,
  MPLL_LENGTH64,
  MPLL_LENGTH128,
  MPLL_LENGTH256,
  MPLL_LENGTH512,
  MPLL_LENGTH1024,
  MPLL_LENGTH1280,
  MPLL_LENGTH1500,
  MPLL_LENGTH2048,
  MPLL_LENGTH4096,
  MPLL_INVALID

} MaxPayloadLengthCode;

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

struct LinkMessage {
  uint8_t msgType1;
  uint8_t msgType2;
} __attribute__((packed));

struct MessageHeader {
  uint8_t msgType1;
  uint8_t msgType2;
  uint16_t length;
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
  uint32_t utcSeconds;
  char timezoneOffsetQuarterHours;
  char dstOffsetQuarterHours;
  uint16_t checksum;
} __attribute__((packed));

struct cta2045_ctx {
  enum cta2045_state state;
  struct cta2045_msg msg; /* defined in common.h */
  uint8_t payload_idx;
  uint8_t calc_checksum;
};

BUILD_ASSERT(MAX_PAYLOAD_LEN <= sizeof(((struct cta2045_msg *)0)->payload),
             "Body max exceeds body array size");

/* Initialize parser context (reset state machine) */
void cta2045_init(struct cta2045_ctx *ctx);

/* Process one incoming byte (call repeatedly for a stream) */
void cta2045_process_byte(struct cta2045_ctx *ctx, uint8_t byte);

/* Queue a response for transmission (IRQ-driven TX).
 * Returns 0 on success, -ENOBUFS if TX ring is full, -ENODEV if UART not
 * ready.
 */
int send_response(const uint8_t *response, size_t len);

/* Start/initialize the UART IRQ path (set callback, enable RX, init rings).
 * Returns 0 on success, <0 on error.
 * If your implementation uses a fixed device inside the .c file, you can keep
 * this as `int cta2045_uart_init(void);` instead—match your .c.
 */
int cta2045_uart_init(void);

/* Optional: expose the RX-consumer thread entry if launched externally */
void cta2045_uart_thread(void *p1, void *p2, void *p3);

#endif /* CTA2045_UART_H */
