#include "cta2045_pack.h"
#include "cta2045_types.h"

void process_response(struct MessageHeader *msg);

void process_basic_message(struct BasicMessage *msg);
void process_datalink_message(struct DataLinkMessage *msg);
void process_intermediate_message(struct IntermediateMessage *msg);

void process_get_device_info_response(struct IntermediateMessage *resp);
void process_commodity_response(struct IntermediateMessage *resp);
void process_set_temperature_offset_response(struct IntermediateMessage *resp);
void process_get_temperature_offset_response(struct IntermediateMessage *resp);

IntermediateTypeCode ConvertIntermediateType(uint16_t intermediateType);
DataLinkTypeCode ConvertDataLinkType(uint8_t datalinkType);
