#ifndef PSTREAMER_H
#define PSTREAMER_H

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif


#define TRANSMIT_OK      (0)
#define OUT_OF_MEMORY    (-2)
#define CONNECTION_ERROR (-3)


#ifdef PSTREAMER_OVER_CAN
// 255 chunks * 7 bytes per chunk
#  define PACKET_MAX_SIZE (255 * 7)
#else
#  define PACKET_MAX_SIZE (1024)
#endif



int send_data(uint8_t *data, size_t size);

#endif //PSTREAMER_H
