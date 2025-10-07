#include "cta2045_resp.h"
#include "cta2045_pack.h"
#include "cta2045_types.h"
#include "cta2045_uart.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(cta2045, LOG_LEVEL_INF);

void process_response(struct MessageHeader *msg_header) {
  uint8_t buf[64];
  size_t n;
  if (msg_header->msgType1 == BASIC_MSG_TYP1 &&
      msg_header->msgType2 == BASIC_MSG_TYP2) {
    printf("Basic");
    struct BasicMessage *basic = (struct BasicMessage *)msg_header;
    process_basic_message(basic);
  } else if (msg_header->msgType1 == DATALINK_MSG_TYP1 &&
             msg_header->msgType2 == DATALINK_MSG_TYP2) {
    printf("Datalink");
    struct DataLinkMessage *datalink = (struct DataLinkMessage *)msg_header;
    process_datalink_message(datalink);
  } else if (msg_header->msgType1 == INTERMEDIATE_MSG_TYP1 &&
             msg_header->msgType2 == INTERMEDIATE_MSG_TYP2) {
    printf("Intermediate");
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
    printf("\tAPP ACK\n");
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case APP_NAK:
    printf("\tAPP NAK Error Code: 0x%02X\n", msg->opCode2);
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case OPER_STATE_RESP:
    printf("\tOperational State Response\n");
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case CUST_OVERRIDE:
    printf("\tCustomer Override\n");
    n = ack_pack(buf, sizeof(buf));

    if (n) {
      send_response(buf, n);
    }

    n = basic_pack(APP_ACK, CUST_OVERRIDE, buf, sizeof(buf));

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
    printf("\tMaximum payload length?\n");
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }

    n = datalink_pack(MAXPAYLOAD_REQ_OP_CODE1, CLEAR_OP_CODE2, buf,
                      sizeof(buf));
    if (n) {
      send_response(buf, n);
    }

    break;
  case DLT_MAX_PAYLOAD_RESPONSE:
    printf("\tMaximum payload length\n");
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;
  default:
    printf("\tRequest Not Support\n");
    n = nak_pack(LLN_REQUEST_NOT_SUPPORTED, buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;
  }
}

void process_get_device_info_response(struct IntermediateMessage *resp) {
  struct GetInfoResponse *msg = (struct GetInfoResponse *)resp;
  LOG_INF("=== GetInfoResponse ===");
  LOG_INF("msgType1=0x%02X  msgType2=0x%02X  length=%u", msg->msgType1,
          msg->msgType2, sys_be16_to_cpu(msg->length));
  LOG_INF("opCode1=0x%02X  opCode2=0x%02X  responseCode=0x%02X", msg->opCode1,
          msg->opCode2, msg->responseCode);
  LOG_INF("version=\"%.*s\"  vendorId=0x%04X  deviceType=0x%04X  "
          "revision=%02X %02X",
          2, msg->version, sys_be16_to_cpu(msg->vendorId),
          sys_be16_to_cpu(msg->deviceType), msg->deviceRevision[0],
          msg->deviceRevision[1]);
  LOG_INF("capability=%02X %02X %02X %02X  reserved=0x%02X", msg->capability[0],
          msg->capability[1], msg->capability[2], msg->capability[3],
          msg->reserved);
  LOG_INF("modelNumber=\"%.*s\"", 16, msg->modelNumber);
  LOG_INF("serialNumber=\"%.*s\"", 16, msg->serialNumber);
  LOG_INF("firmware=20%02u-%02u-%02u  fwVersion=%u.%u", msg->firmwareYear20xx,
          msg->firmwareMonth, msg->firmwareDay, msg->firmwareMajor,
          msg->firmwareMinor);
  LOG_INF("checksum=0x%04X", sys_be16_to_cpu(msg->checksum));
  LOG_INF("=======================");
}

void process_commodity_response(struct IntermediateMessage *resp) {
  struct CommodityResponse *msg = (struct CommodityResponse *)resp;
}

void process_set_temperature_offset_response(struct IntermediateMessage *resp) {
}

void process_get_temperature_offset_response(struct IntermediateMessage *resp) {
  struct GetTemperatureOffsetResponse *msg =
      (struct GetTemperatureOffsetResponse *)resp;
}

void process_intermediate_message(struct IntermediateMessage *msg) {
  uint16_t IntermediateType = *((uint16_t *)(&(msg->opCode1)));
  IntermediateTypeCode messageType = ConvertIntermediateType(IntermediateType);

  uint16_t len = msg->length;

  uint8_t buf[64];
  size_t n;
  switch (messageType) {
  case IT_INFO_RESPONSE:
    printf("\tGetInformation() - Reply\n");

    process_get_device_info_response(msg);

    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_GET_UTC_TIME_REQUEST:
    if (sys_cpu_to_be16(len) == 2) {
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
    printf("\tCommodity response\n");

    process_commodity_response(msg);

    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_SET_ENERGY_PRICE_RESPONSE:
    printf("\tSet Energy Price Response\n");
    n = ack_pack(buf, sizeof(buf));
    if (n) {
      send_response(buf, n);
    }
    break;

  case IT_GET_SET_TEMPERATURE_OFFSET_RESPONSE:
    len = sys_be16_to_cpu(len);
    size_t wire_bytes = (size_t)len + 6;

    if (wire_bytes == sizeof(struct IntermediateMessage)) {
      process_set_temperature_offset_response(msg);
    } else if (wire_bytes == sizeof(struct GetTemperatureOffsetResponse)) {
      process_get_temperature_offset_response(msg);
    } else {
    }
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

IntermediateTypeCode ConvertIntermediateType(uint16_t intermediateType) {
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
