#include "GUISetup.h"
#include <stdio.h>
#include <string.h>

GUI_POINT _aPoints[NUM_GUI_DISPLAY_BINS];
GRAPH_DATA_Handle _hDataXY;
GRAPH_DATA_Handle _hDataYT;
TEXT_Handle freqText;
TEXT_Handle showChannel;
TEXT_Handle showChannel2;
TEXT_Handle ampltudeText;
TEXT_Handle xLabel;
TEXT_Handle yLabel;
TEXT_Handle audioD;
TEXT_Handle audioFilter;
float32_t highest;
int value;
int value2;
char result[20];
char result2[20];
char channelShowContent[20];
char channelShowFiltered[20];
GRAPH_SCALE_Handle hScale;
GRAPH_SCALE_Handle hScale2;
WM_HWIN hGraph;
char dbText[] = "Magnitude (db)";
char vText[] = "Magnitude (V)";
char d1[] = "Unfiltered Frequency Domain";
char d2[] = "Unfiltered Time Domain";
char d3[] = "Filtered Frequency Domain";
char d4[] = "Filtered Time Domain";
GUI_RECT Rect = { 0, 1, 15, 238 };
GUI_RECT Rect2 = { 305, 1, 320, 238 };
WM_HWIN hItem;
float scaleXF;

void Graph_Setup() {
	scaleXF = SAMPLE_FREQ / ((float) NUM_SAMPLES);

	// GUI setup the x axis pixels for both freq and time domain
	for (int i = 0; i < NUM_GUI_DISPLAY_BINS; i++) {
		_aPoints[i].x = i;
	}

	//MULTIBUF is needed to ensure the screen is painted quickly, and fits within the 180MHz tick.
	WM_MULTIBUF_Enable(1);

	// Graph placement remains the same.
	//The graph box is created and the parent is set as the window WIDGET.
	hGraph = GRAPH_CreateEx(15, 0, LCD_GetXSize() - 29, LCD_GetYSize(),
	WM_HBKWIN, WM_CF_HIDE, 0, GUI_ID_GRAPH0);
	//The graph grid is 30 pixels large (x axis).
	GRAPH_SetGridDistX(hGraph, 32);
	//The graph grid is 20 pixels large (x axis).
	GRAPH_SetGridDistY(hGraph, 20);
	//The grid starts at 0
	GRAPH_SetGridOffY(hGraph, 0);
	//Enable grid
	GRAPH_SetGridVis(hGraph, 1);

	//Border around the graph to hold labels.
	GRAPH_SetBorder(hGraph, 27, 10, 5, 30);
	//Color of the border.
	GRAPH_SetColor(hGraph, GUI_DARKGRAY, GRAPH_CI_BORDER);

	//Scale for the dB/ Volt axis
	hScale = GRAPH_SCALE_Create(12, GUI_TA_HCENTER | GUI_TA_VCENTER,
	GRAPH_SCALE_CF_VERTICAL, 1);
	GRAPH_AttachScale(hGraph, hScale);
	//Scale for the kHz axis
	hScale2 = GRAPH_SCALE_Create(220, GUI_TA_VCENTER | GUI_TA_HCENTER,
	GRAPH_SCALE_CF_HORIZONTAL, 1);
	GRAPH_AttachScale(hGraph, hScale2);
	// Create TEXT labels for the dB and KHz scale
	yLabel = TEXT_CreateEx(136, 223, 80, 20, WM_HBKWIN, WM_CF_SHOW,
	TEXT_CF_VCENTER, GUI_ID_TEXT0, "kHz");
	showChannel = TEXT_CreateEx(30, 223, 80, 20, WM_HBKWIN, WM_CF_SHOW,
	TEXT_CF_VCENTER, GUI_ID_TEXT8, "Channel: ");
	showChannel2 = TEXT_CreateEx(240, 223, 80, 20, WM_HBKWIN, WM_CF_SHOW,
	TEXT_CF_VCENTER, GUI_ID_TEXT8, "");
	//Execute all the GUI instructions before moving on to the next instruction in main.
	//Enter into the GUI initializer for the graph.
	freqText = TEXT_CreateEx(250, 223, 100, 20, WM_HBKWIN, WM_CF_SHOW,
	TEXT_CF_VCENTER, GUI_ID_TEXT2, "");
	audioD = TEXT_CreateEx(115, 120, 90, 20, WM_HBKWIN, WM_CF_HIDE,
	TEXT_CF_VCENTER, GUI_ID_TEXT9, "Outputting audio.");
	audioFilter = TEXT_CreateEx(120, 160, 80, 20, WM_HBKWIN, WM_CF_HIDE,
	TEXT_CF_VCENTER, GUI_ID_TEXT6, "");
	ampltudeText = TEXT_CreateEx(20, 223, 100, 20, WM_HBKWIN, WM_CF_SHOW,
	TEXT_CF_VCENTER, GUI_ID_TEXT2, "");
	_hDataXY = GRAPH_DATA_XY_Create(GUI_GREEN, NUM_SAMPLES, _aPoints,
			GUI_COUNTOF(_aPoints));
	GUI_SetColor(GUI_DARKGRAY);
	GUI_SetTextMode(GUI_TM_XOR);
	TEXT_SetText(audioFilter, "Filtering: 1");
	GUI_Exec();
}

