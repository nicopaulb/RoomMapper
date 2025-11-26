/*
 * z_touch_XPT2046_test.c
 *
 *  Created on: 2 giu 2022
 *      Author: mauro
 *
 *  This is related to the functions testing features and showing performance
 *  you don't need this file in the production project
 *
 *  licensing: https://github.com/maudeve-it/ILI9XXX-XPT2046-STM32/blob/c097f0e7d569845c1cf98e8d930f2224e427fd54/LICENSE
 *
 *  Do you want to test functions?
 *  follow STEP_1 in z_displ_ILI9488_test.c
 *  then:
 *
 *  STEP_2
 *	in the main loop remove or comment previous command and put:
 *	Touch_ShowData();					// shows data returned by XPT2046 controller: try touching everywhere the display
 *
 *  STEP_3
 *	in the main loop remove or comment previous command and put:
 *	Touch_TestCalibration();			// compute and shows parameters to adopt converting XPT2046 data into display coordinates.
 *		 								// compare the (green) data shown with data in z_touch_XPT2046.h and, in case of needs, change it.
 *
 *  STEP_4
 *	in the main loop remove or comment previous command and put:
 *	Touch_TestDrawing();				//move pen on display while touching to check quality of parameters detected on step 3
 *										// repeat test on 4 display orientation
 *
 */
#include <string.h>

#include "main.h"

extern int16_t ili9488_width;       								///< (oriented) display width
extern int16_t ili9488_height;      								///< (oriented) display height




/*************************************************************
 * used by Touch_TestDrawing() and Touch_TestCalibration()
 *************************************************************/
void DrawCross(uint16_t x,uint16_t y,uint16_t fcol){
	uint8_t ray=10;
	ILI9488_FillArea(x-ray, y, 2*ray, 2, fcol);
	ILI9488_FillArea(x, y-ray, 2, 2*ray, fcol);
}


/*************************************************************
 * used by Touch_TestDrawing() and Touch_TestCalibration()
 *************************************************************/
void MoveCross(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t fcol,uint16_t bcol){
	const uint8_t steps=20;
	int16_t deltax,deltay;

	deltax = (x2-x1)/steps;
	deltay = (y2-y1)/steps;

	while ((x1!=x2) || (y1!=y2)) {
		DrawCross(x1,y1,bcol);

		x1=((abs((x2-x1))>abs(deltax)) ? (x1+deltax) : x2);
		y1=((abs((y2-y1))>abs(deltay)) ? (y1+deltay) : y2);

		DrawCross(x1,y1,fcol);
		HAL_Delay(500/steps);
	}
}

void Touch_ShowData(void)
{
	uint16_t x;
	uint16_t y;
	char text[30];
	uint32_t touchTime=0,touchDelay;

	ILI9488_Orientation(ILI9488_Orientation_0);
	ILI9488_FillScreen(WHITE);

	while (1) {

		if (XPT2046_GotATouch())
		{
			touchTime=HAL_GetTick();
			XPT2046_GetTouchPosition(&x, &y);
		}
		else {
			x = 0;
			y = 0;
		}

		touchDelay=(HAL_GetTick() - touchTime);

		if ((touchDelay<100) && (touchTime!=0)) {
			strcpy(text,"PENDOWN");
			ILI9488_WString(10,30,text,Font20,1,RED,YELLOW);
		};
		if (touchDelay>=100) {
			strcpy(text,"       ");
			ILI9488_WString(10,30,text,Font20,1,BLUE,WHITE);
			touchDelay=0;
		};


		sprintf(text,"X=%#X -         ",x);
		ILI9488_WString(10,60,text,Font20,1,BLUE,WHITE);
		sprintf(text,"Y=%#X -         ",y);
		ILI9488_WString(10,80,text,Font20,1,BLUE,WHITE);;
		HAL_Delay(100);
	}

}




/****************************************
 * a test with a continue touch polling,
 * drawing values returned,
 * until touch is released
 ****************************************/
