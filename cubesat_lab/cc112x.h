/*
 * Author: Merlin Webster
 * Description: Functions prototypes for interraction with the transceviers
 */

#ifndef CC112X_H_
#define CC112X_H_

#include <stdbool.h>
#include <stdint.h>

void cc112x_print_status_byte(const uint8_t statusByte);
void cc112x_wait_until_ready(const int xcvr);
uint8_t cc112x_get_status(const int xcvr);
static uint8_t cc112x_sgl_reg_access(const int xcvr, const bool rw, const uint16_t addr, const uint16_t data);
uint8_t cc112x_write_regs(const int xcvr, const uint16_t const regSettings[][2], const int arrLen);
void cc112x_manualReset(uint32_t base, uint32_t pin);

#endif /* CC112X_H_ */
