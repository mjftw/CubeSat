/*
 * Author: Merlin Webster
 * Description: Function prototypes for sending and recieving strings via UART. 
 */
#ifndef UART_FUNCS_H_
#define UART_FUNCS_H_

#include <stdbool.h>
#include <stdint.h>

void UART_TX_string(const char* buffer);
void UART_RX_nBytes(uint8_t RXBuffer[], const unsigned int nBytes);
bool UART_RX_string(char* RXBuffer, const unsigned int maxLen); //Not currently working for some reason. Changes to RXBuffer appear to be only local.

#endif /* UART_FUNCS_H_ */
