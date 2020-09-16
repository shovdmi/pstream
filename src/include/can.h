#ifndef CAN_H
#define CAN_H

#define TP_DT (0x00EB00)
#define TP_CM (0x00EC00)


int transmit_P2P_packet(uint8_t *data, size_t size);
int transmit_TP_CM_packet(size_t packets_total, size_t size);
int transmit_TP_DT_packet(size_t packet_num, uint8_t *data, size_t size);

#endif // CAN_H
