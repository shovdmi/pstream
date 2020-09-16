#ifndef PSTREAMER_H
#define PSTREAMER_H

#define TRANSMIT_OK      (0)
#define OUT_OF_MEMORY    (-2)
#define CONNECTION_ERROR (-3)

enum sm_state_t {
	SM_IDLE = 0,
	SM_HEADER,
	SM_PAYLOAD,
	SM_TAIL_CRC,
	SM_TAIL_EOF,
}; 

struct packet_t {
	size_t bytes_received;
	size_t size;
	uint16_t crc;
	uint16_t fcs;
	uint8_t prev_data;
};



int transmit_data(uint8_t *data, size_t size);
int send_over_uart(uint8_t *data, size_t size);
int packet_sm(uint8_t data);
int sm_reset(void);

#endif //PSTREAMER_H
