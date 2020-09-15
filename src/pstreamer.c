#include "unity.h"
#include "stdint.h"
#include "usart.h"
#include "mutex.h"
#include "pstreamer.h"

mutex_t bus_mutex;

int transmit_data(uint8_t *data, size_t size)
{
	int res = 0;
	mutex_lock(bus_mutex);
#ifdef OVER_CAN
	res = send_over_can(data, size);
#else
	res = send_over_uart(data, size);
#endif
	mutex_unlock(bus_mutex); 
	return res;
}

uint8_t NRZI_ENCODE(uint8_t data)
{
	return data;
}


int send_over_uart(uint8_t *data, size_t size)
{
	// Header
	if (usart_transmit(0x7E) != 0) {
		return OUT_OF_MEMORY;
	}
	
	for (size_t i = 0; i<size; i++)
	{
		uint8_t nrzi_data = NRZI_ENCODE(data[i]);
		if (usart_transmit(nrzi_data) != 0) {
			return OUT_OF_MEMORY;
		}
	} 

	// Tail
	if (usart_transmit(0x7E) != 0) {
		return OUT_OF_MEMORY;
	}
	
	return TRANSMIT_OK;
}

enum {
	IDLE,
	HEADER,
	BODY,
	CRC,
	TAIL,
} packet_state_t;

packet_state_t packet_state = IDLE;
size_t packet_size = 0;
size_t bytes_received = 0;
uint16_t packet_crc = 0;
uint16_t header_crc = 0;

void sm_reset()
{
	packet_state = IDLE;
	packet_size = 0;
	bytes_received = 0;
	packet_crc = 0;
	header_crc = 0;
	header_hcs = 0xAAAA; // dummy number to be different from header_crc initial value
	packet_fcs = 0x5555; // dummy number to be different from packet_crc initial value
}

// "0x7E SZ1 SZ0 HCS1 HCS0 D1 D2 ... DN CS1 CS0 0x7E"

int parse_header(size_t offset, uint8_t data)
{
	ASSERT(offset < HEADER_SIZE);

	packet_size <<= 8;
	packet_size |= data;
	bytes_received++;
	if (offset >= HEADER_SIZE) {
		if (header_crc == header_hfc) {
			packet_state = BODY;
			if (packet_size - bytes_received < FCS_SIZE) {
				packet_state = BODY;
			}
		}
	}

	// crc ok, header received complitely
	return 0;

	// await next header's byte
	return 1;

	// header crc error
	return -1;

	// frame size error
	return -2;
}

void sm_process(uint8_t data)
{
	if (data == 0x7E) {
		if (packet_state == IDLE) {
			packet_state = HEADER; 
		}
		else if (packet_state == TAIL) {
			if (packet_fcs == packet_crc) {
				tsrb_commit();
				send_msg(packet_size);
				sm_reset();
			}
		}
		else {
			// Frame error
			sm_reset();
		}

		return;
	}

	// drop all data except 0x7E if we haven't received SOF.
	if (packet_state == IDLE) {
		sm_reset();
		return;
	}

	bytes_received++;
	header_crc = crc16(header_crc, data);
	packet_crc = crc16(packet_crc, data);
	switch (packet_state)
	{
		case HEADER:
			break;
		case BODY:
			break;
		default:
			break;
	}

}

void USART_Handler(void)
{
	if (usart_status(RECEIVED)) {
		uint8_t data = usart_read();
	}
}
