/*
 * Author: Merlin Webster
 * Description: Recieve program used for interraction between the transceiver and MCU.
 * 		A transmit program is also written, with the aim of falicitating transmission between two transceviers.
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

#define _DELAY_MS(x) SysCtlDelay(x * SysCtlClockGet() / (3*1000))

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

    SSIEnable(SSI0_BASE);

}

int main(void)
{
	setup();

	UART_TX_string("XCVR RX:\n\r");

	cc112x_manualReset(GPIO_PORTA_BASE, GPIO_PIN_6);

    	cc112x_write_regs(1, cc112x_regSettings, sizeof(cc112x_regSettings) / (2 * sizeof(uint16_t)));

	SSIDataPut(SSI0_BASE, CC112X_SRX);
	while(SSIBusy(SSI0_BASE));

	_DELAY_MS(1000);

    while(1)
    {

    	SSIDataPut(SSI0_BASE, (uint8_t)((CC112X_NUM_RXBYTES & 0xFF00) >> 8));
    	SSIDataPut(SSI0_BASE, (uint8_t)(CC112X_NUM_RXBYTES & 0x00FF));
    	while(SSIBusy(SSI0_BASE));

    	uint32_t recieved;
    	while(SSIDataGetNonBlocking(SSI0_BASE, &recieved));

    	SSIDataPut(SSI0_BASE, CC112X_SNOP);
    	while(SSIBusy(SSI0_BASE));
    	SSIDataGet(SSI0_BASE, &recieved);

    	char tmp2[100];
    	sprintf(tmp2, "\r\nbytes in rx fifo: %i\r\n", recieved);
    	UART_TX_string(tmp2);

    	int num_bytes = recieved;
    	int i;
    	for(i = 0; i < num_bytes; i++)
    	{
    		recieved = 0;
    		SSIDataPut(SSI0_BASE, CC112X_SINGLE_RXFIFO);
        	while(SSIDataGetNonBlocking(SSI0_BASE, &recieved));
        	SSIDataPut(SSI0_BASE, CC112X_SNOP);
        	SSIDataGet(SSI0_BASE, &recieved);

        	char tmp[10];
        	sprintf(tmp, "%i\r\n", recieved);
        	UART_TX_string(tmp);
    	}
    }
}
