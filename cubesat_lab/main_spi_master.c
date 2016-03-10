/*
 * Author: Merlin Webster
 * Description: Master SPI program used for interraction between two MCUs, with the data being packeted and encoded.
 */

#define _DELAY_MS(x) SysCtlDelay(x * SysCtlClockGet() / (3*1000))

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

void UARTIntHandler(void)
{
    uint32_t ui32Status;

    // Get the interrrupt status.
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART0_BASE, ui32Status);

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

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 10000, 8);
    //SSIAdvModeSet(SSI0_BASE, SSI_ADV_MODE_READ_WRITE);
    //SSIAdvFrameHoldEnable(SSI0_BASE);

    SSIEnable(SSI0_BASE);

}

int main(void)
{
	setup();

	//master
	UART_TX_string("master\n\r");

	raw_data rd;
	rd.length = 64;
	rd.data = malloc(rd.length);
	unsigned int rd_putp = 0;
	while(1)
	{
		char thingy[2];
		char to_send = ROM_UARTCharGet(UART0_BASE);
		thingy[0] = to_send;
		thingy[1] = '\0';
		UART_TX_string(thingy);
		if(to_send == '\r')
		{
			UART_TX_string("\n\r");
			rd.data[rd_putp++] = to_send;
			rd.data[rd_putp] = '\0';

			raw_data packeted = packet_data(rd, 2, 16);
			SSIDataPut(SSI0_BASE, 0x7E);
			while(SSIBusy(SSI0_BASE));
			unsigned int i;
			for(i = 0; i < packeted.length; i++)
			{
				SSIDataPut(SSI0_BASE, packeted.data[i]);
				while(SSIBusy(SSI0_BASE));
			}
			free(packeted.data);
			rd_putp = 0;
		}
		else
		{
			rd.data[rd_putp++] = to_send;
		}
	}
	free(rd.data);
	return 0;