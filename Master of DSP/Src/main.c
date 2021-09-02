/**
 *************************************************
 * @author  C.F. Wagner
 * @author  H. vd Westhuizen
 *************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "GUISetup.h"
#include "GUIMenu.h"
#include <stdio.h>

//#define AUDIO_DETECTION				// Define this to detect if there is audio output and if not, do not output


/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc1;

DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac2;

TIM_HandleTypeDef htim2;

// User variables
// GUI
uint8_t GUI_Initialized = 0;
uint8_t buttonState = 0;
uint8_t prevButtonState = 0;
WM_HWIN hItem;

// Data
uint16_t adc_buff[ADC_BUF_LENG];
uint16_t dac_buff[DAC_BUF_LENG];
uint16_t* adc_in = adc_buff;	// Pointer to the beginning of the adc_buff that should be used.
uint16_t* dac_out = dac_buff;	// Pointer to the beginning of the dac_buff that should be used.
uint16_t* dac_out_inst = dac_buff;

q15_t filter_in_buff[NUM_SAMPLES];
q15_t filtered_buff[NUM_SAMPLES];
q15_t* filtered_buff_ptr = filtered_buff;

q15_t filter_demodulated[DEMODULATE_OUT_NUM_SAMPLES];
q15_t* filter_demodulated_ptr = filter_demodulated;

// Dummy variables
uint32_t dummy32 = 0;

// Selection
uint8_t adc_update = 0; // 0 - No update, 1 - Half complete, 2 - Full complete
uint8_t bDACNotStarted = 1;
uint8_t numFFTBufsFilled = 0;

// Audio detection
#ifdef AUDIO_DETECTION
q15_t max_dac_out = 0;
uint8_t bPrefSignalDetected = 1; // Previous block detected as default
#endif

// FFT
arm_rfft_fast_instance_f32 fft_unfiltered_inst;
arm_rfft_fast_instance_f32 fft_filtered_inst;
float32_t fft_in[NUM_SAMPLES*FFT_FILTERED_ZOOM];
float32_t fft_out[NUM_SAMPLES*FFT_FILTERED_ZOOM]; // N/2 complex numbers. fft_out[0] is DC offset, fft_out[1] is 0.
// X = { real[0], imag[0], real[1], imag[1], real[2], imag[2] ... real[(N/2)-1], imag[(N/2)-1 }
// X = { fft_out[0, fft_out[1]], ... fft_out[NUM_SAMPLES-2], fft_out[NUM_SAMPLES-1] }
float32_t* fft_in_ptr = fft_in;
float32_t* fft_out_ptr = fft_out;

// IRR setup
// Number of 2nd order stages in the IRR filter.
#define NUM_STAGES 2
q15_t pState[NUM_STAGES * 4];
arm_biquad_casd_df1_inst_q15 biquad_inst;
int8_t postShift[NUM_AM_CHANNELS] = {
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
q15_t pCoeffs[NUM_AM_CHANNELS][NUM_STAGES*6] = {
{17, 0, -16, 17, 20427, -16275, 16384, 0, -24700, 16384, 20701, -16276},
{17, 0, -15, 17, 20040, -16275, 16384, 0, -24382, 16384, 20318, -16276},
{17, 0, -15, 17, 19649, -16275, 16384, 0, -24058, 16384, 19930, -16276},
{17, 0, -14, 17, 19253, -16276, 16384, 0, -23729, 16384, 19538, -16276},
{17, 0, -14, 17, 18853, -16276, 16384, 0, -23394, 16384, 19141, -16276},
{17, 0, -13, 17, 18449, -16276, 16384, 0, -23055, 16384, 18740, -16276},
{17, 0, -13, 17, 18041, -16276, 16384, 0, -22710, 16384, 18334, -16276},
{17, 0, -12, 17, 17628, -16276, 16384, 0, -22360, 16384, 17924, -16276},
{17, 0, -12, 17, 17211, -16276, 16384, 0, -22005, 16384, 17511, -16276},
{17, 0, -12, 17, 16791, -16276, 16384, 0, -21645, 16384, 17093, -16276},
{17, 0, -11, 17, 16367, -16276, 16384, 0, -21281, 16384, 16671, -16276},
{17, 0, -11, 17, 15939, -16276, 16384, 0, -20911, 16384, 16246, -16276},
{17, 0, -10, 17, 15507, -16276, 16384, 0, -20537, 16384, 15817, -16276},
{17, 0, -10, 17, 15072, -16276, 16384, 0, -20158, 16384, 15384, -16276},
{17, 0, -9, 17, 14633, -16276, 16384, 0, -19775, 16384, 14948, -16276},
{17, 0, -9, 17, 14191, -16276, 16384, 0, -19388, 16384, 14508, -16276},
{17, 0, -8, 17, 13746, -16276, 16384, 0, -18995, 16384, 14066, -16276},
{17, 0, -8, 17, 13298, -16276, 16384, 0, -18599, 16384, 13619, -16276},
{17, 0, -7, 17, 12846, -16276, 16384, 0, -18199, 16384, 13170, -16276},
{17, 0, -7, 17, 12392, -16276, 16384, 0, -17794, 16384, 12718, -16276},
{17, 0, -6, 17, 11935, -16276, 16384, 0, -17386, 16384, 12263, -16276},
{17, 0, -6, 17, 11475, -16276, 16384, 0, -16973, 16384, 11805, -16276},
{17, 0, -5, 17, 11013, -16276, 16384, 0, -16557, 16384, 11345, -16276},
{17, 0, -5, 17, 10548, -16276, 16384, 0, -16137, 16384, 10882, -16276},
{17, 0, -4, 17, 10081, -16276, 16384, 0, -15714, 16384, 10416, -16276},
{17, 0, -4, 17, 9611, -16276, 16384, 0, -15287, 16384, 9948, -16276},
{17, 0, -3, 17, 9139, -16276, 16384, 0, -14856, 16384, 9478, -16276},
{17, 0, -3, 17, 8666, -16276, 16384, 0, -14423, 16384, 9005, -16276},
{17, 0, -2, 17, 8190, -16276, 16384, 0, -13986, 16384, 8531, -16276},
{17, 0, -2, 17, 7712, -16276, 16384, 0, -13545, 16384, 8055, -16276},
{17, 0, -1, 17, 7233, -16276, 16384, 0, -13102, 16384, 7576, -16276},
{17, 0, -1, 17, 6752, -16276, 16384, 0, -12656, 16384, 7097, -16276},
{17, 0, 0, 17, 6269, -16276, 16384, 0, -12207, 16384, 6615, -16276},
{17, 0, 0, 17, 5785, -16276, 16384, 0, -11756, 16384, 6132, -16276},
{17, 0, 1, 17, 5300, -16276, 16384, 0, -11301, 16384, 5648, -16276},
{17, 0, 1, 17, 4813, -16276, 16384, 0, -10845, 16384, 5162, -16276},
{17, 0, 2, 17, 4325, -16276, 16384, 0, -10385, 16384, 4675, -16276},
{17, 0, 2, 17, 3837, -16276, 16384, 0, -9924, 16384, 4187, -16276},
{17, 0, 3, 17, 3347, -16276, 16384, 0, -9460, 16384, 3698, -16276},
{17, 0, 3, 17, 2857, -16276, 16384, 0, -8994, 16384, 3208, -16276},
{17, 0, 4, 17, 2366, -16276, 16384, 0, -8527, 16384, 2718, -16276},
{17, 0, 4, 17, 1875, -16276, 16384, 0, -8057, 16384, 2227, -16276},
{17, 0, 5, 17, 1383, -16276, 16384, 0, -7585, 16384, 1735, -16276},
{17, 0, 5, 17, 891, -16276, 16384, 0, -7112, 16384, 1243, -16276},
{17, 0, 6, 17, 398, -16276, 16384, 0, -6638, 16384, 751, -16276},
{17, 0, 6, 17, -94, -16276, 16384, 0, -6161, 16384, 259, -16276},
{17, 0, -6, 17, -234, -16276, 16384, 0, 6479, 16384, -587, -16276},
{17, 0, -5, 17, -726, -16276, 16384, 0, 6954, 16384, -1079, -16276},
{17, 0, -5, 17, -1219, -16276, 16384, 0, 7428, 16384, -1571, -16276},
{17, 0, -4, 17, -1711, -16276, 16384, 0, 7900, 16384, -2063, -16276},
{17, 0, -4, 17, -2202, -16276, 16384, 0, 8370, 16384, -2554, -16276},
{17, 0, -3, 17, -2693, -16276, 16384, 0, 8839, 16384, -3045, -16276},
{17, 0, -3, 17, -3184, -16276, 16384, 0, 9305, 16384, -3535, -16276},
{17, 0, -2, 17, -3674, -16276, 16384, 0, 9770, 16384, -4024, -16276},
{17, 0, -2, 17, -4163, -16276, 16384, 0, 10232, 16384, -4512, -16276},
{17, 0, -1, 17, -4651, -16276, 16384, 0, 10692, 16384, -5000, -16276},
{17, 0, -1, 17, -5138, -16276, 16384, 0, 11149, 16384, -5486, -16276},
{17, 0, 0, 17, -5623, -16276, 16384, 0, 11605, 16384, -5971, -16276},
{17, 0, 0, 17, -6108, -16276, 16384, 0, 12057, 16384, -6454, -16276},
{17, 0, 1, 17, -6591, -16276, 16384, 0, 12507, 16384, -6936, -16276},
{17, 0, 1, 17, -7073, -16276, 16384, 0, 12954, 16384, -7417, -16276},
{17, 0, 2, 17, -7552, -16276, 16384, 0, 13398, 16384, -7895, -16276},
{17, 0, 2, 17, -8031, -16276, 16384, 0, 13839, 16384, -8372, -16276},
{17, 0, 3, 17, -8507, -16276, 16384, 0, 14277, 16384, -8847, -16276},
{17, 0, 3, 17, -8982, -16276, 16384, 0, 14712, 16384, -9321, -16276},
{17, 0, 4, 17, -9454, -16276, 16384, 0, 15144, 16384, -9792, -16276},
{17, 0, 4, 17, -9925, -16276, 16384, 0, 15572, 16384, -10260, -16276},
{17, 0, 5, 17, -10393, -16276, 16384, 0, 15997, 16384, -10727, -16276},
{17, 0, 5, 17, -10858, -16276, 16384, 0, 16418, 16384, -11191, -16276},
{17, 0, 6, 17, -11321, -16276, 16384, 0, 16835, 16384, -11652, -16276},
{17, 0, 6, 17, -11782, -16276, 16384, 0, 17249, 16384, -12111, -16276},
{17, 0, 7, 17, -12240, -16276, 16384, 0, 17659, 16384, -12567, -16276},
{17, 0, 7, 17, -12695, -16276, 16384, 0, 18064, 16384, -13020, -16276},
{17, 0, 8, 17, -13148, -16276, 16384, 0, 18466, 16384, -13470, -16276},
{17, 0, 8, 17, -13597, -16276, 16384, 0, 18864, 16384, -13917, -16276},
{17, 0, 9, 17, -14043, -16276, 16384, 0, 19257, 16384, -14361, -16276},
{17, 0, 9, 17, -14486, -16276, 16384, 0, 19646, 16384, -14802, -16276},
{17, 0, 9, 17, -14926, -16276, 16384, 0, 20031, 16384, -15239, -16276},
{17, 0, 10, 17, -15362, -16276, 16384, 0, 20411, 16384, -15673, -16276},
{17, 0, 10, 17, -15795, -16276, 16384, 0, 20787, 16384, -16103, -16276},
{17, 0, 11, 17, -16224, -16276, 16384, 0, 21158, 16384, -16530, -16276},
{17, 0, 11, 17, -16650, -16276, 16384, 0, 21524, 16384, -16953, -16276},
{17, 0, 12, 17, -17072, -16276, 16384, 0, 21886, 16384, -17372, -16276},
{17, 0, 12, 17, -17490, -16276, 16384, 0, 22242, 16384, -17787, -16276},
{17, 0, 13, 17, -17903, -16276, 16384, 0, 22594, 16384, -18198, -16276},
{17, 0, 13, 17, -18313, -16276, 16384, 0, 22940, 16384, -18605, -16276},
{17, 0, 14, 17, -18719, -16276, 16384, 0, 23282, 16384, -19008, -16276},
{17, 0, 14, 17, -19120, -16276, 16384, 0, 23618, 16384, -19406, -16276},
{17, 0, 15, 17, -19518, -16275, 16384, 0, 23949, 16384, -19800, -16276},
{17, 0, 15, 17, -19910, -16275, 16384, 0, 24274, 16384, -20189, -16276},
{17, 0, 15, 17, -20298, -16275, 16384, 0, 24594, 16384, -20574, -16276},
{17, 0, 16, 17, -20682, -16275, 16384, 0, 24909, 16384, -20954, -16276},
{17, 0, 16, 17, -21061, -16275, 16384, 0, 25218, 16384, -21330, -16276},
{17, 0, 17, 17, -21435, -16275, 16384, 0, 25521, 16384, -21700, -16276},
{17, 0, 17, 17, -21804, -16275, 16384, 0, 25819, 16384, -22066, -16276},
{17, 0, 18, 17, -22168, -16275, 16384, 0, 26111, 16384, -22426, -16276},
{17, 0, 18, 17, -22527, -16275, 16384, 0, 26397, 16384, -22782, -16276},
{17, 0, 18, 17, -22881, -16275, 16384, 0, 26677, 16384, -23132, -16277},
{17, 0, 19, 17, -23230, -16275, 16384, 0, 26952, 16384, -23477, -16277},
{17, 0, 19, 17, -23573, -16275, 16384, 0, 27220, 16384, -23817, -16277},
{17, 0, 20, 17, -23911, -16275, 16384, 0, 27482, 16384, -24151, -16277},
{17, 0, 20, 17, -24244, -16275, 16384, 0, 27738, 16384, -24480, -16277},
{17, 0, 20, 17, -24571, -16275, 16384, 0, 27988, 16384, -24803, -16277},
{17, 0, 21, 17, -24893, -16275, 16384, 0, 28232, 16384, -25121, -16277},
{17, 0, 21, 17, -25209, -16275, 16384, 0, 28470, 16384, -25433, -16277},
{17, 0, 22, 17, -25519, -16275, 16384, 0, 28701, 16384, -25739, -16277},
{17, 0, 22, 17, -25824, -16275, 16384, 0, 28926, 16384, -26039, -16277},
{17, 0, 22, 17, -26122, -16275, 16384, 0, 29144, 16384, -26334, -16277},
{17, 0, 23, 17, -26415, -16275, 16384, 0, 29357, 16384, -26622, -16277},
{17, 0, 23, 17, -26701, -16275, 16384, 0, 29562, 16384, -26904, -16277},
{17, 0, 23, 17, -26982, -16275, 16384, 0, 29761, 16384, -27180, -16277},
{17, 0, 24, 17, -27256, -16275, 16384, 0, 29954, 16384, -27450, -16277},
{17, 0, 24, 17, -27524, -16275, 16384, 0, 30140, 16384, -27714, -16277},
{17, 0, 24, 17, -27786, -16275, 16384, 0, 30320, 16384, -27972, -16277},
{17, 0, 25, 17, -28042, -16275, 16384, 0, 30492, 16384, -28223, -16277},
{17, 0, 25, 17, -28291, -16275, 16384, 0, 30659, 16384, -28468, -16277},
{17, 0, 25, 17, -28534, -16275, 16384, 0, 30818, 16384, -28706, -16277},
{17, 0, 26, 17, -28770, -16275, 16384, 0, 30971, 16384, -28937, -16277},
{17, 0, 26, 17, -29000, -16275, 16384, 0, 31118, 16384, -29163, -16277},
{17, 0, 26, 17, -29223, -16275, 16384, 0, 31257, 16384, -29381, -16277}
};


// Functions within main
static void BSP_Config(void);
static void SystemClock_Config(void);
static void MX_DMA_Init(void);
void BSP_Background(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_DAC_Init(void);
static void MX_ADC2_Init(void);
static void MX_ADC3_Init(void);

int main(void) {
	/* STM32F4xx HAL library initialization */
	HAL_Init();

	/* Initialize LCD and LEDs */
	BSP_Config();
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);

	/* Configure the system clock to 180 MHz */
	SystemClock_Config();

	MX_DMA_Init();
	MX_ADC1_Init();
	MX_TIM2_Init();
	MX_DAC_Init();
	MX_ADC2_Init();
	MX_ADC3_Init();

	// Initialize the STEMWIN configuration files
	GUI_Init();
	GUI_Initialized = 1;

	/* Activate the use of memory device feature */
	WM_SetCreateFlags(WM_CF_MEMDEV);
	Graph_Setup();
	GUI_GraphChange(displayOption,channelDisplayOption, filterDisplayOption);

	// Setup DAC and ADC's, but do not start yet.
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t*) dac_buff, DAC_BUF_LENG/2, DAC_ALIGN_12B_R);
	HAL_ADC_Start(&hadc3);
	HAL_ADC_Start(&hadc2);

	// Start the sampling after the setup is complete
	// Divide length by 2, since 16-bit array casted to 32-bit array
	HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*) adc_buff, ADC_BUF_LENG/2);


	// FFT inti
	arm_rfft_fast_init_f32(&fft_unfiltered_inst, NUM_SAMPLES);
	arm_rfft_fast_init_f32(&fft_filtered_inst, NUM_SAMPLES*FFT_FILTERED_ZOOM);

	// IIR filter init
	arm_biquad_cascade_df1_init_q15(&biquad_inst, NUM_STAGES, pCoeffs[filterDisplayOption-1], pState, postShift[filterDisplayOption-1]);

	while (1) {
		// Check if mode select
		buttonState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

		// Change the button state only once per button press.
		if (buttonState != prevButtonState) {
			prevButtonState = buttonState;
			if (buttonState == 1) {
				// Set DAC output to off (center).
				arm_fill_q15(2048, (q15_t*)dac_buff, DAC_BUF_LENG);

				TEXT_SetTextColor(audioD, GUI_BLACK);
				TEXT_SetTextColor(audioFilter, GUI_BLACK);
				GUI_SetBkColor(GUI_BLACK);
				GUI_Menu();
				GUI_GraphChange(displayOption,channelDisplayOption, filterDisplayOption);
				// displayOption:
				// 1 - Audio
				// 2 - Unfiltered
				// 3 - Filtered


				// Init the filter again with new channel
				arm_biquad_cascade_df1_init_q15(&biquad_inst, NUM_STAGES, pCoeffs[filterDisplayOption-1], pState, postShift[filterDisplayOption-1]);

				// Reset LED and wait for next ADC buffer being red in.
				BSP_LED_Off(LED4);
				adc_update = 0;
				numFFTBufsFilled = 0;

#ifdef AUDIO_DETECTION
				bPrefSignalDetected = 1;
#endif
			}
			GUI_Exec();
		}

		// Check for update
		if (adc_update != 0){
			// Select the array to work with
			if (adc_update == 1){
				// First half
				adc_in = adc_buff;
				dac_out = dac_buff;
			} else{
				// Second half
				adc_in = (uint16_t*)(&adc_buff[NUM_SAMPLES]);
				dac_out = (uint16_t*)(&dac_buff[DEMODULATE_OUT_NUM_SAMPLES]);
			}

			// Shift input
			arm_offset_q15((q15_t*)adc_in, -2048, filter_in_buff, NUM_SAMPLES);

			// Select mode
			if (displayOption == 1){
				// Audio

				// Apply pre-scaling
				arm_shift_q15(filter_in_buff, 1, filter_in_buff, NUM_SAMPLES); // Left shift

				// Filter data
				arm_biquad_cascade_df1_fast_q15(&biquad_inst, filter_in_buff, filtered_buff, NUM_SAMPLES);

				// Demodulate
				filtered_buff_ptr = filtered_buff;
				filter_demodulated_ptr = filter_demodulated;

				for (int i = DEMODULATE_OUT_NUM_SAMPLES/4; i > 0; i -= 1){
					arm_max_q15(filtered_buff_ptr, DOWNSAMPLING_FACTOR, filter_demodulated_ptr++, &dummy32);
					filtered_buff_ptr += DOWNSAMPLING_FACTOR;
					arm_max_q15(filtered_buff_ptr, DOWNSAMPLING_FACTOR, filter_demodulated_ptr++, &dummy32);
					filtered_buff_ptr += DOWNSAMPLING_FACTOR;
					arm_max_q15(filtered_buff_ptr, DOWNSAMPLING_FACTOR, filter_demodulated_ptr++, &dummy32);
					filtered_buff_ptr += DOWNSAMPLING_FACTOR;
					arm_max_q15(filtered_buff_ptr, DOWNSAMPLING_FACTOR, filter_demodulated_ptr++, &dummy32);
					filtered_buff_ptr += DOWNSAMPLING_FACTOR;
				}


				// Apply offset and output to DAC
				arm_offset_q15(filter_demodulated, 100, filter_demodulated, DEMODULATE_OUT_NUM_SAMPLES);
				arm_scale_q15(filter_demodulated, 26200, 0, (q15_t*)dac_out, DEMODULATE_OUT_NUM_SAMPLES); // Scale by 0.8

				// Check limit to min and max to avoid overflow
				// Unroll the loop
				dac_out_inst = dac_out;
				for (int i = DEMODULATE_OUT_NUM_SAMPLES/4; i > 0; i -= 1){
					if (*(q15_t*)dac_out_inst < 0)
						*dac_out_inst = 0;
					if (*dac_out_inst++ > 4095)
						*(dac_out_inst-1) = 4095;

					if (*(q15_t*)dac_out_inst < 0)
						*dac_out_inst = 0;
					if (*dac_out_inst++ > 4095)
						*(dac_out_inst-1) = 4095;

					if (*(q15_t*)dac_out_inst < 0)
						*dac_out_inst = 0;
					if (*dac_out_inst++ > 4095)
						*(dac_out_inst-1) = 4095;

					if (*(q15_t*)dac_out_inst < 0)
						*dac_out_inst = 0;
					if (*dac_out_inst++ > 4095)
						*(dac_out_inst-1) = 4095;
				}

				if (bDACNotStarted){
					for (int i = 0; i<1000; i++); // Wait a bit
					HAL_TIM_Base_Start_IT(&htim2); // Start DMA (Should be a bit behind the ADC) ( TODO: Maybe not here. )
					bDACNotStarted = 0;
				}

#ifdef AUDIO_DETECTION
				arm_max_q15((q15_t*)dac_out, DEMODULATE_OUT_NUM_SAMPLES, &max_dac_out, &dummy32);

				if (bPrefSignalDetected == 0){
					// Output no signal if the previous block did not contain any useful data.
					arm_fill_q15(2048, (q15_t*)dac_out, DEMODULATE_OUT_NUM_SAMPLES);
				}

				if (max_dac_out >= 400)
					// Signal detected
					bPrefSignalDetected = 1;
				else
					// Signal not detected
					bPrefSignalDetected = 0;
#endif

			} else if (displayOption == 2){
				// Unfiltered

				// Apply pre-scaling
				// arm_shift_q15(filter_in_buff, 3, filter_in_buff, NUM_SAMPLES); // Left shift

				// Convert to float
				arm_q15_to_float(filter_in_buff, fft_in, NUM_SAMPLES);

				// Do FFT
				arm_rfft_fast_f32(&fft_unfiltered_inst, fft_in, fft_out, 0);

				// Get absolute
				arm_cmplx_mag_f32(fft_out, fft_in, NUM_SAMPLES/2); // Number of complex samples, thus half NUM_SAMPLES

				// Scale FFT
				arm_scale_f32(fft_in, 26.4*2.0/(float32_t)NUM_SAMPLES, fft_in, NUM_SAMPLES/2);
				fft_in[0] /= 2.0;

				// Convert magnitude to dB
				fft_in_ptr = fft_in;

				for (int i = NUM_SAMPLES/8; i > 0; i -= 1){
					*fft_in_ptr++ = 40*log10f(*fft_in_ptr); // Times two for output scaling
					*fft_in_ptr++ = 40*log10f(*fft_in_ptr);
					*fft_in_ptr++ = 40*log10f(*fft_in_ptr);
					*fft_in_ptr++ = 40*log10f(*fft_in_ptr);
				}

				// GUI
				GUI_XY_Graph(fft_in, 256, displayOption, channelDisplayOption);

			} else{
				// Filtered

				// Apply pre-scaling
				arm_shift_q15(filter_in_buff, 1, filter_in_buff, NUM_SAMPLES); // Left shift

				// Filter data
				arm_biquad_cascade_df1_fast_q15(&biquad_inst, filter_in_buff, filtered_buff, NUM_SAMPLES);

				// Convert to float (Output to the larger fft_in, according to the value of numFFTBufsFilled.)
				arm_q15_to_float(filtered_buff, &fft_in[NUM_SAMPLES*numFFTBufsFilled], NUM_SAMPLES);

				// Only if the whole FFT buffer has been, filled do the FFT and display it, else wait for the buffer to be filled.
				if (numFFTBufsFilled >= (FFT_FILTERED_ZOOM-1)){
					// Do FFT
					arm_rfft_fast_f32(&fft_filtered_inst, fft_in, fft_out, 0);

					// Get absolute
					// Number of complex samples, thus half NUM_SAMPLES*FFT_FILTERED_ZOOM.
					arm_cmplx_mag_f32(fft_out, fft_in, NUM_SAMPLES*FFT_FILTERED_ZOOM/2);

					// Scale FFT
					arm_scale_f32(fft_in, 13.2*2.0/(float32_t)(NUM_SAMPLES*FFT_FILTERED_ZOOM), fft_in, NUM_SAMPLES*FFT_FILTERED_ZOOM/2);
					fft_in[0] /= 2.0;

					// Convert magnitude to dB
					// Use loop unrolling
					fft_in_ptr = fft_in;

					for (int i = NUM_SAMPLES*FFT_FILTERED_ZOOM/8; i > 0; i -= 1){
						*fft_in_ptr++ = 40*log10f(*fft_in_ptr); // Times two for output scaling
						*fft_in_ptr++ = 40*log10f(*fft_in_ptr);
						*fft_in_ptr++ = 40*log10f(*fft_in_ptr);
						*fft_in_ptr++ = 40*log10f(*fft_in_ptr);
					}

					// GUI
					GUI_XY_Graph(fft_in, 256, displayOption, channelDisplayOption);

					// Reset the counter to start filling up the buffer again.
					numFFTBufsFilled = 0;
				} else {
					// Go to the next buffer
					numFFTBufsFilled++;
				}
			}

			// Get ready for next time it is called.
			adc_update = 0;
		}
	}

}

