#include "unity.h"

#include "mock_mutex.h"
#include "mock_usart.h"
#include "mock_can.h"
#include "mock_tsrb_ext.h"
#include "mock_crc.h"
#include "mock_message.h"
#include "mock_sm.h"
#include "pstreamer.h"
#include "pstreamer_can.h"
#include "pstreamer_usart.h"


extern struct BAM_sm_t bam_sm;

void setUp(void)
{
}

void tearDown(void)
{
}

/* -----------------------------------------------------------------------------
 *
 *          Tests of wrapping data sending (over USART)
 *
 * ---------------------------------------------------------------------------*/

void test_pstreamer_send_over_can(void)
{
	uint8_t test_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	for (size_t i = 0; i < 8; i++)
	{
		transmit_P2P_package_ExpectAndReturn(test_data, i, 0);
		send_over_can(test_data, i);
	}
}
