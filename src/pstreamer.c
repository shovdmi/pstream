#include "unity.h"
#include "stdint.h"
#include "usart.h"
#include "mutex.h"
#include "pstreamer.h"

mutex_t bus_mutex;

int transmit_data(uint8_t *data, size_t size)
{
	mutex_lock(bus_mutex);
#ifdef OVER_CAN
	send_over_can(data, size);
#else
	send_over_uart(data, size);
#endif
	mutex_unlock(bus_mutex);
}

uint8_t NRZI_ENCODE(uint8_t data)
{
	return data;
}


int send_over_uart(uint8_t *data, size_t size)
{
	// Header
	if (usart_transmit(0x7E)) {
		return OUT_OF_MEMORY;
	}
	
	for (size_t i = 0; i<size; i++)
	{
		uint8_t nrzi_data = NRZI_ENCODE(data[i]);
		if (usart_transmit(nrzi_data)) {
			return OUT_OF_MEMORY;
		}
	} 

	// Tail
	if (usart_transmit(0x7E)) {
		return OUT_OF_MEMORY;
	}
	
	return TRANSMIT_OK;
}
