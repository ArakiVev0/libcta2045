#include "cta2045_pack.h"
#include "cta2045_types.h"

void process_response(struct MessageHeader *msg);

void process_basic_message(struct BasicMessage *msg);
void process_datalink_message(struct DataLinkMessage *msg);
void process_intermediate_message(struct IntermediateMessage *msg);

IntermediateTypeCode ConvertIntermediateType(uint16_t intermediateType);
DataLinkTypeCode ConvertDataLinkType(uint16_t datalinkType);

void send_link_layer_ACK();
