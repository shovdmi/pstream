#ifndef CAN_H
#define CAN_H

#define TP_DT (0x00EB00)
#define TP_CM (0x00EC00)
#define P2P_PGN (0x00AA00)

#define PACKAGE_RECEIVED (1 << 0)

// 250_000uS
#define BAM_TIMEOUT (250000)


uint32_t can_status(uint32_t bit_status);

int transmit_P2P_package(uint8_t *data, size_t size);
int transmit_TP_CM_package(size_t packets_total, size_t size);
int transmit_TP_DT_package(size_t packet_num, uint8_t *data, size_t size);

#endif // CAN_H
