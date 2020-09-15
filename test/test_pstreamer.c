#include "unity.h"

#include "mock_mutex.h"
#include "mock_usart.h"
#include "mock_tsrb_ext.h"
#include "mock_crc.h"
#include "mock_message.h"
#include "mock_sm.h"
#include "pstreamer.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_pstreamer_transmit_over_uart_on_no_errors(void)
{

	uint8_t test_data[] = { 0, 1, 2, 3, 4, 5, 6, 7};

	mutex_lock_ExpectAndReturn(0, 0);

	usart_transmit_ExpectAndReturn(0x7E, TRANSMIT_OK);

	for (int i = 0; i < sizeof(test_data) ;i++)
	{
		usart_transmit_ExpectAndReturn(test_data[i], TRANSMIT_OK);
	}

	usart_transmit_ExpectAndReturn(0x7E, TRANSMIT_OK);

	mutex_unlock_ExpectAndReturn(0, 0);
	

	int result = transmit_data(&test_data[0], sizeof(test_data));
	TEST_ASSERT_EQUAL_INT(TRANSMIT_OK, result);
}

void test_pstreamer_transmit_over_uart_if_Out_Of_Memory_error_happend(void)
{

       uint8_t test_data[16];

       mutex_lock_ExpectAndReturn(0, 0);

       usart_transmit_ExpectAndReturn(0x7E, TRANSMIT_OK);

       for (int i = 0; i < sizeof(test_data) ;i++)
       {
               test_data[i] = i;
               usart_transmit_ExpectAndReturn(test_data[i], TRANSMIT_OK);
       }

       usart_transmit_ExpectAndReturn(0x7E, OUT_OF_MEMORY);

       mutex_unlock_ExpectAndReturn(0, 0);
       

       int result = transmit_data(&test_data[0], sizeof(test_data));
       TEST_ASSERT_EQUAL_INT(OUT_OF_MEMORY, result);
}

