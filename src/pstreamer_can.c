#include <stdint.h>
#include <string.h>
#include "can.h"
#include "mutex.h"
#include "tsrb_ext.h"
#include "crc.h"
#include "message.h"
#include "sm.h"
#include "pstreamer.h"
#include "pstreamer_can.h"

#define CEILING(x,y) ((x) + (y) - 1) / (y)

STATIC struct BAM_sm_t bam_sm;

STATIC uint32_t time_now()
{
	return 0;
}

/* -----------------------------------------------------------------------------
 *              Transmitting
 *
 * -------------------------------------------------------------------------- */
int send_over_can(uint8_t *data, size_t size)
{
	if (size <= 7) {
		return transmit_P2P_package(data, size);
	}
	
	size_t packages_total = CEILING(size, 7);

	if (transmit_TP_CM_package(packages_total, size) != 0) {
		return CONNECTION_ERROR;
	}

	for (int i = 0; i < packages_total; i++)
	{
		uint8_t bytes = size - i * 7;
		if (bytes > 7) {
			bytes = 7;
		}

		if (transmit_TP_DT_package(i, &data[i], bytes)) {
			return CONNECTION_ERROR;
		}
	}
}

/* -----------------------------------------------------------------------------
 *              Receiving State-machine
 *
 * -------------------------------------------------------------------------- */


int bam_sm_reset(struct BAM_sm_t *bam_sm)
{
	tsrb_reject();
	memset(bam_sm, 0, sizeof(struct BAM_sm_t));
}

STATIC int bam_start(struct BAM_sm_t *bam_sm, uint8_t data[8])
{
	bam_sm_reset(bam_sm);

	size_t bytes = data[1] + (data[2] << 8);
	size_t packages =  data[3];

	if (bytes > PACKET_MAX_SIZE) {
		return -1;
	}

	if (CEILING(bytes, 7) != packages) {
		return -1;
	}

	bam_sm->bytes = bytes;
	bam_sm->packages = packages;
	bam_sm->state = sm_change_state(RECEIVING);

}


STATIC int bam_feed(struct BAM_sm_t *bam_sm, uint8_t data[8])
{
	if (bam_sm->state != RECEIVING) {
		bam_sm_reset(bam_sm);
		return 0;
	}

	// bam_sm->state is RECEIVING
	//
	uint32_t now = time_now();
	if (now - bam_sm->timestamp > BAM_TIMEOUT) {
		bam_sm_reset(bam_sm);
		return -1;
	}
	bam_sm->timestamp = now;

	bam_sm->packages_received++;
	if (bam_sm->packages_received != data[0]) {
		bam_sm_reset(bam_sm);
		return -1;
	}

	size_t bytes = bam_sm->bytes;
	if (bytes > 7) {
		bytes = 7;
	}

	for (size_t i = 1; i < 7; i++) {
		tsrb_add_tmp(data[i]);
	}
	bam_sm->bytes = bam_sm->bytes - bytes;

	if (bam_sm->bytes == 0) {
		size_t bytes = tsrb_commit();
		send_msg(bytes);
		bam_sm_reset(bam_sm);
	}

	return 0;
}

STATIC int p2p_parse(uint8_t data[8])
{
	size_t bytes = data[0];
	
	if (bytes > 7) {
		return -1;
	}

	for (size_t i = 1; i < bytes; i++)
	{
		if (tsrb_add_tmp(data[i])) {
			return -1; //OUT_OF_MEMORY
		}
	}
	tsrb_commit();

	return 0;
}

/* \breif filter out TP_CM and TP_DT from bus packet's id
 *
 */
STATIC uint32_t get_pgn(uint32_t can_id)
{
	return can_id;
}


int feed_parser(uint32_t can_id, uint8_t data[8])
{
	int res = 0;

	uint32_t pgn = get_pgn(can_id);

	if ((pgn != TP_CM) || (pgn != TP_DT) || (pgn != P2P_PGN)) {
		return 0;
	}

	switch (pgn)
	{
	case TP_CM:
		res = bam_start(&bam_sm, data);
		break;
	case TP_DT:
		res = bam_feed(&bam_sm, data);
		break;
	case P2P_PGN:
		res = p2p_parse(data);
		break;
	default:
		break;
	}

	if (res != 0) {
		bam_sm_reset(&bam_sm);
	}

	return res;
}
