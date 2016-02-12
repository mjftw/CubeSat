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

    /*while(ROM_UARTCharsAvail(UART0_BASE))
    {
        ROM_UARTCharPutNonBlocking(UART0_BASE, ROM_UARTCharGetNonBlocking(UART0_BASE));
        SysCtlDelay(SysCtlClockGet() / (1000 * 3));
    }*/
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
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

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

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
    //SSIAdvModeSet(SSI0_BASE, SSI_ADV_MODE_READ_WRITE);
    //SSIAdvFrameHoldEnable(SSI0_BASE);

    SSIEnable(SSI0_BASE);

}

int main(void)
{
	setup();

	cc112x_manualReset(GPIO_PORTA_BASE, GPIO_PIN_6);

	UART_TX_string("Program started:\n\r");

    cc112x_write_regs(1, cc112x_regSettings, sizeof(cc112x_regSettings) / (2 * sizeof(uint16_t)));

	SSIDataPut(SSI0_BASE, CC112X_STX);

	//cc112x_print_status_byte(cc112x_get_status(TX_XCVR));
	while(SSIBusy(SSI0_BASE));

	int i = 0;
    while(1)
    {

		//ROM_UARTCharPutNonBlocking(UART0_BASE, command);
    	i = i++ % 255;
    	SSIDataPut(SSI0_BASE, i);
    	while(SSIBusy(SSI0_BASE));
    	//cc112x_wait_until_ready(TX_XCVR);


    	cc112x_print_status_byte(cc112x_get_status(TX_XCVR));
    	UART_TX_string("\n\r");
    	//SysCtlDelay(SysCtlClockGet() / (3*1000)); //approx 1ms delay

    }
}
