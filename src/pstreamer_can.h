#ifndef PSTREAMER_CAN_H
#define PSTREAMER_CAN_H

enum BAM_sm_state_t {
	IDLE = 0,
	RECEIVING,
};

struct BAM_sm_t {
	size_t bytes;
	size_t packages;
	size_t packages_received;
	uint32_t timestamp;
	enum BAM_sm_state_t state;
};

int send_over_can(uint8_t *data, size_t size);
int feed_parser(uint32_t can_id, uint8_t data[8]);
int bam_sm_reset(struct BAM_sm_t *bam_sm);

#endif //PSTREAMER_CAN_H
