/*

faustreceiver

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset */
#include <math.h>

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "stm32746g_discovery_sd.h"
#include "stm32f7xx_hal.h"
#include "common/clockconfig.h"

#define ARM_MATH_CM7
#include "arm_math.h"

#include "Heavy_letest.h"

// defines
#define VOLUME 10
#define SAMPLE_RATE 24000
#define AUDIO_DMA_BUFFER_SIZE 2048  // divided by 4 (C array item length) reserves a stereo audio buffer of 1024 bytes
                                    // translating to 2x512 bytes mono
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)

// audio buffers
static int16_t int_bufProcessedOut[AUDIO_DMA_BUFFER_SIZE];
static uint8_t audioOutBuf[AUDIO_DMA_BUFFER_SIZE];
extern SAI_HandleTypeDef haudio_out_sai;

// midi
UART_HandleTypeDef uart_config;
uint8_t midicounter=0;
int16_t mididata[3] = {0,-1,-1};
uint8_t rx_byte[1];


// typedef struct {
//
//   int fSamplingFreq;
//   int iVec0[2];
//   float fRec0[2];
//   float fRec3[2];
//   float fRec1[2];
//   float fRec2[2];
//   FAUSTFLOAT fHslider0;
//   float fConst0;
//   float fConst1;
//   FAUSTFLOAT fHslider1;
//   float fConst2;
//   FAUSTFLOAT fHslider2;
//
// } mydsp;
//
// // mydsp* newmydsp() {
// //   mydsp* dsp = (mydsp*)malloc(sizeof(mydsp));
// //   return dsp;
// // }
//
// mydsp* dsp;
// float* outputs[512];


double sampleRate = 24000;
HeavyContextInterface *context;//= hv_letest_new(sampleRate);
// *context;// = hv//_letest_new(sampleRate);

// header
void initAudio();
void computeAudio();
void UART6_Config();
void handle_midi();
// void calcCoeffs();
// void computemydsp(mydsp* dsp, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs);
// void initmydsp(mydsp* dsp, int samplingFreq);


int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config(); 
  
  // config UART for MIDI communication
  UART6_Config();
  
  // led and pushbutton config
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Off(LED_GREEN);
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
  
  // Init LCD and Touchscreen
  BSP_LCD_Init();
  if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_OK) {
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
	  BSP_TS_ITConfig();
  }  
  
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);

  // start audio system
  initAudio();
  
  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)"faustreceiver", CENTER_MODE);
  
  //newmydsp();
  //dsp->fHslider0 = 2.0f;
  //initmydsp(dsp, 48000);
  context = hv_letest_new(sampleRate);
  
  int blah = hv_getNumOutputChannels(context);
  
  // main loop
  while (1) {

    // do nothing
    //BSP_LED_Toggle(LED_GREEN);
    char a[] = "";
    // //sprintf(a, "%d", (int)filterfreq);
    sprintf(a, "%d", (int)HAL_GetTick());
    // //sprintf(a, "%d", (int)result*1000);
    BSP_LCD_DisplayStringAt(50, 50, (uint8_t *)a, LEFT_MODE);

  }

  return 0;
}

void initAudio() {
    if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, VOLUME, SAMPLE_RATE) != 0) {
      BSP_LCD_DisplayStringAt(5, 5, (uint8_t *)"initAudio error", LEFT_MODE);
      Error_Handler();
    }
    
    BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
    BSP_AUDIO_OUT_SetVolume(VOLUME);
    BSP_AUDIO_OUT_Play((uint16_t *)audioOutBuf, AUDIO_DMA_BUFFER_SIZE);

}

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void) {
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
  // clear global buffer
  memset(int_bufProcessedOut, 0, sizeof int_bufProcessedOut);
	computeAudio();
	memcpy(audioOutBuf, int_bufProcessedOut, AUDIO_DMA_BUFFER_SIZE2);  
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  memset(int_bufProcessedOut, 0, sizeof int_bufProcessedOut);
	computeAudio();
	memcpy(&audioOutBuf[AUDIO_DMA_BUFFER_SIZE2], int_bufProcessedOut, AUDIO_DMA_BUFFER_SIZE2);
}

