#ifndef __UART_H__
#define __UART_H__

#include "defs.h"

void uartInit();
BOOL uartReadyToRead();
UINT8 uartReadByte();
BOOL uartReadyToWrite();
void uartWriteByte(const UINT8 byte);

#endif