// Called when ADC Buf is half full
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
	BSP_LED_Toggle(LED3);
	if (adc_update == 0){
		adc_update = 1;
	} else {
		BSP_LED_Toggle(LED4);
	}
}

// Called when ADC Buf is completely full
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	BSP_LED_Toggle(LED3);
	if (adc_update == 0){
		adc_update = 2;
	} else {
		BSP_LED_Toggle(LED4);
	}
}

static void BSP_Config(void) {
	//The GUI needs to make use of the SDRAM.
	BSP_SDRAM_Init();
	//The Touch interface.
	BSP_TS_Init(240, 320);
	//Needs the RCC clock.
	__HAL_RCC_CRC_CLK_ENABLE();
}

//The system clock is configured to ensure the GUI and FFT completes
//within the alloted time period.
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 180;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_MultiModeTypeDef multimode = { 0 };
	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */
	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = DISABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}
	/** Configure the ADC multi-mode
	 */
	multimode.Mode = ADC_TRIPLEMODE_INTERL;
	multimode.DMAAccessMode = ADC_DMAACCESSMODE_2;
	multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_6CYCLES;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
		Error_Handler();
	}
	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_13;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief ADC2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC2_Init(void) {

	/* USER CODE BEGIN ADC2_Init 0 */

	/* USER CODE END ADC2_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC2_Init 1 */

	/* USER CODE END ADC2_Init 1 */
	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc2.Instance = ADC2;
	hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc2.Init.Resolution = ADC_RESOLUTION_12B;
	hadc2.Init.ScanConvMode = DISABLE;
	hadc2.Init.ContinuousConvMode = ENABLE;
	hadc2.Init.DiscontinuousConvMode = DISABLE;
	hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc2.Init.NbrOfConversion = 1;
	hadc2.Init.DMAContinuousRequests = DISABLE;
	hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc2) != HAL_OK) {
		Error_Handler();
	}
	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_13;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC2_Init 2 */

	/* USER CODE END ADC2_Init 2 */

}

