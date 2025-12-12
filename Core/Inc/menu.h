/*
 * menu.h
 *
 *  Created on: Dec 8, 2025
 *      Author: Nicolas BESNARD
 */

#ifndef INC_MENU_H_
#define INC_MENU_H_

#include <stdint.h>

typedef enum
{
    MENU_SCREEN_MAIN, MENU_SCREEN_MAP, MENU_SCREEN_DIAG
} menu_screen_e;

extern const uint8_t logo[12288];
extern const uint8_t volume[1728];

void MENU_SetScreen(menu_screen_e screen);
void MENU_HandleTouch(void);
void MENU_UpdateScreen(void);


#endif /* INC_MENU_H_ */
