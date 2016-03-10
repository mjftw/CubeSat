/*
 * Author: Merlin Webster
 * Description: Functions for sending and recieving strings via UART. 
 */

#include "uart_funcs.h"

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"

void UART_TX_string(const char* buffer)
{
	int max_len = 512;
	int i = 0;
	for(i = 0; (*(buffer + i) != '\0') && (i < max_len); i++)
	{
				ROM_UARTCharPutNonBlocking(UART0_BASE, *((uint8_t *)buffer + i));
		    	SysCtlDelay(SysCtlClockGet() / (3*1000));
	}
}

void UART_RX_nBytes(uint8_t RXBuffer[], const unsigned int nBytes)
{
	int byteNo = 0;
	while(byteNo < nBytes)
	{
		while(ROM_UARTCharsAvail(UART0_BASE) && (byteNo < nBytes))
		{
			RXBuffer[byteNo++] = ROM_UARTCharGetNonBlocking(UART0_BASE);

			ROM_UARTCharPutNonBlocking(UART0_BASE, RXBuffer[byteNo-1]);

			SysCtlDelay(SysCtlClockGet() / (1000 * 3));
		}
	}
}

bool UART_RX_string(char* RXBuffer, const unsigned int maxLen) //Not currently working for some reason. Changes to RXBuffer appear to be only local.
{
	int bufPos = 0;
	char rxChar = '0';

	do{
		if(ROM_UARTCharsAvail(UART0_BASE))
		{
			rxChar = ROM_UARTCharGetNonBlocking(UART0_BASE);
			RXBuffer[bufPos++] = rxChar;

			SysCtlDelay(SysCtlClockGet() / (1000 * 3));
		}
	}while((rxChar != '\r') && (bufPos <= maxLen));

	if((bufPos > maxLen))
		return 1;
	else
	{
		RXBuffer[bufPos] = '\0';
		return 0;
	}
}
