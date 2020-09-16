#include "unity.h"

#include "mock_mutex.h"
#include "mock_usart.h"
#include "mock_tsrb_ext.h"
#include "mock_crc.h"
#include "mock_message.h"
#include "mock_sm.h"
#include "pstreamer.h"
#include "pstreamer_usart.h"


void setUp(void)
{
}

void tearDown(void)
{
}

extern struct parser_t parser;
extern enum parser_state_t parser_state;

/* -----------------------------------------------------------------------------
 *
 *          Tests of wrapping data sending (over USART)
 *
 * ---------------------------------------------------------------------------*/

void test_pstreamer_send_over_uart_on_no_errors(void)
{

	uint8_t test_data[] = { 0, 1, 2, 3, 4, 5, 6, 7};

	mutex_lock_ExpectAnyArgsAndReturn(0);

	// Transmit SOF
	usart_transmit_ExpectAndReturn(0x7E, TRANSMIT_OK);

	// Transmit Header (size of packet) : 2 bytes to be transmitted
	uint8_t val = sizeof(test_data) & 0xFF;
	calc_crc16_ExpectAndReturn(0, val, 0);
	usart_transmit_ExpectAndReturn(val, TRANSMIT_OK);

	val = (sizeof(test_data) >> 8) & 0xFF;
	calc_crc16_ExpectAndReturn(0, val, 0);
	usart_transmit_ExpectAndReturn(val, TRANSMIT_OK);

	// Transit Payload
	for (int i = 0; i < sizeof(test_data) ;i++)
	{
		calc_crc16_ExpectAndReturn(0, test_data[i], 0);
		usart_transmit_ExpectAndReturn(test_data[i], TRANSMIT_OK);
	}

	// Transmit Tail (crc of packet) : 2 bytes to be transmitted
	usart_transmit_ExpectAndReturn(0, TRANSMIT_OK);
	usart_transmit_ExpectAndReturn(0, TRANSMIT_OK);

	// Transmit EOF
	usart_transmit_ExpectAndReturn(0x7E, TRANSMIT_OK);

	mutex_unlock_ExpectAnyArgsAndReturn(0);
	

	int result = send_data(&test_data[0], sizeof(test_data));
	TEST_ASSERT_EQUAL_INT(TRANSMIT_OK, result);
}

void test_pstreamer_send_over_uart_if_Out_Of_Memory_error_happend(void)
{

	uint8_t test_data[16];

	mutex_lock_ExpectAnyArgsAndReturn(0);

	// Transmit SOF
	usart_transmit_ExpectAndReturn(0x7E, TRANSMIT_OK);

	// Transmit Header (size of packet) : 2 bytes to be transmitted
	uint8_t val = sizeof(test_data) & 0xFF;
	calc_crc16_ExpectAndReturn(0, val, 0);
	usart_transmit_ExpectAndReturn(val, TRANSMIT_OK);

	val = (sizeof(test_data) >> 8) & 0xFF;
	calc_crc16_ExpectAndReturn(0, val, 0);
	usart_transmit_ExpectAndReturn(val, TRANSMIT_OK);


	// Transit Payload
	for (int i = 0; i < sizeof(test_data) ;i++)
	{
		test_data[i] = i;
		calc_crc16_ExpectAndReturn(0, test_data[i], 0);
		usart_transmit_ExpectAndReturn(test_data[i], TRANSMIT_OK);
	}

	// Transmit Tail (crc of packet) : 2 bytes to be transmitted
	usart_transmit_ExpectAndReturn(0, TRANSMIT_OK);
	usart_transmit_ExpectAndReturn(0, TRANSMIT_OK);

	// Transmit EOF
	usart_transmit_ExpectAndReturn(0x7E, OUT_OF_MEMORY);

	mutex_unlock_ExpectAnyArgsAndReturn(0);


	int result = send_data(&test_data[0], sizeof(test_data));
	TEST_ASSERT_EQUAL_INT(OUT_OF_MEMORY, result);
}

/* -----------------------------------------------------------------------------
 *
 *          Tests of wrapping data receiving (over USART)
 *
 * ---------------------------------------------------------------------------*/

//
// Tests of SM_HEADER state
//
void test_pstreamer_usart_receiver_state_machine_Header_state_SOF_Receiving(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);


       // Receiving SOF in the Header state resets State-machine, but keeps state equal to Header
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       // feed state-machine
       parser_feed(0x7E);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_HEADER);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}

void test_pstreamer_usart_receiver_state_machine_Header_state_receives_too_large_packet_size(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       //
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0xFA, 0);
       // feed state-machine
       parser_feed(0xFA);

       calc_crc16_ExpectAndReturn(0, 0xAB, 0);
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_IDLE, SM_IDLE);
       // feed state-machine
       parser_feed(0xAB);

       TEST_ASSERT_EQUAL_INT(parser_state, SM_IDLE);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}

void test_pstreamer_usart_receiver_state_machine_Header_state_to_Payload_transition(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       //
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x04, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // feed state-machine
       parser_feed(0x04);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0004);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);
}


//
// Tests of SM_PAYLOAD state
//
void test_pstreamer_usart_receiver_state_machine_Payload_state_receives_SOF(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // [header:2bytes] + [payload:4 bytes]  + [CRC:2 bytes] + [EOF:1 byte]

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       // Error on SOF receiving
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);
       parser_feed(0x7E);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_HEADER);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}

void test_pstreamer_usart_receiver_state_machine_Payload_state_to_CRC_Tail_transition(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // [header:2bytes] + [payload:4 bytes]  + [CRC:2 bytes] + [EOF:1 byte]

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0);
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       parser_feed(0x45);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_CRC);
}

