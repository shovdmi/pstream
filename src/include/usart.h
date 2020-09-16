#ifndef USART_H
#define USART_H

int usart_transmit(char data);
char usart_read(void);

unsigned int usart_status(unsigned int status_bit);

#endif
