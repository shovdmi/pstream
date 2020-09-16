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

When pstream uses CAN-bus for communication there is no need of sending CRC explicitly as CAN-bus itseft do this on hardware level.


For short packages pstream uses regular pear-to-pear CAN-package. The first data-byte in payload contains the size of payload (0 -- 7 bytes).


If a packet size is bigger than 7 bytes pstream uses J1939 BAM protocol.
TP_CM package signals Start of Frame and contain number of packages and bytes to be received.
TP_DT packages contain sequence number in the first data-byte and payload in the rest 7 bytes.


<img src="BAM0.jpg" width=350>
<img src="BAM1.jpg" width=350><img src="sae-j1939-21-transport-protocol-broadcast-data-transfer.jpg" width=150>


### CAN-bus Receiver State-machine
![state machine](index.png?raw=true "CAN-bus Receiver State-machine")