/**
 * @brief ADC3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC3_Init(void) {

	/* USER CODE BEGIN ADC3_Init 0 */

	/* USER CODE END ADC3_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC3_Init 1 */

	/* USER CODE END ADC3_Init 1 */
	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc3.Instance = ADC3;
	hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc3.Init.Resolution = ADC_RESOLUTION_12B;
	hadc3.Init.ScanConvMode = DISABLE;
	hadc3.Init.ContinuousConvMode = ENABLE;
	hadc3.Init.DiscontinuousConvMode = DISABLE;
	hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc3.Init.NbrOfConversion = 1;
	hadc3.Init.DMAContinuousRequests = DISABLE;
	hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc3) != HAL_OK) {
		Error_Handler();
	}
	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_13;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC3_Init 2 */

	/* USER CODE END ADC3_Init 2 */

}

/**
 * @brief DAC Initialization Function
 * @param None
 * @retval None
 */
static void MX_DAC_Init(void) {

	/* USER CODE BEGIN DAC_Init 0 */

	/* USER CODE END DAC_Init 0 */

	DAC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN DAC_Init 1 */

	/* USER CODE END DAC_Init 1 */
	/** DAC Initialization
	 */
	hdac.Instance = DAC;
	if (HAL_DAC_Init(&hdac) != HAL_OK) {
		Error_Handler();
	}
	/** DAC channel OUT2 config
	 */
	sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN DAC_Init 2 */

	/* USER CODE END DAC_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 1536*2-1; // DAC output at 58593 sps
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream6_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	/* DMA2_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

// Allocate and deallocate (init an deinit) different resources.
/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void) {

	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();

	/* System interrupt init*/
}

