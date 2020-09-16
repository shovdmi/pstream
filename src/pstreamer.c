#include <stdint.h>
#include <string.h>
#include "usart.h"
#include "mutex.h"
#include "tsrb_ext.h"
#include "crc.h"
#include "message.h"
#include "sm.h"
#include "pstreamer.h"
#include "pstreamer_usart.h"


STATIC mutex_t bus_mutex;

int send_data(uint8_t *data, size_t size)
{
	int res = 0;
	mutex_lock(&bus_mutex);
#ifdef OVER_CAN
	res = send_over_can(data, size);
#else
	res = send_over_uart(data, size);
#endif
	mutex_unlock(&bus_mutex);
	return res;
}

void USART_Handler(void)
{
	if (usart_status(USART_RECEIVED)) {
		uint8_t data = usart_read();
		parser_feed(data);
	}
}
