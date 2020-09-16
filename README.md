# pstream
pstream is a Packet Stream driver for UART and CAN communication

[TOC]

## pstream over USART

pstream uses HDLC-like protocol for USART communication. 

Except for the SOF and EOF every byte is bit-stuffed by zero-inserting (TODO).

Header consists of size of the rest of packet (including final 0x7E (EOF)).
Tail consists of 2 bytes CRC16 and 1 byte EOF.

![packet format](pstream_usart_format.png?raw=true "PStream over USART packet format")

### USART Receiver State-machine

## pstream over CAN-bus

pstream uses J1939 BAM protocol to send/receive packets which have length more than 7 bytes.
For short packages it uses regular pear-to-pear CAN-packege.

### CAN-bus Receiver State-machine
![state machine](index.png?raw=true "CAN-bus Receiver State-machine")
