#include <stdint.h>
#include <string.h>
#include "can.h"
#include "mutex.h"
#include "tsrb_ext.h"
#include "crc.h"
#include "message.h"
#include "sm.h"
#include "pstreamer.h"
#include "pstreamer_usart.h"


int send_over_can(uint8_t *data, size_t size)
{
	if (size <= 8) {
		return transmit_p2p_packet(data, size);
	}
	
	size_t packets_total = size / 7;

	if (transmit_TP_CM_packet(packets_num, size) != 0) {
		return CONNECTION_ERROR;
	}

	for (int i = 0; i < size; i++)
	{
		uint8_t bytes = size - i * 7;
		if (bytes > 7) {
			bytes = 7;
		}

		if (transmit_TP_DT_packet(i, &data[i], bytes)) {
			return CONNECTION_ERROR;
		}
	}
}

enum BAM_state_t {
	IDLE = 0,
	RECEIVING,
};

struct BAM_sm_t {
	size_t bytes;
	size_t packets;
	size_t packets_received;

	enum BAM_state_t state;
} bam_sm;

int bam_start(BAM_sm_t *bam_sm, uint8_t data[8])
{
	tsrb_reject();
	bam_sm->packets_received = 0;

	bam_sm->bytes = data[0];
	bam_sm->packets = data[1];
	bam_sm->state = RECEIVING;
}


int bam_feed(BAM_sm_t *bam_sm, uint8_t data[8])
{
	if (bam_sm->state == IDLE) {
		bam_reset(bam_sm);

	}

	if (bam_sm->state == RECEIVING) {
		if (bam_sm->packet_received + 1 != data[0]) {
			bam_reset(bam_sm);
			return -1;
		}

		for (size_t i = 1; i < 7; i++) {
			trsb_add_tmp(data[i]);
		}
		bam_sm->packet_received++;
	}

	return 0;
}

/* \breif filter out TP_CM and TP_DT from bus packet's id
 *
 */
uint32_t parse_id(uint32_t _id)
{
	return _id;
}


int feed_parser(uint32_t id, uint8_t data[8])
{
	int res = 0;

	id = parse_id(id);

	if ((id != TP_CM) || (id != TP_DM)) {
		return 0;
	}

	switch (id)
	{
	case TP_CM:
		res = bam_start(&bam_sm, data);
		break;
	case TP_DT:
		res = bam_feed(&bam_sm, data);
		break;
	default:
		break;
	}

	return res;

}
