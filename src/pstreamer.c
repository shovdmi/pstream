#include <stdint.h>
#include <string.h>
#include "usart.h"
#include "mutex.h"
#include "tsrb_ext.h"
#include "crc.h"
#include "message.h"
#include "sm.h"
#include "pstreamer.h"

#define USART_RECEIVED (1 << 0)

// 2 bytes parser.packet_size
#define HEADER_SIZE (2)
// 2 bytes CRC + 1 byte EOF
#define TAIL_SIZE (3)

#ifdef OVER_CAN
// 255 chunks * 7 bytes per chunk
#  define PACKET_MAX_SIZE (255 * 7)
#else
#  define PACKET_MAX_SIZE (1024)
#endif

STATIC mutex_t bus_mutex;
STATIC enum parser_state_t parser_state = SM_IDLE;
STATIC struct parser_t parser;


int send_data(uint8_t *data, size_t size)
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

STATIC uint8_t zero_bit_insert(uint8_t prev_data, uint8_t data)
{
	// TODO
	(void)prev_data;
	return data;
}

STATIC uint8_t zero_bit_delete(uint8_t prev_data, uint8_t data)
{
	// TODO
	(void)prev_data;
	return data;
}


int send_over_uart(uint8_t *data, size_t size)
{
	// Start-of-Frame (SOF)
	if (usart_transmit(0x7E) != 0) {
		return OUT_OF_MEMORY;
	}
	
	uint16_t crc16 = 0;
	uint8_t zbi = 0;
	uint8_t prev_zbi = 0;

	// Header: size of the header and payload
	uint16_t header = size;
	for (size_t i = 0; i < sizeof(header); i++)
	{
		crc16 = calc_crc16(crc16, (header & 0xFF));
		zbi = zero_bit_insert(prev_zbi, (header & 0xFF));
		prev_zbi = zbi;
		if (usart_transmit(zbi) != 0) {
			return OUT_OF_MEMORY;
		}
		header >>= 8;
	}

	// Payload
	for (size_t i = 0; i < size; i++)
	{
		crc16 = calc_crc16(crc16, data[i]);
		zbi = zero_bit_insert(prev_zbi, data[i]);
		prev_zbi = zbi;
		if (usart_transmit(zbi) != 0) {
			return OUT_OF_MEMORY;
		}
	} 

	// Tail: CRC16 of the header and payload
	uint16_t tail = crc16;
	for (size_t i = 0; i < sizeof(tail); i++)
	{
		zbi = zero_bit_insert(prev_zbi, (tail & 0xFF));
		prev_zbi = zbi;
		tail >>= 8;
		if (usart_transmit(zbi) != 0) {
			return OUT_OF_MEMORY;
		}
	}

	// End-of-Frame (EOF)
	if (usart_transmit(0x7E) != 0) {
		return OUT_OF_MEMORY;
	}
	
	return TRANSMIT_OK;
}


int parser_reset(void)
{
	tsrb_reject();
	memset(&parser, 0, sizeof(struct parser_t));
	return 0;
}

STATIC int parse_0x7E(void)
{
	switch (parser_state)
	{
		case SM_IDLE:
			parser_reset();
			parser_state = sm_change_state(SM_HEADER);
			break;
		case SM_TAIL_EOF:
			tsrb_commit();
			send_msg(parser.bytes_received - TAIL_SIZE - 1); // -1 because we received 0x7E and didn't increased bytes_received yet
			parser_reset();
			parser_state = sm_change_state(SM_IDLE);
			break;

		case SM_HEADER:
			__attribute__((fallthrough));
		case SM_PAYLOAD:
			__attribute__((fallthrough));
		case SM_TAIL_CRC:
			__attribute__((fallthrough));
		default:
			// Frame error
			parser_reset();
			parser_state = sm_change_state(SM_HEADER);
			break;
	}

	return 0;
}

int parser_feed(uint8_t data)
{
	if (data == 0x7E) {
		// drop all data except 0x7E if we haven't received SOF or expecting EOF.
		parse_0x7E();
		return 0;
	}
	else {
		parser.bytes_received++;
		data = zero_bit_delete(parser.prev_data, data);
		parser.prev_data = data;
	}

	if ((parser_state == SM_HEADER) || (parser_state == SM_PAYLOAD)) {
		parser.crc = calc_crc16(parser.crc, data);
	}


	switch (parser_state)
	{
		case SM_IDLE:
			// reject all the data except 0x7E
			break;
		case SM_HEADER:
			parser.packet_size <<= 8;
			parser.packet_size |= data;
			if (parser.bytes_received >= HEADER_SIZE) {
				if (parser.packet_size > PACKET_MAX_SIZE) {
					// Reset on error
					parser_reset();
					parser_state = sm_change_state(SM_IDLE);
				}
				else {
					parser_state = sm_change_state(SM_PAYLOAD);
				}
			}
			break;
		case SM_PAYLOAD:
			tsrb_add_tmp(data);
			if (parser.packet_size - parser.bytes_received <= TAIL_SIZE) {
				parser_state = sm_change_state(SM_TAIL_CRC);
			}
			break;
		case SM_TAIL_CRC:
			parser.fcs <<=8;
			parser.fcs |= data;
			if (parser.packet_size - parser.bytes_received <= 1) {
				if (parser.crc == parser.fcs) {
					parser_state = sm_change_state(SM_TAIL_EOF);
				}
				else {
					// CRC error
					parser_reset();
					parser_state = sm_change_state(SM_IDLE);
				}
			}
			break;
		case SM_TAIL_EOF:
			// We must only get 0x7E if SM in this state, reset otherwise
			parser_reset();
			parser_state = sm_change_state(SM_IDLE);
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
		parser_feed(data);
	}
}
