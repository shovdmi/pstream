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

extern struct packet_t packet;
extern enum sm_state_t sm_state;

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


void test_pstreamer_usart_receiver_state_machine_Header_state_SOF_Receiving(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       packet_sm(0x7E);


       // Receiving SOF in the Header state resets State-machine, but keeps state equal to Header
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       // feed state-machine
       packet_sm(0x7E);
       TEST_ASSERT_EQUAL_INT(sm_state, SM_HEADER);
       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&packet, sizeof(struct packet_t));
}

void test_pstreamer_usart_receiver_state_machine_Header_state_receives_too_large_packet_size(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       packet_sm(0x7E);

       //
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0xFA, 0);
       // feed state-machine
       packet_sm(0xFA);

       calc_crc16_ExpectAndReturn(0, 0xAB, 0);
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_IDLE, SM_IDLE);
       // feed state-machine
       packet_sm(0xAB);

       TEST_ASSERT_EQUAL_INT(sm_state, SM_IDLE);
       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&packet, sizeof(struct packet_t));
}

void test_pstreamer_usart_receiver_state_machine_Header_state_to_Payload_transition(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       packet_sm(0x7E);

       //
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       packet_sm(0x00);

       calc_crc16_ExpectAndReturn(0, 0x04, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // feed state-machine
       packet_sm(0x04);

       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(packet.size, 0x0004);
       TEST_ASSERT_EQUAL_INT(sm_state, SM_PAYLOAD);
}


void test_pstreamer_usart_receiver_state_machine_Payload_state_receives_SOF(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       packet_sm(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       packet_sm(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       packet_sm(0x09); // header: 2bytes + payload: 4 bytes  + CRC: 2 bytes + EOF: 1 byte

       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(packet.size, 0x0009);
       TEST_ASSERT_EQUAL_INT(sm_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       packet_sm(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       packet_sm(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       packet_sm(0x33);

       // Error on SOF receiving
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);
       packet_sm(0x7E);
       TEST_ASSERT_EQUAL_INT(sm_state, SM_HEADER);
       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&packet, sizeof(struct packet_t));
}

void test_pstreamer_usart_receiver_state_machine_Payload_state_to_CRC_Tail_transition(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       packet_sm(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       packet_sm(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       packet_sm(0x09); // header: 2bytes + payload: 4 bytes  + CRC: 2 bytes + EOF: 1 byte

       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(packet.size, 0x0009);
       TEST_ASSERT_EQUAL_INT(sm_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       packet_sm(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       packet_sm(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       packet_sm(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0);
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       packet_sm(0x45);

       TEST_ASSERT_EQUAL_INT(packet.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(sm_state, SM_TAIL_CRC);
}