void Touch_TestDrawing() {
	uint16_t px=0,py,npx,npy;

	for (uint8_t k=0;k<4;k++){

		switch (k){
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

		ILI9488_FillScreen(DD_BLUE);
		ILI9488_CString(0,10,ili9488_width,Font12.Height+10,"Touch and drag over display",Font12,1,WHITE,DD_BLUE);

		XPT2046_GotATouch();
		XPT2046_WaitForTouch(0);

		while (1) {

			if (!XPT2046_GetTouchPosition(&npx,&npy))
				break;
			if (px!=0)
				DrawCross(px,py,DD_BLUE);
			DrawCross(npx,npy,WHITE);
			px=npx;
			py=npy;
			HAL_Delay(30);
		}
	}
}

//void Touch_TestCalibration(){
//	uint8_t correct_orientation=0;
//	ILI9488_Orientation_e orientation=ILI9488_Orientation_0;
//
//	const uint16_t shift=80;
//	char text[30];
//	uint16_t x[5];
//	uint16_t y[5];
//	uint32_t read_x[5]={0,0,0,0,0};
//	uint32_t read_y[5]={0,0,0,0,0};
//	uint32_t r_x,r_y,r_z;
//	uint32_t xx,yy;
//	uint8_t k,h;
//	float ax[2];
//	float bx[2];
//	float ay[2];
//	float by[2];
//	float axx,bxx,ayy,byy,e;
////	uint8_t orientation;
//	sFONT font;
//
//	while (! correct_orientation) {
//		ILI9488_FillScreen(WHITE);
//		ILI9488_Orientation(orientation);
//		//setting positions for calibration
//		x[0]=shift;
//		x[1]=ili9488_width-shift;
//		x[2]=shift;
//		x[3]=ili9488_width-shift;
//		x[4]=ili9488_width>>1;
//		y[0]=shift;
//		y[1]=ili9488_height-shift;
//		y[2]=ili9488_height-shift;
//		y[3]=shift;
//		y[4]=ili9488_height>>1;
//		for (uint8_t k=0; k<2; k++) {
//			switch (k) {
//			case 0:
//				strcpy(text,"Press and briefly hold");
//				break;
//			case 1:
//				strcpy(text,"stylus on the target.");
//				break;
//			}
//			ILI9488_CString(0,10+Font12.Height*k,ili9488_width,10+Font12.Height*(1+k),text,Font12,1,BLACK,WHITE);
//
//		}
//		for (uint8_t k=0; k<2; k++) {
//			switch (k) {
//			case 0:
//				strcpy(text,"Repeat as the target");
//				break;
//			case 1:
//				strcpy(text,"moves around the screen.");
//				break;
//			}
//			ILI9488_CString(0,ili9488_height+Font12.Height*(k-2)-10,ili9488_width,ili9488_height+Font12.Height*(k-1)-10,text,Font12,1,BLACK,WHITE);
//
//		}
//
//		HAL_Delay(1000);
//		Touch_WaitForUntouch(0);
//
//		for (h=0;h<5;h++){    // 5 point calibration
//			DrawCross(x[h],y[h],BLACK);
//	// wait for stylus
//			XPT2046_WaitForTouch(0);
//
//	// makes NUM_READINGS touch polling calculating average value
//			k=0;
//			while (k<NUM_READINGS) {
//				r_x=XPT2046_PollAxis(X_AXIS);
//				r_y=XPT2046_PollAxis(Y_AXIS);
//				r_z=XPT2046_PollAxis(Z_AXIS);
//				if ((r_z>Z_THRESHOLD) && (r_x>X_THRESHOLD)) {
//					read_x[h]+=r_x;
//					read_y[h]+=r_y;
//					k++;
//					HAL_Delay(10);
//				}
//
//			}
//			read_x[h]=read_x[h]/NUM_READINGS;
//			read_y[h]=read_y[h]/NUM_READINGS;
//
//			if (h!=4)
//				MoveCross(x[h],y[h],x[h+1],y[h+1],BLACK,WHITE);
//
//			// wait for user removing stylus
//			Touch_WaitForUntouch(0);
//		}
//
//
//		//check il display and touch_sensor orientation are aligned
//
//		correct_orientation=1;
//		correct_orientation &= (read_x[1]>read_x[0]);
//		correct_orientation &= (read_y[1]>read_y[0]);
//		correct_orientation &= (read_x[2]<read_x[1]);
//		correct_orientation &= (read_x[3]>read_x[2]);
//		correct_orientation &= (read_y[3]<read_y[2]);
//		correct_orientation &= (read_x[4]<read_x[3]);
//		correct_orientation &= (read_y[4]>read_y[3]);
//
//		if (! correct_orientation){   //they could be alighen but inverted x axes
//			correct_orientation=1;
//			correct_orientation &= (read_x[1]<read_x[0]);
//			correct_orientation &= (read_y[1]>read_y[0]);
//			correct_orientation &= (read_x[2]>read_x[1]);
//			correct_orientation &= (read_x[3]<read_x[2]);
//			correct_orientation &= (read_y[3]<read_y[2]);
//			correct_orientation &= (read_x[4]>read_x[3]);
//			correct_orientation &= (read_y[4]>read_y[3]);
//		}
//
//
//		if (! correct_orientation){  // if not aligned, rotate display
//			ILI9488_FillScreen(WHITE);
//			ILI9488_CString(0,((ili9488_height>>1)-31),ili9488_width,((ili9488_height>>1)-10),"please",Font20,1,BLUE,WHITE);
//			ILI9488_CString(0,((ili9488_height>>1)-11),ili9488_width,((ili9488_height>>1)+10),"repeat",Font20,1,BLUE,WHITE);
//			ILI9488_CString(0,((ili9488_height>>1)+11),ili9488_width,((ili9488_height>>1)+20),"calibration",Font20,1,BLUE,WHITE);
//			HAL_Delay(2000);
//			switch (orientation) {
//			case ILI9488_Orientation_0:
//				orientation=ILI9488_Orientation_90;
//				break;
//			case ILI9488_Orientation_90:
//				orientation=ILI9488_Orientation_180;
//				break;
//			case ILI9488_Orientation_180:
//				orientation=ILI9488_Orientation_270;
//				break;
//			case ILI9488_Orientation_270:
//				orientation=ILI9488_Orientation_0;
//				break;
//
//			}
//
//		}
//
//	}
//
//
//
//	//calculate linear conversion parameter between point 1 and 2
//
//	ax[0]=(x[0]+0.0f)-x[1];
//	bx[0]=((x[1]+0.0f)*read_x[0])-((x[0]+0.0f)*read_x[1]);
//	e=((read_x[0]+0.0f)-read_x[1]);
//	ax[0]=ax[0]/e;
//	bx[0]=bx[0]/e;
//
//	ay[0]=(y[0]+0.0f)-y[1];
//	by[0]=((y[1]+0.0f)*read_y[0])-((y[0]+0.0f)*read_y[1]);
//	ay[0]=ay[0]/((read_y[0]+0.0f)-read_y[1]);
//	by[0]=by[0]/((read_y[0]+0.0f)-read_y[1]);
//
//	//calculate linear conversion parameter between point 3 and 4
//	ax[1]=(x[2]+0.0f)-x[3];
//	bx[1]=((x[3]+0.0f)*read_x[2])-((x[2]+0.0f)*read_x[3]);
//	e=((read_x[2]+0.0f)-read_x[3]);
//	ax[1]=ax[1]/e;
//	bx[1]=bx[1]/e;
//
//	ay[1]=(y[2]+0.0f)-y[3];
//	by[1]=((y[3]+0.0f)*read_y[2])-((y[2]+0.0f)*read_y[3]);
//	ay[1]=ay[1]/((read_y[2]+0.0f)-read_y[3]);
//	by[1]=by[1]/((read_y[2]+0.0f)-read_y[3]);
//
//
//	// calculate average conversion parameters
//
//	axx = (ax[0] + ax[1])/2;
//	bxx = (bx[0] + bx[1])/2;
//	ayy = (ay[0] + ay[1])/2;
//	byy = (by[0] + by[1])/2;
//
//
//	ILI9488_FillScreen(WHITE);
//
//	font=Font16;
//
//	k=1;
//	sprintf(text,"Current config:");
//	ILI9488_WString(10,10+Font12.Height*k++,text,font,1,BLACK,WHITE);
//	sprintf(text,"Ax=%f Bx=%f",AX,BX);
//	ILI9488_WString(10,10+Font12.Height*k++,text,font,1,BLACK,WHITE);
//	sprintf(text,"Ay=%f By=%f",AY,BY);
//	ILI9488_WString(10,10+Font12.Height*k++,text,font,1,BLACK,WHITE);
//#ifdef  T_ROTATION_0
//	sprintf(text,"Orientation 0");
//#endif
//#ifdef  T_ROTATION_90
//	sprintf(text,"Orientation 90");
//#endif
//#ifdef  T_ROTATION_180
//	sprintf(text,"Orientation 180");
//#endif
//#ifdef  T_ROTATION_270
//	sprintf(text,"Orientation 270");
//#endif
//	ILI9488_WString(10,10+Font12.Height*k++,text,font,1,BLACK,WHITE);
//	k++;
//	sprintf(text,"Current test:");
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,BLACK,WHITE);
//	sprintf(text,"Ax=%f Bx=%f",ax[0],bx[0]);
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,RED,WHITE);
//	sprintf(text,"Ay=%f By=%f",ay[0],by[0]);
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,RED,WHITE);
//	sprintf(text,"Ax=%f Bx=%f",ax[1],bx[1]);
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,BLUE,WHITE);
//	sprintf(text,"Ay=%f By=%f",ay[1],by[1]);
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,BLUE,WHITE);
//	k++;
//	sprintf(text,"Proposed config:");
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,BLACK,WHITE);
//	sprintf(text,"Ax=%f Bx=%f",axx,bxx);
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,DD_GREEN,WHITE);
//	sprintf(text,"Ay=%f By=%f",ayy,byy);
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,DD_GREEN,WHITE);
//	switch (orientation) {
//	case 0:
//		sprintf(text,"Orientation 0");
//		break;
//	case 1:
//		sprintf(text,"Orientation 270");
//		break;
//	case 2:
//		sprintf(text,"Orientation 180");
//		break;
//	case 3:
//		sprintf(text,"Orientation 90");
//	}
//	ILI9488_WString(10,10+font.Height*k++,text,font,1,DD_GREEN,WHITE);
//
//	xx=(ax[0]*read_x[4]+bx[0]);
//	yy=(ay[0]*read_y[4]+by[0]);
//	DrawCross(xx,yy,RED);
//	xx=(ax[1]*read_x[4]+bx[1]);
//	yy=(ay[1]*read_y[4]+by[1]);
//	DrawCross(xx,yy,BLUE);
//	xx=(axx*read_x[4]+bxx);
//	yy=(ayy*read_y[4]+byy);
//	DrawCross(xx,yy,DD_GREEN);
//
//	XPT2046_WaitForTouch(0);
//}

