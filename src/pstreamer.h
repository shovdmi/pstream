#ifndef PSTREAMER_H
#define PSTREAMER_H

#define TRANSMIT_OK      (0)
#define OUT_OF_MEMORY    (-2)
#define CONNECTION_ERROR (-3)

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

enum parser_state_t {
	SM_IDLE = 0,
	SM_HEADER,
	SM_PAYLOAD,
	SM_TAIL_CRC,
	SM_TAIL_EOF,
}; 

struct parser_t {
	size_t bytes_received;
	size_t packet_size;
	uint16_t crc;
	uint16_t fcs;
	uint8_t prev_data;
};



int send_data(uint8_t *data, size_t size);
int send_over_uart(uint8_t *data, size_t size);
int parser_feed(uint8_t data);
int parser_reset(void);

#endif //PSTREAMER_H
