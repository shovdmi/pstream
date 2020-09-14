#ifndef PSTREAMER_H
#define PSTREAMER_H

#define TRANSMIT_OK      (0)
#define OUT_OF_MEMORY    (-2)
#define CONNECTION_ERROR (-3)

int transmit_data(uint8_t *data, size_t size);
int send_over_uart(uint8_t *data, size_t size);

#endif //PSTREAMER_H