//Display the XY data in the frequency domain.
void GUI_XY_Graph(float32_t *y_data, int arr_length, uint8_t displayOption,
		uint8_t channelDisplayOption) {
	highest = 0;
	value2 = highest;
	// Remove the following if it works every time
	// Check array length
	if (arr_length > GUI_COUNTOF(_aPoints)) {
		while (1)
			;
	}

	if (displayOption == 2) {
		// Frequency domain - Unfiltered
		// Input values: 0 to -100 dB
		// Expected values from 0 to -200, so the dB value must be multiplied by 2 before calling this function.
		// The actual values received (0 to -200) will be converted, resulting in 200 to 0.

		// Offset y_data to center around the selected display channel.
		y_data += 162 + (uint32_t) ((4.9152) * (channelDisplayOption - 1));

		for (int i = 0; i < arr_length; i += 1) {
			_aPoints[i].y = ((I16P) y_data[i]) + 200;

//			//Find highest point
//			if (y_data[i] > highest && i > 3) {
//				value = _aPoints[i].x * scaleXF;
//				highest = y_data[i];
//			}
		}
	} else {
		y_data += (290*FFT_FILTERED_ZOOM - 128) + (uint32_t) ((4.9152 * FFT_FILTERED_ZOOM) * (channelDisplayOption - 1));

		for (int i = 0; i < arr_length; i += 1) {
			_aPoints[i].y = ((I16P) y_data[i]) + 200;

//			//Find highest point
//			if (y_data[i] > highest && i > 3) {
//				value = _aPoints[i].x * scaleXF;
//				highest = y_data[i];
//			}
		}
	}
	// Convert value2 to dB
	value2 = (int) highest;

	// Ignore noise
	if (value2 < -54) {
		value = 0;
		value2 = 0;
	}
	sprintf(result, "%i Hz", value);
	sprintf(result2, "%i dB", value2);

	// Get the GRAPH handle
	hItem = WM_GetFirstChild(WM_HBKWIN);

	// Detach data handle and delete it to ensure the same
	// variable is used and the memory is saved.
	GRAPH_DetachData(hItem, _hDataXY);
	GRAPH_DATA_XY_Delete(_hDataXY);

	// Create a new XY data object array for the graph.
	_hDataXY = GRAPH_DATA_XY_Create(GUI_GREEN, arr_length, _aPoints,
			GUI_COUNTOF(_aPoints));
	//Attach the data to the graph (prime it).
	GRAPH_AttachData(hItem, _hDataXY);
	//Type of line (default).
	GRAPH_DATA_XY_SetLineStyle(_hDataXY, GUI_LS_SOLID);
	// Update the pointer if the User button is being pressed.
	//Pauses the graph for easy showing potential.
	//Print the Hz of the expected input signal
//		sprintf(result,"%i Hz",value);
//		sprintf(result2,"%i dB",value2);
//		TEXT_SetText(freqText, result);
//		TEXT_SetText(ampltudeText,result2);
	GUI_Exec();
}

