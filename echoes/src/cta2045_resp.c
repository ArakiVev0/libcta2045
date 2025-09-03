#include "cta2045_resp.h"
#include "cta2045_pack.h"
#include "cta2045_uart.h"
#include <stdint.h>
#include <zephyr/sys/byteorder.h>

void send_link_layer_NAK(LinkLayerNakCode error_code) {
  uint8_t buf[64];
  size_t n;

  n = nak_pack(error_code, buf, sizeof(buf));
  if (n) {
    send_response(buf, n);
  }
}

void send_link_layer_ACK() {
  uint8_t buf[64];
  size_t n;
  n = ack_pack(buf, sizeof(buf));
  if (n) {
    send_response(buf, n);
  }
}

void process_response(struct MessageHeader *msg_header) {
  if (msg_header->msgType1 == BASIC_MSG_TYP1 ||
      msg_header->msgType2 == BASIC_MSG_TYP2) {
    struct BasicMessage *basic = (struct BasicMessage *)msg_header;
    process_basic_message(basic);
  } else if (msg_header->msgType1 == DATALINK_MSG_TYP1 ||
             msg_header->msgType2 == DATALINK_MSG_TYP2) {
    struct DataLinkMessage *datalink = (struct DataLinkMessage *)msg_header;
    process_datalink_message(datalink);
  } else if (msg_header->msgType1 == INTERMEDIATE_MSG_TYP1 ||
             msg_header->msgType2 == INTERMEDIATE_MSG_TYP2) {
    struct IntermediateMessage *intermediate =
        (struct IntermediateMessage *)msg_header;
    process_intermediate_message(intermediate);
  } else {
    send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
  }
}

void process_basic_message(struct BasicMessage *msg) {
  switch (msg->opCode1) {
  case APP_ACK:
    send_link_layer_ACK();
    break;

  case APP_NAK:
    send_link_layer_ACK();
    break;

  case OPER_STATE_RESP:
    send_link_layer_ACK();
    break;

  case CUST_OVERRIDE:
    send_link_layer_ACK();

    struct BasicMessage resp;
    resp.msgType1 = BASIC_MSG_TYP1;
    resp.msgType2 = BASIC_MSG_TYP2;
    resp.length = sys_cpu_to_be16(2);
    resp.opCode1 = APP_ACK;
    resp.opCode2 = CUST_OVERRIDE;
    resp.checksum = checksum_calc((uint8_t *)&resp, sizeof(resp) - 2);
    send_response((uint8_t *)&resp, sizeof(resp));

    break;

  default:
    send_link_layer_NAK(LLN_UNSUPPORTED_MESSAGE_TYPE);
    break;
  }
}

void process_datalink_message(struct DataLinkMessage *msg) {
  DataLinkTypeCode messageType = ConvertDataLinkType(msg->opCode1);
  switch (messageType) {
  case DLT_MAX_PAYLOAD_REQUEST:
    send_link_layer_ACK();

    struct DataLinkMessage resp;
    resp.msgType1 = DATALINK_MSG_TYP1;
    resp.msgType2 = DATALINK_MSG_TYP2;
    resp.length = sys_cpu_to_be16(2);
    resp.opCode1 = MAXPAYLOAD_RESP;
    resp.opCode2 = (uint8_t)get_max_payload_length();
    resp.checksum = checksum_calc((uint8_t *)&resp, sizeof(resp) - 2);

    send_response((uint8_t *)&resp, sizeof(resp));
    break;
  case DLT_MAX_PAYLOAD_RESPONSE:
    // MaxPayloadLengthCode maxPayloadLength =
    // convert_max_payload_length(msg->opCode2);
    send_link_layer_ACK();
    break;
  default:
    send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
    break;
  }
}

void process_intermediate_message(struct IntermediateMessage *msg) {
  uint16_t intermediateType = *((uint16_t *)(&(msg->opCode1)));

  switch (intermediateType) {
  case IT_INFO_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_UTC_TIME_REQUEST:
    if (sys_cpu_to_be16(msg->length) == 2) {
      send_link_layer_ACK();
      struct GetUTCTimeResponse resp;

      resp.utcSeconds = 0;
      resp.timezoneOffsetQuarterHours = 0;
      resp.dstOffsetQuarterHours = 0;

      resp.msgType1 = INTERMEDIATE_MSG_TYP1;
      resp.msgType2 = INTERMEDIATE_MSG_TYP2;
      resp.length = sys_be16_to_cpu(sizeof(msg->length) - 6);
      resp.opCode1 = GET_UTC_TIME;
      resp.opCode2 = OP_CODE2_REPLY;
      resp.responseCode = OP_CODE2_REPLY;
      resp.checksum = checksum_calc((uint8_t *)&resp, sizeof(resp) - 2);

      send_response((uint8_t *)&resp, sizeof(resp));
    } else {
      send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
    }
    break;

  case IT_COMMODITY_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_SET_ENERGY_PRICE_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_SET_SETPOINT_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_GET_PRESENT_TEMPERATURE_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_START_CYCLING_RESPONSE:
    send_link_layer_ACK();
    break;

  case IT_TERMINATE_CYCLING_RESPONSE:
    send_link_layer_ACK();
    break;

  default:
    send_link_layer_NAK(LLN_REQUEST_NOT_SUPPORTED);
    break;
  }
}
