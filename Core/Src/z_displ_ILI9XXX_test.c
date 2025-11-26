/*
 * z_displ_ILI9488_test.c
 *
 *  Created on: Mar 25, 2022
 *      Author: mauro
 *
 *  This is related to the functions testing features and showing performance
 *  you don't need this file in the production project
 *
 *  licensing: https://github.com/maudeve-it/ILI9XXX-XPT2046-STM32/blob/c097f0e7d569845c1cf98e8d930f2224e427fd54/LICENSE
 *
 *  Do you want to test functions?
 *
 *  STEP 1
 *
 *  on main.c (USER CODE BEGIN 2) put:
 *  Displ_Init(Displ_Orientat_180);			// initialize display controller
 *  Displ_CLS(BLACK);						// clear the screen
 *  Displ_BackLight('I');  					// initialize backlight
 *  Displ_BackLight('1');					// light-up display ad max light level
 *
 *  then, in the main loop put:
 *  Displ_PerfTest();						// shows graphics, measuring time.
 *  and run.
 *
 *  Next STEPs on z_touch_XPT2046_test.c
 *
 *
 */

#include "main.h"

extern int16_t ili9488_width;       			///< (oriented) display width
extern int16_t ili9488_height;      			///< (oriented) display height

void testRects(uint16_t color) {
	int n, i, i2, cx = ili9488_width / 2, cy = ili9488_height / 2;

	ILI9488_FillScreen(BLACK);
	n = min(ili9488_width, ili9488_height);
	for (i = 2; i < n; i += 6) {
		i2 = i / 2;
		ILI9488_DrawBorder(cx - i2, cy - i2, i, i, 1, color);
	}
}

void testFilledRects(uint16_t color1, uint16_t color2) {
	int n, i, i2, cx = ili9488_width / 2 - 1, cy = ili9488_height / 2 - 1;

	ILI9488_FillScreen(BLACK);
	n = min(ili9488_width, ili9488_height);
	for (i = n; i > 0; i -= 6) {
		i2 = i / 2;
		ILI9488_FillArea(cx - i2, cy - i2, i, i, color1);
		ILI9488_DrawBorder(cx - i2, cy - i2, i, i, 1, color2);
	}
}

void testFilledCircles(uint8_t radius, uint16_t color) {
	int x, y, w = ili9488_width, h = ili9488_height, r2 = radius * 2;
	ILI9488_FillScreen(BLACK);
	for (x = radius; x < w; x += r2) {
		for (y = radius; y < h; y += r2) {
			ILI9488_FillCircle(x, y, radius, color);
		}
	}
}

void testCircles(uint8_t radius, uint16_t color) {
	int x, y, r2 = radius * 2, w = ili9488_width + radius, h = ili9488_height
			+ radius;
	// Screen is not cleared for this one -- this is
	// intentional and does not affect the reported time.
	for (x = 0; x < w; x += r2) {
		for (y = 0; y < h; y += r2) {
			ILI9488_DrawCircle(x, y, radius, color);
		}
	}
}

void TestChar() {

	uint16_t x, y, k, a, b;
	uint8_t c;

	for (k = 0; k < 2500; k++) {
		a = rand();
		b = rand();
		x = a % (ili9488_width - 11);
		y = b % (ili9488_height - 18);
		c = a % 26 + 'A';
		ILI9488_WChar(x, y, c, Font16, 1, YELLOW, RED);
		x = b % (ili9488_width - 11);
		y = a % (ili9488_height - 18);
		c = b % 26 + 'A';
		ILI9488_WChar(x, y, c, Font16, 1, RED, YELLOW);
	}
}
;

void wait(uint16_t delay) {
	uint16_t time;
	volatile uint32_t dummy1, dummy2;

	time = HAL_GetTick();
	dummy1 = 0;
	while ((HAL_GetTick() - time) < delay) {
		dummy2 = dummy1;
		dummy1 = dummy2;
	}

}

void TestFillScreen(uint16_t delay) {
	ILI9488_FillScreen(RED);
	if (delay)
		wait(delay);
	ILI9488_FillScreen(GREEN);
	if (delay)
		wait(delay);
	ILI9488_FillScreen(BLUE);
	if (delay)
		wait(delay);
	ILI9488_FillScreen(YELLOW);
	if (delay)
		wait(delay);
	ILI9488_FillScreen(BLACK);
	if (delay)
		wait(delay);
}

void TestHVLine() {
	uint16_t k, x, y, l, a, b;
	ILI9488_FillScreen(BLACK);
	for (k = 0; k < 15000; k++) {
		a = rand();
		b = rand();
		x = a % ili9488_width;
		y = b % ili9488_height;
		l = a % (ili9488_width - x);
		ILI9488_FillArea(x, y, l, 1, BLUE);
		x = b % ili9488_width;
		y = a % ili9488_height;
		l = b % (ili9488_height - y);
		ILI9488_FillArea(x, y, 1, l, CYAN);
	}
}

/* @brief private function for TestDisplay() */
void Displ_Page(char *str1, char *str2, char *str3, uint8_t mode) {
	const uint16_t bcol0 = BLACK, col1 = WHITE, col2 = WHITE, col3 = WHITE,
			bcol1 = BLUE, bcol2 = BLACK, bcol3 = BLACK;
	ILI9488_FillArea(0, 21, ili9488_width, 72, bcol0);
	ILI9488_CString(1, 21, ili9488_width - 1, 21 + 24, str1, Font24, 1, col1,
			bcol1);
	ILI9488_CString(1, 54, ili9488_width - 1, 54 + 24, str2, Font20, 1, col2,
			bcol2);
	ILI9488_CString(1, 77, ili9488_width - 1, 77 + 24, str3, Font20, 1, col3,
			bcol3);
}

