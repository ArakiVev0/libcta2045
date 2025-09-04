#include "cta2045_resp.h"
#include "cta2045_pack.h"
#include "cta2045_types.h"
#include "cta2045_uart.h"
#include <stddef.h>
#include <stdio.h>
#include <zephyr/sys/byteorder.h>

void process_response(struct MessageHeader *msg_header) {
  uint8_t buf[64];
  size_t n;
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
    n = nak_pack(LLN_REQUEST_NOT_SUPPORTED, buf, sizeof(buf));
    if (n) {
      send_response(buf, sizeof(buf));
    }
  }
}

void process_basic_message(struct BasicMessage *msg) {
  uint8_t buf[64];
  size_t n;
  switch (msg->opCode1) {

  case APP_ACK:
    n = ack_pack(buf, sizeof(buf));
    printf("Hello world\n");
    if (n) {
      send_response(buf, n);
    }
    break;

  case APP_NAK:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case OPER_STATE_RESP:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case CUST_OVERRIDE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }

    basic_pack(APP_ACK, CUST_OVERRIDE, buf, sizeof(buf));

    if (n) {
      send_response(buf, n);
    }
    break;

  default:
    n = nak_pack(LLN_UNSUPPORTED_MESSAGE_TYPE, buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;
  }
}

void process_datalink_message(struct DataLinkMessage *msg) {
  DataLinkTypeCode messageType = ConvertDataLinkType(msg->opCode1);

  uint8_t buf[64];
  size_t n;

  switch (messageType) {

  case DLT_MAX_PAYLOAD_REQUEST:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }

    n = datalink_pack_max_payload_req(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }

    break;
  case DLT_MAX_PAYLOAD_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;
  default:
    n = nak_pack(LLN_REQUEST_NOT_SUPPORTED, buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;
  }
}

void process_intermediate_message(struct IntermediateMessage *msg) {
  uint16_t intermediateType = *((uint16_t *)(&(msg->opCode1)));
  uint8_t buf[64];
  size_t n;
  switch (intermediateType) {
  case IT_INFO_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_GET_UTC_TIME_REQUEST:
    if (sys_cpu_to_be16(msg->length) == 2) {
      n = ack_pack(buf, sizeof(buf));
      if (n) {
        send_response(buf, n);
      }
      struct GetUTCTimeResponse resp;

      n = intermediate_pack_get_utc_time_resp(buf, sizeof(buf));
      if (n) {
        send_response((uint8_t *)&resp, sizeof(resp));
      }
    } else {
      n = nak_pack(LLN_REQUEST_NOT_SUPPORTED, buf, sizeof(buf));
      if (n) {
        send_response(buf, n);
      }
    }
    break;

  case IT_COMMODITY_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_SET_ENERGY_PRICE_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_GET_SET_SETPOINT_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_GET_PRESENT_TEMPERATURE_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_START_CYCLING_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_TERMINATE_CYCLING_RESPONSE:
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  default:
    n = nak_pack(LLN_REQUEST_NOT_SUPPORTED, buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;
  }
}

IntermediateTypeCode ConvertIntermediate(uint16_t intermediateType) {
  uint16_t host = sys_be16_to_cpu(intermediateType);
  switch (host) {
  case IT_INFO_REQUEST:
    return IT_INFO_REQUEST;
  case IT_INFO_RESPONSE:
    return IT_INFO_RESPONSE;
  case IT_GET_UTC_TIME_REQUEST:
    return IT_GET_UTC_TIME_REQUEST;
  case IT_COMMODITY_REQUEST:
    return IT_COMMODITY_REQUEST;
  case IT_COMMODITY_RESPONSE:
    return IT_COMMODITY_RESPONSE;
  case IT_SET_ENERGY_PRICE_RESPONSE:
    return IT_SET_ENERGY_PRICE_RESPONSE;
  case IT_GET_SET_TEMPERATURE_OFFSET_REQUEST:
    return IT_GET_SET_TEMPERATURE_OFFSET_REQUEST;
  case IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE:
    return IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE;
  case IT_GET_SET_SETPOINT_RESPONSE:
    return IT_GET_SET_SETPOINT_RESPONSE;
  case IT_GET_PRESENT_TEMPERATURE_RESPONSE:
    return IT_GET_PRESENT_TEMPERATURE_RESPONSE;
  case IT_START_CYCLING_RESPONSE:
    return IT_START_CYCLING_RESPONSE;
  case IT_TERMINATE_CYCLING_RESPONSE:
    return IT_TERMINATE_CYCLING_RESPONSE;
  default:
    return IT_INVALID;
  }
}

DataLinkTypeCode ConvertDataLinkType(uint8_t datalinkType) {
  switch (datalinkType) {
  case DLT_MAX_PAYLOAD_REQUEST:
    return DLT_MAX_PAYLOAD_REQUEST;
  case DLT_MAX_PAYLOAD_RESPONSE:
    return DLT_MAX_PAYLOAD_RESPONSE;
  case DLT_SEND_NEXT_COMMAND_TO_SLOT:
    return DLT_SEND_NEXT_COMMAND_TO_SLOT;
  default:
    return DLT_INVALID;
  }
}
