/*
 * cc112x.h
 *
 *  Created on: 3 Dec 2015
 *      Author: Merlin
 */

#ifndef CC112X_H_
#define CC112X_H_

#include "driverlib/ssi.h"
#include "cc112x_spi.h"
#include "uart_funcs.h"

enum{TX_XCVR, RX_XCVR};
enum{REG_WRITE = 0, REG_READ = 1};

#define XCVR_READY(x) !((x & 0x80) >> 7)

static void cc112x_print_status_byte(const uint8_t statusByte)
{
	switch(statusByte >> 4)
	{
	case CC112X_STATE_IDLE >> 4:
		UART_TX_string("CC112X_STATE_IDLE");
		break;
	case CC112X_STATE_RX >> 4:
		UART_TX_string("CC112X_STATE_RX");
		break;
	case CC112X_STATE_TX >> 4:
		UART_TX_string("CC112X_STATE_TX");
		break;
	case CC112X_STATE_FSTXON >> 4:
		UART_TX_string("CC112X_STATE_FSTXON");
		break;
	case CC112X_STATE_CALIBRATE >> 4:
		UART_TX_string("CC112X_STATE_CALIBRATE");
		break;
	case CC112X_STATE_SETTLING >> 4:
		UART_TX_string("CC112X_STATE_SETTLING");
		break;
	case CC112X_STATE_RXFIFO_ERROR >> 4:
		UART_TX_string("CC112X_STATE_RXFIFO_ERROR");
		break;
	case CC112X_STATE_TXFIFO_ERROR >> 4:
		UART_TX_string("CC112X_STATE_TXFIFO_ERROR");
		break;
	}
}

void cc112x_wait_until_ready(const int xcvr)
{
	uint32_t status = 0x80;

//	do
//	{
//		SSIDataPut(SSI0_BASE, CC112X_SNOP);
//		while(SSIBusy(SSI0_BASE));
//		SSIDataGet(SSI0_BASE, (uint32_t *)&status);
//		if(status >> 4 == CC112X_STATE_IDLE >> 4)
//		{
//			SSIDataPut(SSI0_BASE, CC112X_STX);
//			while(SSIBusy(SSI0_BASE));
//			SSIDataGet(SSI0_BASE, (uint32_t *)&status);
//		}
//	}while(!XCVR_READY(status));

	SysCtlDelay(SysCtlClockGet() / (3*1000));
}

uint32_t cc112x_get_status(const int xcvr)
{
	uint32_t status = 0;

	SSIDataPut(SSI0_BASE, CC112X_SNOP);
	while(SSIBusy(SSI0_BASE));
	SSIDataGet(SSI0_BASE, &status);

	return status;
}

static uint8_t cc112x_sgl_reg_access(const int xcvr, const bool rw, const uint16_t addr, const uint16_t data)
{
	uint8_t status = 0;

	SSIDataPut(SSI0_BASE, CC112X_SINGLE_REG_ACC(rw, addr));
	while(SSIBusy(SSI0_BASE));
	SSIDataGet(SSI0_BASE, (uint32_t *)&status);
	cc112x_wait_until_ready(xcvr);

	SSIDataPut(SSI0_BASE, data);
	while(SSIBusy(SSI0_BASE));
	SSIDataGet(SSI0_BASE, (uint32_t *)&status);
	cc112x_wait_until_ready(xcvr);

	return status;
}


uint8_t cc112x_write_regs(const int xcvr, const uint16_t regSettings[][2], const int arrLen)
{
	SSIDataPut(SSI0_BASE, CC112X_SRES);

    while(SSIBusy(SSI0_BASE));
//	SysCtlDelay(SysCtlClockGet() / (3*1000));
	cc112x_wait_until_ready(xcvr);

	int i;
	for(i = 0; i < arrLen; i++)
	{
		//cc112x_print_status_byte(cc112x_sgl_reg_access(xcvr, REG_WRITE, regSettings[i][0], regSettings[i][1]));
		cc112x_sgl_reg_access(xcvr, REG_WRITE, regSettings[i][0], regSettings[i][1]);
		//UART_TX_string("\n\r");

	}

	SysCtlDelay(SysCtlClockGet() / (3*100));
	return 0;
}

void cc112x_manualReset(uint32_t base, uint32_t pin)
{
	GPIOPinWrite(base, pin, 0);
	SysCtlDelay(SysCtlClockGet() / (3*1000));

	GPIOPinWrite(base, pin, pin);
	SysCtlDelay(SysCtlClockGet() / (3*1000));
}


#endif /* CC112X_H_ */
