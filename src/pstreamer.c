#include <stdint.h>
#include <string.h>
#include "usart.h"
#include "can.h"
#include "mutex.h"
#include "tsrb_ext.h"
#include "crc.h"
#include "message.h"
#include "sm.h"
#include "pstreamer.h"
#include "pstreamer_usart.h"
#include "pstreamer_can.h"


STATIC mutex_t bus_mutex;

int send_data(uint8_t *data, size_t size)
{
	int res = 0;
	mutex_lock(&bus_mutex);
#ifdef PSTREAMER_OVER_CAN
	res = send_over_can(data, size);
#else
	res = send_over_uart(data, size);
#endif
	mutex_unlock(&bus_mutex);
	return res;
}

void pstreamer_init(void)
{
	mutex_init(&bus_mutex);
#ifdef PSTREAMER_OVER_CAN
	bam_sm_reset();
#else
	parser_reset();
#endif
}

void USART_Handler(void)
{
	if (usart_status(USART_RECEIVED)) {
		uint8_t data = usart_read();
		parser_feed(data);
	}
}

void CAN_Handler(void)
{
	if (can_status(PACKAGE_RECEIVED)) {
		uint32_t can_id = 0; //  = get_canid();
		uint8_t *data = NULL; // = get_payload();
		feed_parser(can_id, data);
	}
}