void computeAudio() {

  float audioproc[512];
  float32_t mixdown[1024];

  // calculate 512 monosamples
  // for(int sampleit=0;sampleit<512;sampleit++) {
//
//     audioproc[sampleit] = 0;
//
//   }
  
  //computemydsp(dsp, 512, 0, outputs);
  
  hv_processInline(context, NULL, audioproc, 256);
  
  // double array mono -> stereo
  int counter = 0;
  for(int sampleit=0;sampleit<1024;sampleit+=2) {
    mixdown[sampleit] = audioproc[counter];
    mixdown[sampleit+1] = audioproc[counter];
    counter++;
  }

  // convert float to q15 / int16
  // send to transferbuffer
  arm_float_to_q15(mixdown, int_bufProcessedOut, 512);
}







// void classInitmydsp(int samplingFreq) {
//
// }
//
// void instanceResetUserInterfacemydsp(mydsp* dsp) {
//   dsp->fHslider0 = (FAUSTFLOAT)-20.0f;
//   dsp->fHslider1 = (FAUSTFLOAT)49.0f;
//   dsp->fHslider2 = (FAUSTFLOAT)0.10000000000000001f;
//
// }
//
// void instanceClearmydsp(mydsp* dsp) {
//   /* C99 loop */
//   {
//     int i0;
//     for (i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
//       dsp->iVec0[i0] = 0;
//
//     }
//
//   }
//   /* C99 loop */
//   {
//     int i1;
//     for (i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
//       dsp->fRec0[i1] = 0.0f;
//
//     }
//
//   }
//   /* C99 loop */
//   {
//     int i2;
//     for (i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
//       dsp->fRec3[i2] = 0.0f;
//
//     }
//
//   }
//   /* C99 loop */
//   {
//     int i3;
//     for (i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
//       dsp->fRec1[i3] = 0.0f;
//
//     }
//
//   }
//   /* C99 loop */
//   {
//     int i4;
//     for (i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
//       dsp->fRec2[i4] = 0.0f;
//
//     }
//
//   }
//
// }
//
// void instanceConstantsmydsp(mydsp* dsp, int samplingFreq) {
//   dsp->fSamplingFreq = samplingFreq;
//   dsp->fConst0 = min(192000.0f, max(1.0f, (float)dsp->fSamplingFreq));
//   dsp->fConst1 = (6.28318548f / dsp->fConst0);
//   dsp->fConst2 = (1.0f / dsp->fConst0);
//
// }
//
// void instanceInitmydsp(mydsp* dsp, int samplingFreq) {
//   instanceConstantsmydsp(dsp, samplingFreq);
//   instanceResetUserInterfacemydsp(dsp);
//   instanceClearmydsp(dsp);
// }
//
// void initmydsp(mydsp* dsp, int samplingFreq) {
//   classInitmydsp(samplingFreq);
//   instanceInitmydsp(dsp, samplingFreq);
// }
//
// void computemydsp(mydsp* dsp, int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
//   FAUSTFLOAT* output0 = outputs[0];
//   float fSlow0 = (0.00100000005f * powf(10.0f, (0.0500000007f * (float)dsp->fHslider0)));
//   float fSlow1 = expf((0.0f - (dsp->fConst2 / (float)dsp->fHslider2)));
//   float fSlow2 = (440.0f * (powf(2.0f, (0.0833333358f * ((float)dsp->fHslider1 + -49.0f))) * (1.0f - fSlow1)));
//   /* C99 loop */
//   {
//     int i;
//     for (i = 0; (i < count); i = (i + 1)) {
//       dsp->iVec0[0] = 1;
//       dsp->fRec0[0] = (fSlow0 + (0.999000013f * dsp->fRec0[1]));
//       dsp->fRec3[0] = (fSlow2 + (fSlow1 * dsp->fRec3[1]));
//       float fTemp0 = (dsp->fConst1 * dsp->fRec3[0]);
//       float fTemp1 = sinf(fTemp0);
//       float fTemp2 = cosf(fTemp0);
//       dsp->fRec1[0] = ((dsp->fRec2[1] * fTemp1) + (dsp->fRec1[1] * fTemp2));
//       dsp->fRec2[0] = (((dsp->fRec2[1] * fTemp2) + (dsp->fRec1[1] * (0.0f - fTemp1))) + (float)(1 - dsp->iVec0[1]));
//       output0[i] = (FAUSTFLOAT)(dsp->fRec0[0] * dsp->fRec1[0]);
//       dsp->iVec0[1] = dsp->iVec0[0];
//       dsp->fRec0[1] = dsp->fRec0[0];
//       dsp->fRec3[1] = dsp->fRec3[0];
//       dsp->fRec1[1] = dsp->fRec1[0];
//       dsp->fRec2[1] = dsp->fRec2[0];
//
//     }
//
//   }
//
// }