// Display the XY data in the time domain.
// domain = 0 is freq 1 is time
// PrePost = 0 is pre and 1 is post prosessing
void GUI_GraphChange(uint8_t displayOption, int8_t channel,
		int8_t filterChannel) {
	sprintf(channelShowFiltered, "Filtering: %i", filterChannel);
	WM_HideWindow(audioD);
	WM_HideWindow(audioFilter);
	if (displayOption == 1) {
		hItem = WM_GetFirstChild(WM_HBKWIN);
		WM_HideWindow(hItem);
		GUI_SetBkColor(GUI_BLACK);
		GUI_Clear();
		WM_ShowWindow(audioD);
		WM_ShowWindow(audioFilter);
		TEXT_SetTextColor(audioD, GUI_WHITE);
		TEXT_SetTextAlign(audioD, GUI_TA_HCENTER);
		TEXT_SetText(audioFilter, channelShowFiltered);
		TEXT_SetTextColor(audioFilter, GUI_WHITE);
		TEXT_SetTextAlign(audioFilter, GUI_TA_HCENTER);
	} else if (displayOption == 2) {
		WM_ShowWindow(hGraph);
		GUI_FillRectEx(&Rect);
		GUI_DispStringInRectEx(dbText, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER,
				strlen(dbText), GUI_ROTATE_CCW);

		//The handlers for the x and y scale. The parent window is the WM_HWIN.
		//The graph needs a handle to ensure data can be changed.
		//No decimals are needed
		GRAPH_SCALE_SetNumDecs(hScale, 0);
		//Show from 0 till -100 dB thus the scale must be divided by 2.
		GRAPH_SCALE_SetFactor(hScale, 0.5);
		//20 pixels between dB labels.
		GRAPH_SCALE_SetTickDist(hScale, 20);
		//Graph shows from 0 till -100 dB
		GRAPH_SCALE_SetOff(hScale, 200);
		//Change colour and attach scale.
		GRAPH_SCALE_SetTextColor(hScale, GUI_BLACK);

		//The labels show till the closest 100Hz
		GRAPH_SCALE_SetNumDecs(hScale2, 0);
		//Each bin is 93,75 Hz thus each pixel must be multiplied by this factor.
		GRAPH_SCALE_SetFactor(hScale2, (float) (scaleXF / 1000.0));
		//30 pixels between kHz labels.
		GRAPH_SCALE_SetTickDist(hScale2, 32);
		//Graph starts at 0 and fits the 257 bins.
		GRAPH_SCALE_SetOff(hScale2, -(float) (162 + (4.9152 * (channel - 1))));
		//Attach the y scale
		GRAPH_SCALE_SetTextColor(hScale2, GUI_BLACK);
		TEXT_SetText(yLabel, "Frequency (kHz)");
		sprintf(channelShowContent, "Channel: %i", channel);
		TEXT_SetText(showChannel, channelShowContent);
		TEXT_SetText(showChannel2, "");
		GUI_FillRectEx(&Rect2);
		GUI_DispStringInRectEx(d1, &Rect2, GUI_TA_HCENTER | GUI_TA_VCENTER,
				strlen(d1), GUI_ROTATE_CCW);
	} else if (displayOption == 3) {
		WM_ShowWindow(hGraph);
		GUI_FillRectEx(&Rect);
		GUI_DispStringInRectEx(dbText, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER,
				strlen(dbText), GUI_ROTATE_CCW);

		//The handlers for the x and y scale. The parent window is the WM_HWIN.
		//The graph needs a handle to ensure data can be changed.
		//No decimals are needed
		GRAPH_SCALE_SetNumDecs(hScale, 0);
		//Show from 0 till -100 dB thus the scale must be divided by 2.
		GRAPH_SCALE_SetFactor(hScale, 0.5);
		//20 pixels between dB labels.
		GRAPH_SCALE_SetTickDist(hScale, 20);
		//Graph shows from 0 till -100 dB
		GRAPH_SCALE_SetOff(hScale, 200);
		//Change colour and attach scale.
		GRAPH_SCALE_SetTextColor(hScale, GUI_BLACK);

		//The labels show till the closest 100Hz
		GRAPH_SCALE_SetNumDecs(hScale2, 0);
		//Each bin is 93,75 Hz thus each pixel must be multiplied by this factor.
		GRAPH_SCALE_SetFactor(hScale2, scaleXF / (1000.0 * FFT_FILTERED_ZOOM));
		//30 pixels between kHz labels.
		GRAPH_SCALE_SetTickDist(hScale2, 32);
		//Graph starts at 0 and fits the 257 bins.
		GRAPH_SCALE_SetOff(hScale2, -((290*FFT_FILTERED_ZOOM - 128) + ((4.9152*FFT_FILTERED_ZOOM) * (channel - 1))));
		//Attach the y scale
		GRAPH_SCALE_SetTextColor(hScale2, GUI_BLACK);
		TEXT_SetText(yLabel, "Frequency (kHz)");

		GUI_FillRectEx(&Rect2);
		GUI_DispStringInRectEx(d3, &Rect2, GUI_TA_HCENTER | GUI_TA_VCENTER,
				strlen(d3), GUI_ROTATE_CCW);
		sprintf(channelShowContent, "Channel: %i", channel);
		TEXT_SetText(showChannel, channelShowContent);
		TEXT_SetText(showChannel2, channelShowFiltered);
	}
	GUI_Exec();
}