/**
 * @brief ADC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hadc: ADC handle pointer
 * @retval None
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (hadc->Instance == ADC1) {
		/* Peripheral clock enable */
		__HAL_RCC_ADC1_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();
		/**ADC1 GPIO Configuration
		 PC3     ------> ADC1_IN13
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		/* ADC1 DMA Init */
		/* ADC1 Init */
		hdma_adc1.Instance = DMA2_Stream0;
		hdma_adc1.Init.Channel = DMA_CHANNEL_0;
		hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
		hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		hdma_adc1.Init.Mode = DMA_CIRCULAR;
		hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
		hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
			Error_Handler();
		}

		__HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);

	} else if (hadc->Instance == ADC2) {
		/* Peripheral clock enable */
		__HAL_RCC_ADC2_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();
		/**ADC2 GPIO Configuration
		 PC3     ------> ADC2_IN13
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	} else if (hadc->Instance == ADC3) {
		/* Peripheral clock enable */
		__HAL_RCC_ADC3_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();
		/**ADC3 GPIO Configuration
		 PC3     ------> ADC3_IN13
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	}

}

/**
 * @brief ADC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hadc: ADC handle pointer
 * @retval None
 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {
	if (hadc->Instance == ADC1) {
		/* Peripheral clock disable */
		__HAL_RCC_ADC1_CLK_DISABLE();

		/**ADC1 GPIO Configuration
		 PC3     ------> ADC1_IN13
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);

		/* ADC1 DMA DeInit */
		HAL_DMA_DeInit(hadc->DMA_Handle);
	} else if (hadc->Instance == ADC2) {
		/* Peripheral clock disable */
		__HAL_RCC_ADC2_CLK_DISABLE();

		/**ADC2 GPIO Configuration
		 PC3     ------> ADC2_IN13
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);

	} else if (hadc->Instance == ADC3) {
		/* Peripheral clock disable */
		__HAL_RCC_ADC3_CLK_DISABLE();

		/**ADC3 GPIO Configuration
		 PC3     ------> ADC3_IN13
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);
	}

}

/**
 * @brief DAC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hdac: DAC handle pointer
 * @retval None
 */
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (hdac->Instance == DAC) {
		/* Peripheral clock enable */
		__HAL_RCC_DAC_CLK_ENABLE();

		__HAL_RCC_GPIOA_CLK_ENABLE();
		/**DAC GPIO Configuration
		 PA5     ------> DAC_OUT2
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_5;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* DAC DMA Init */
		/* DAC2 Init */
		hdma_dac2.Instance = DMA1_Stream6;
		hdma_dac2.Init.Channel = DMA_CHANNEL_7;
		hdma_dac2.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_dac2.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_dac2.Init.MemInc = DMA_MINC_ENABLE;
		hdma_dac2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		hdma_dac2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		hdma_dac2.Init.Mode = DMA_CIRCULAR;
		hdma_dac2.Init.Priority = DMA_PRIORITY_MEDIUM;
		hdma_dac2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		if (HAL_DMA_Init(&hdma_dac2) != HAL_OK) {
			Error_Handler();
		}

		__HAL_LINKDMA(hdac, DMA_Handle2, hdma_dac2);
	}

}

/**
 * @brief DAC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hdac: DAC handle pointer
 * @retval None
 */
void HAL_DAC_MspDeInit(DAC_HandleTypeDef *hdac) {
	if (hdac->Instance == DAC) {
		/* Peripheral clock disable */
		__HAL_RCC_DAC_CLK_DISABLE();

		/**DAC GPIO Configuration
		 PA5     ------> DAC_OUT2
		 */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);

		/* DAC DMA DeInit */
		HAL_DMA_DeInit(hdac->DMA_Handle2);
	}

}

/**
 * @brief TIM_Base MSP Initialization
 * This function configures the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {
	if (htim_base->Instance == TIM2) {
		/* Peripheral clock enable */
		__HAL_RCC_TIM2_CLK_ENABLE();
	}

}

/**
 * @brief TIM_Base MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base) {
	if (htim_base->Instance == TIM2) {
		/* Peripheral clock disable */
		__HAL_RCC_TIM2_CLK_DISABLE();
	}

}


/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
