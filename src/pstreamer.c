#include "unity.h"
#include "stdint.h"
#include "usart.h"
#include "mutex.h"
#include "tsrb_ext.h"
#include "crc.h"
#include "message.h"
#include "sm.h"
#include "pstreamer.h"
#include <string.h>

#define USART_RECEIVED (1 << 0)

// 2 bytes packet.size
#define HEADER_SIZE (2)
// 2 bytes CRC + 1 byte EOF
#define TAIL_SIZE (3)

#ifdef OVER_CAN
// 255 chunks * 7 bytes per chunk
#  define PACKET_MAX_SIZE (255 * 7)
#else
#  define PACKET_MAX_SIZE (1024)
#endif

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


enum sm_state_t sm_state = SM_IDLE;
struct packet_t packet;

int sm_reset(void)
{
	tsrb_reject();
	memset(&packet, 0, sizeof(struct packet_t));
	return 0;
}

uint8_t  bit_stuff(uint8_t data)
{
	return data;
}

int parse_0x7E(void)
{
	switch (sm_state)
	{
		case SM_IDLE:
			sm_reset();
			sm_state = sm_change_state(SM_HEADER);
			break;
		case SM_TAIL_EOF:
			tsrb_commit();
			send_msg(packet.bytes_received - TAIL_SIZE - 1); // -1 because we received 0x7E and didn't increased bytes_received yet
			sm_reset();
			sm_state = sm_change_state(SM_IDLE);
			break;

		case SM_HEADER:
			__attribute__((fallthrough));
		case SM_PAYLOAD:
			__attribute__((fallthrough));
		case SM_TAIL_CRC:
			__attribute__((fallthrough));
		default:
			// Frame error
			sm_reset();
			sm_state = sm_change_state(SM_HEADER);
			break;
	}

	return 0;
}

int packet_sm(uint8_t data)
{
	if (data == 0x7E) {
		// drop all data except 0x7E if we haven't received SOF or expecting EOF.
		parse_0x7E();
		return 0;
	}
	else {
		packet.bytes_received++;
		data = bit_stuff(data);
	}

	if ((sm_state == SM_HEADER) || (sm_state == SM_PAYLOAD)) {
		packet.crc = calc_crc16(packet.crc, data);
	}


	switch (sm_state)
	{
		case SM_IDLE:
			// reject all the data except 0x7E
			break;
		case SM_HEADER:
			packet.size <<= 8;
			packet.size |= data;
			if (packet.bytes_received >= HEADER_SIZE) {
				if (packet.size > PACKET_MAX_SIZE) {
					// Reset on error
					sm_reset();
					sm_state = sm_change_state(SM_IDLE);
				}
				else {
					sm_state = sm_change_state(SM_PAYLOAD);
				}
			}
			break;
		case SM_PAYLOAD:
			tsrb_add_tmp(data);
			if (packet.size - packet.bytes_received <= TAIL_SIZE) {
				sm_state = sm_change_state(SM_TAIL_CRC);
			}
			break;
		case SM_TAIL_CRC:
			packet.fcs <<=8;
			packet.fcs |= data;
			if (packet.size - packet.bytes_received <= 1) {
				if (packet.crc == packet.fcs) {
					sm_state = sm_change_state(SM_TAIL_EOF);
				}
				else {
					// CRC error
					sm_reset();
					sm_state = sm_change_state(SM_IDLE);
				}
			}
			break;
		case SM_TAIL_EOF:
			// We must only get 0x7E if SM in this state, reset otherwise
			sm_reset();
			sm_state = sm_change_state(SM_IDLE);
			break;

		default:
			break;
	}
	return 0;
}


void USART_Handler(void)
{
	if (usart_status(USART_RECEIVED)) {
		uint8_t data = usart_read();
		packet_sm(data);
	}
}
