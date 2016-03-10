/*
 * Author: Merlin Webster
 * Description: Slave SPI program used for interraction between two MCUs, with the data being unpacketed and decoded.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/ssi.h"
#include "cc112x_spi.h"
#include "cc112x_reg.h"
#include "cc112x.h"
#include "uart_funcs.h"
#include "api.h"
#include <string.h>

// The error routine that is called if the driver library encounters an error.
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

#define NUM_SPI_DATA 149

void UARTIntHandler(void)
{
    uint32_t ui32Status;

    // Get the interrrupt status.
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART0_BASE, ui32Status);

    /*while(ROM_UARTCharsAvail(UART0_BASE))
    {
        ROM_UARTCharPutNonBlocking(UART0_BASE, ROM_UARTCharGetNonBlocking(UART0_BASE));
        SysCtlDelay(SysCtlClockGet() / (1000 * 3));
    }*/
}

uint8_t SPI_RX_DATA[NUM_SPI_DATA];
volatile unsigned long SPI_RXTO_COUNT = 0;

volatile unsigned int read_put_p = 0;
volatile uint8_t receiving = 0;
volatile uint8_t packet_received = 0;

void SSI0IntHandler(void)
{
	unsigned long status, index;

	// Read interrupt status.
	status = SSIIntStatus(SSI0_BASE, 1);

	// Check the reason for the interrupt.
	if(status & SSI_RXTO)
	{
		// Interrupt is because of RX time out.  So increment counter to tell
		// main loop that RX timeout interrupt occurred.
		SPI_RXTO_COUNT++;

		// Read NUM_SSI_DATA bytes of data from SSI2 RX FIFO.
		uint32_t tmp;
		SSIDataGet(SSI0_BASE, &tmp);
		if(receiving)
			SPI_RX_DATA[read_put_p++] = tmp & 0xff;
		if(read_put_p == 0 && tmp == 0x7E)
			receiving = 1;
		else if(read_put_p == 0)
			return;
		if(read_put_p == NUM_SPI_DATA)
		{
			read_put_p = 0;
			packet_received = 1;
			receiving = 0;
		}
		//char temp[10];
		//sprintf(temp, "%d\r\n", read_put_p);
		//UART_TX_string(temp);
	}

	// Clear interrupts.
	SSIIntClear(SSI0_BASE, status);
}

void setup()
{
	//--------------------- GENERAL ---------------------

    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    ROM_FPUEnable();
    ROM_FPULazyStackingEnable();

    // Set the clocking to run directly from the crystal.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    ROM_IntMasterEnable();

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6);

	//--------------------- UART ---------------------

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);


	//--------------------- SSI ---------------------

    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_SLAVE, 10000, 8);
    //SSIAdvModeSet(SSI0_BASE, SSI_ADV_MODE_READ_WRITE);
    //SSIAdvFrameHoldEnable(SSI0_BASE);

    SSIEnable(SSI0_BASE);
    SSIIntEnable(SSI0_BASE, SSI_RXTO);
    IntEnable(INT_SSI0);

}

int main(void)
{
	setup();

	//slave
	UART_TX_string("slave\n\r");

	//remove anything left in RX FIFO
	uint32_t junk = 1;
	while(SSIDataGetNonBlocking(SSI0_BASE, &junk));
	SSIIntClear(SSI0_BASE, SSI_RXTO);

	raw_data rd;
	rd.length = NUM_SPI_DATA;
	while(1)
	{
		while(packet_received == 0);
		UART_TX_string("packet_received\n\r");

		rd.data = SPI_RX_DATA;
		raw_data decoded;
		unpacket_data(rd, &decoded, 2, 16);
//
		/*int i;
		for(i = 0; i < NUM_SPI_DATA; i++)
		{
			char tmp[10];
			sprintf(tmp, "%i\r\n", SPI_RX_DATA[i]);
			UART_TX_string(tmp);
		}*/

		UART_TX_string(decoded.data);
		UART_TX_string("\n\r");
		free(decoded.data);

		packet_received = 0;
	}

//	while(1)
//	{
//		//Wait for data to be recieved
//		while(SPI_RXTO_COUNT == 0);
//		SPI_RXTO_COUNT = 0;
//
//		char byte = SPI_RX_DATA[0];
//
//		char tmp[10];
//		sprintf(tmp, "%i\r\n", byte);
//		UART_TX_string(tmp);
//
//
////		if(byte == 0x7E)
////		{
////			UART_TX_string("Recieved 0x7E.\r\n");
////			//Wait for data to be recieved
////			while(SPI_RXTO_COUNT == 0);
////			SPI_RXTO_COUNT = 0;
////
////			sprintf(tmp, "%i\r\n", SPI_RX_DATA[0]);
////			UART_TX_string(tmp);
////
////			rd.data = &(SPI_RX_DATA[0]);
////			raw_data decoded;
////			if(!unpacket_data(rd, &decoded, 2, 16))
////				UART_TX_string("Failed to decode!\n\r");
////			else
////				UART_TX_string((char*)decoded.data);
////		}
//	}
	return 0;
}

//
//	UART_TX_string("started\n\r");
//	raw_data rd;
//	rd.length = 64;
//	rd.data = (uint8_t*)malloc(rd.length);
//	char tmp[30];
//	sprintf(tmp, "rd.data = %p\n\r", rd.data);
//	UART_TX_string(tmp);
//	int i;
//	for(i = 0; i < rd.length; i++)
//		rd.data[i] = i;
//	UART_TX_string("initialised\n\r");
//
//	raw_data encoded = packet_data(rd, 2, 16);
//	UART_TX_string("packeted\n\r");
//
//	raw_data decoded;
//	unpacket_data(encoded, &decoded, 2, 16);
//	UART_TX_string("unpacketed\n\r");
//
//	//for(i = 0; i < decoded.length; i++)
//	//	printf("%i\n", decoded.data[i]);
//	if(!memcmp(rd.data, decoded.data, rd.length))
//		UART_TX_string("success!\n\r");
//	else
//		UART_TX_string("failure!\n\r");
//
//	free(encoded.data);
//	free(decoded.data);
//
//	return 0;

//	cc112x_manualReset(GPIO_PORTA_BASE, GPIO_PIN_6);

//	UART_TX_string("Program started:\n\r");
//
//    cc112x_write_regs(1, cc112x_regSettings, sizeof(cc112x_regSettings) / (2 * sizeof(uint16_t)));
//
//	SSIDataPut(SSI0_BASE, CC112X_STX);
//
//	//cc112x_print_status_byte(cc112x_get_status(TX_XCVR));
//	while(SSIBusy(SSI0_BASE));
//
//	//int i = 0;
//    while(1)
//    {
//
//		//ROM_UARTCharPutNonBlocking(UART0_BASE, command);
//    	i = i++ % 255;
//    	SSIDataPut(SSI0_BASE, i);
//    	while(SSIBusy(SSI0_BASE));
//    	cc112x_wait_until_ready(TX_XCVR);
//    	//SysCtlDelay(SysCtlClockGet() / (3*1000));
//
//
//
////    	if(UART_RX_string(strFromUART, 128))
////    		UART_TX_string("\nError: UART RX Overflow!\n");
////    	UART_TX_string(strFromUART);
//    }
//}