// UART (MIDI) HANDLING --------------------------------------------

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  //BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)"le midi", CENTER_MODE);
  // check for status (>127) or data
  // does not handle aftertouch yet!
  // e.g. an message with just 1 data byte
  if(rx_byte[0]>127) {
    mididata[0]=rx_byte[0];
  } 
  else
    // data coming in, in two bytes
  {
    if(mididata[1]==-1) {
      mididata[1] = rx_byte[0];
    } else {
      mididata[2] = rx_byte[0];
      handle_midi();
      // float req_freq = (13.75 * (pow(2,(mididata[1]-9.0)/12.0)));
//       osc->freq = req_freq;
//       osc2->freq = (13.75 * (pow(2,(mididata[1]-12.0-9.0)/12.0)));
//       adsr_trig = 1;
      
      mididata[1] = -1;
    }
  }
  // reset interrupt
  HAL_UART_Receive_IT(&uart_config, rx_byte, 1);
}  
  
void handle_midi() {

  // note on if vel (mididata[2]) > 0
  if(mididata[0] == 144 && mididata[2] > 0) {
    // do midi stuff
  }
  
  // implicit note off
  // or note off
  if((mididata[0] == 144 && mididata[2] == 0)  || (mididata[0] == 128)) {
    // do midi stuff
  }

  // controllers
 if(mididata[0] == 176) {
   if(mididata[1]==71) {
    
   }
   
   if(mididata[1]==72) {
   
   }
 }
}



void UART6_Config() {
  
//************  UART CONFIG  *****************************//

  __USART6_CLK_ENABLE();

  uart_config.Instance=USART6;

  uart_config.Init.BaudRate=31250;
  uart_config.Init.WordLength=UART_WORDLENGTH_8B;
  uart_config.Init.StopBits=UART_STOPBITS_1;
  uart_config.Init.Parity=UART_PARITY_NONE;
  uart_config.Init.Mode=UART_MODE_RX;
  uart_config.Init.HwFlowCtl=UART_HWCONTROL_NONE;
  //uart_config.Init.OverSampling = UART_OVERSAMPLING_8;
  
  HAL_UART_Init(&uart_config);
  
  HAL_NVIC_SetPriority(USART6_IRQn,10,1);
  HAL_NVIC_EnableIRQ(USART6_IRQn);

//**********************************************************//

//************ UART GPIO CONFIG  *********************//

  GPIO_InitTypeDef uart_gpio;

  __GPIOC_CLK_ENABLE();

  uart_gpio.Pin=GPIO_PIN_7;
  uart_gpio.Mode=GPIO_MODE_AF_PP;
  //uart_gpio.Pull=GPIO_NOPULL;
  uart_gpio.Pull=GPIO_PULLUP;
  uart_gpio.Speed=GPIO_SPEED_FAST;
  uart_gpio.Alternate=GPIO_AF8_USART6;

  HAL_GPIO_Init(GPIOC, &uart_gpio);
  HAL_UART_Receive_IT(&uart_config, rx_byte, 1);
}

void USART6_IRQHandler(void)
{
  HAL_UART_IRQHandler(&uart_config);
}
