#ifndef __UART_H__
#define __UART_H__

#include "defs.h"
/*
 * Initilize the Uart
 */
void uartInit();
/*
 * Return "TURE" (in defs.h) if the bit in LSR for dataReady, is on.
 * retun "FALSE" otherwise.
 */
BOOL uartReadyToRead();
/*
 * Return the byte that stores in the RBR register.
 */
UINT8 uartReadByte();
/*
 * Return "TURE" (in defs.h) if the bit in LSR for thrIsEmpty, is on.
 * retun "FALSE" otherwise.
 */
BOOL uartReadyToWrite();
/*
 * Writes the byte to the THR register.
 */
void uartWriteByte(const UINT8 byte);

#endif