//
// Tests of SM_TAIL_CRC state
//
void test_pstreamer_usart_receiver_state_machine_Tail_CRC_state_on_Wrong_CRC(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // [header:2bytes] + [payload:4 bytes]  + [CRC:2 bytes] + [EOF:1 byte]

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0x1122); // Here we set parser.crc to 0x1122
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       parser_feed(0x45);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_CRC);

       //
       // feed state-machine at TAIL_CRC state
       //
       parser_feed(0xAB);

       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_IDLE, SM_IDLE);
       parser_feed(0xCD);

       TEST_ASSERT_EQUAL_INT(parser_state, SM_IDLE);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}

void test_pstreamer_usart_receiver_state_machine_Tail_CRC_state_receives_SOF(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // [header:2bytes] + [payload:4 bytes]  + [CRC:2 bytes] + [EOF:1 byte]

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0x1122); // Here we set parser.crc to 0x1122
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       parser_feed(0x45);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_CRC);

       //
       // feed state-machine at TAIL_CRC state
       //
       // Error on SOF receiving
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       TEST_ASSERT_EQUAL_INT(parser_state, SM_HEADER);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}

void test_pstreamer_usart_receiver_state_machine_Tail_CRC_state_to_Tail_EOF_transition(void)
{
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // [header:2bytes] + [payload:4 bytes]  + [CRC:2 bytes] + [EOF:1 byte]

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0x1122); // Here we set parser.crc to 0x1122
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       parser_feed(0x45);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_CRC);

       //
       // feed state-machine at TAIL_CRC state
       //
       parser_feed(0x11);

       sm_change_state_ExpectAndReturn(SM_TAIL_EOF, SM_TAIL_EOF);
       parser_feed(0x22);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 8);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_EOF);
}

//
// Tests of SM_TAIL_EOF state
//
void test_Tail_EOF_state_receives_not_EOF(void)
{
       tsrb_reject_IgnoreAndReturn(0);
       parser_reset();
			 parser_state = SM_IDLE;
       tsrb_reject_StopIgnore();

       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // [header:2bytes] + [payload:4 bytes]  + [CRC:2 bytes] + [EOF:1 byte]

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0x1122); // Here we set parser.crc to 0x1122
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       parser_feed(0x45);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_CRC);

       //
       // feed state-machine at TAIL_CRC state
       //
       parser_feed(0x11);

       sm_change_state_ExpectAndReturn(SM_TAIL_EOF, SM_TAIL_EOF);
       parser_feed(0x22);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 8);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_EOF);

       //
       // feed state-machine at TAIL_EOF state
       //
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_IDLE, SM_IDLE);

       parser_feed(0x11);

       TEST_ASSERT_EQUAL_INT(parser_state, SM_IDLE);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}

void test_pstreamer_usart_receiver_state_machine_Tail_EOF_state_receives_EOF(void)
{
       tsrb_reject_IgnoreAndReturn(0);
       parser_reset();
       parser_state = SM_IDLE;
       tsrb_reject_StopIgnore();


       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_HEADER, SM_HEADER);

       parser_feed(0x7E);

       // feed state-machine at Header state
       // 2 bytes of size
       //
       calc_crc16_ExpectAndReturn(0, 0, 0);
       // feed state-machine
       parser_feed(0x00);

       calc_crc16_ExpectAndReturn(0, 0x09, 0);
       sm_change_state_ExpectAndReturn(SM_PAYLOAD, SM_PAYLOAD);
       // 
       parser_feed(0x09); // header: 2bytes + payload: 4 bytes  + CRC: 2 bytes + EOF: 1 byte

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 2);
       TEST_ASSERT_EQUAL_INT(parser.packet_size, 0x0009);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_PAYLOAD);



       //
       // feed state-machine at Payload state
       //
       calc_crc16_ExpectAndReturn(0, 0x11, 0);
       tsrb_add_tmp_ExpectAndReturn(0x11, 0);
       parser_feed(0x11);

       calc_crc16_ExpectAndReturn(0, 0x22, 0);
       tsrb_add_tmp_ExpectAndReturn(0x22, 0);
       parser_feed(0x22);

       calc_crc16_ExpectAndReturn(0, 0x33, 0);
       tsrb_add_tmp_ExpectAndReturn(0x33, 0);
       parser_feed(0x33);

       calc_crc16_ExpectAndReturn(0, 0x45, 0x1122); // Here we set parser.crc to 0x1122
       tsrb_add_tmp_ExpectAndReturn(0x45, 0);
       sm_change_state_ExpectAndReturn(SM_TAIL_CRC, SM_TAIL_CRC);
       parser_feed(0x45);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 6);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_CRC);

       //
       // feed state-machine at TAIL_CRC state
       // crc value is 0x1122 as was in the header
       //
       parser_feed(0x11);

       sm_change_state_ExpectAndReturn(SM_TAIL_EOF, SM_TAIL_EOF);
       parser_feed(0x22);

       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 8);
       TEST_ASSERT_EQUAL_INT(parser_state, SM_TAIL_EOF);

       //
       // feed state-machine at TAIL_EOF state
       //
       tsrb_commit_ExpectAndReturn(0);
       send_msg_ExpectAndReturn(4,0);
       tsrb_reject_ExpectAndReturn(0);
       sm_change_state_ExpectAndReturn(SM_IDLE, SM_IDLE);

       parser_feed(0x7E);

       TEST_ASSERT_EQUAL_INT(parser_state, SM_IDLE);
       TEST_ASSERT_EQUAL_INT(parser.bytes_received, 0);
       TEST_ASSERT_EACH_EQUAL_INT32(0, (uint8_t*)&parser, sizeof(struct parser_t));
}
