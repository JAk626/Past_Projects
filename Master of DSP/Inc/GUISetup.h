/*
 * FFT.h
 *
 *  Created on: 24 Mar 2021
 *      Author: C.F. Wagner
 *
 * Requirments
 *  ADC_BUF_LENG and NUM_SAMPLES must be #define(d) in "main.h".
 */

#ifndef APPLICATION_USER_GUISetup_H_
#define APPLICATION_USER_GUISetup_H_

#include "main.h"
#define NUM_GUI_DISPLAY_BINS 256

extern TEXT_Handle audioD;
extern TEXT_Handle audioFilter;
void Graph_Setup();
void GUI_XY_Graph(float32_t* y_data, int arr_length, uint8_t displayOption, uint8_t channelDisplayOption);
void GUI_GraphChange(uint8_t, int8_t,int8_t);
#endif /* APPLICATION_USER_GUISetup_H_ */
