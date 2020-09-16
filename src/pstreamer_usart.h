#ifndef PSTREAMER_USART_H
#define PSTREAMER_USART_H

#define USART_RECEIVED (1 << 0)

// 2 bytes parser.packet_size
#define PSTREAM_USART_HEADER_SIZE (2)
// 2 bytes CRC + 1 byte EOF
#define PSTREAM_USART_TAIL_SIZE (3)


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



int send_over_uart(uint8_t *data, size_t size);
int parser_feed(uint8_t data);
int parser_reset(void);

#endif //PSTREAMER_USART_H
