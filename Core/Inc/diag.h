/*
 * menu.h
 *
 *  Created on: Dec 8, 2025
 *      Author: Nicolas BESNARD
 */

#ifndef INC_DIAG_H_
#define INC_DIAG_H_

#include <stdint.h>

extern const uint8_t cross[1728];

void DIAG_Show(void);
void DIAG_Touch(uint16_t x, uint16_t y);

#endif /* INC_DIAG_H_ */