void testFillScreen() {
	ILI9488_FillScreen(RED);
	ILI9488_FillScreen(GREEN);
	ILI9488_FillScreen(BLUE);
	ILI9488_FillScreen(YELLOW);
	ILI9488_FillScreen(BLACK);
}

void Displ_ColorTest() {
	const uint8_t colnum = 8;
	const uint8_t rownum = 3;
	const uint16_t colortab[] = { DD_WHITE, RED, BLUE, GREEN, YELLOW, MAGENTA,
			CYAN, WHITE,
			DDD_WHITE, D_RED, D_BLUE, D_GREEN, D_YELLOW, D_MAGENTA, D_CYAN,
			D_WHITE,
			DDDD_WHITE, DD_RED, DD_BLUE, DD_GREEN, DD_YELLOW, DD_MAGENTA,
			DD_CYAN, DD_WHITE };
	static ILI9488_Orientation_e orientation = ILI9488_Orientation_0;
	uint16_t x, y, dx, dy;
	ILI9488_Orientation(orientation);
	if (orientation == ILI9488_Orientation_0)
		orientation = ILI9488_Orientation_90;
	else
		orientation = ILI9488_Orientation_0;
	dx = ili9488_width / colnum;
	dy = ili9488_height / rownum;
	for (y = 0; y < rownum; y++) {
		for (x = 0; x < colnum; x++) {
			ILI9488_FillArea(x * dx, y * dy, dx, dy, colortab[y * colnum + x]);
		}
		__NOP();
	}
	if ((x * dx) < ili9488_width)
		ILI9488_FillArea(x * dx, 0, (ili9488_width - x * dx), ili9488_height,
				BLACK);
	if ((y * dy) < ili9488_height)
		ILI9488_FillArea(0, y * dy, ili9488_width, (ili9488_height - y * dy),
				BLACK);
}

void Displ_TestAll() {
	testFillScreen();
	testRects(GREEN);
	testFilledRects(YELLOW, MAGENTA);
	testFilledCircles(10, MAGENTA);
	testCircles(10, WHITE);
}

void Displ_PerfTest() {
	uint32_t time[6];
	char riga[40];
	uint8_t k;

	for (uint8_t k1 = 0; k1 < 4; k1++) {

		switch (k1) {
		case 0:
			ILI9488_Orientation(ILI9488_Orientation_0);
			break;
		case 1:
			ILI9488_Orientation(ILI9488_Orientation_90);
			break;
		case 2:
			ILI9488_Orientation(ILI9488_Orientation_180);
			break;
		case 3:
			ILI9488_Orientation(ILI9488_Orientation_270);
			break;
		}

		time[1] = HAL_GetTick();
		Displ_TestAll();
		time[1] = HAL_GetTick() - time[1];
		sprintf(riga, "%ld ms", time[1]);
		Displ_Page("TEST 1", "TestAll:", riga, 0);
		HAL_Delay(3000);

		time[2] = HAL_GetTick();
		for (k = 0; k < 10; k++)
			TestFillScreen(0);
		time[2] = HAL_GetTick() - time[2];
		sprintf(riga, "%ld ms", time[2]);
		Displ_Page("TEST 2", "50 screens:", riga, 0);
		HAL_Delay(3000);

		time[3] = HAL_GetTick();
		TestHVLine();
		time[3] = HAL_GetTick() - time[3];
		sprintf(riga, "%ld ms", time[3]);
		Displ_Page("TEST 3", "30k lines:", riga, 0);
		HAL_Delay(3000);

		ILI9488_FillScreen(BLACK);
		time[4] = HAL_GetTick();
		TestChar();
		time[4] = HAL_GetTick() - time[4];
		sprintf(riga, "%ld ms", time[4]);
		Displ_Page("TEST 4", "5000 chars:", riga, 0);
		HAL_Delay(3000);

		ILI9488_FillScreen(BGCOLOR);

		ILI9488_FillArea(0, 0, ili9488_width, ili9488_height, BGCOLOR);

		uint16_t deltah = Font24.Height;
		ILI9488_CString(0, 0, ili9488_width, deltah, "RESULTS", Font24, 1,
				WHITE, BLUE);

		for (uint8_t k = 1; k < 5; k++) {
			switch (k) {
			case 0:
				//				sprintf(riga,"INITIAL SETUP: %ld ms",time[k]);
				break;
			case 1:
				sprintf(riga, "STD TEST   %4ld", time[k]);
				break;
			case 2:
				sprintf(riga, "50 SCREENS %4ld", time[k]);
				break;
			case 3:
				sprintf(riga, "30k LINES  %4ld", time[k]);
				break;
			case 4:
				sprintf(riga, "5k CHARS   %4ld", time[k]);
				break;
			}
			//		Displ_WString(0, 10+deltah+(k)*Font20.Height*2, riga, Font20, 1, WHITE, BGCOLOR);
			ILI9488_CString(0, 10 + deltah + (k) * Font20.Height * 2,
					ili9488_width, 10 + deltah + Font20.Height * ((k) * 2 + 1),
					riga, Font20, 1, WHITE, BGCOLOR);
		};
		HAL_Delay(5000);
	}
}

