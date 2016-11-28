/*

soundpipe on stm32f7 discovery

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

#include "soundpipe.h"

/* SOUNDPIPE */

static sp_data *sp;
static sp_blsaw *blsaw;
static sp_blsaw *blsaw2;
static sp_ftbl *ft;
static sp_posc3 *posc3[8];
static sp_osc *osc;
static sp_osc *osc2;



uint8_t audioReady = 0;

#define VOLUME 90
#define SAMPLE_RATE 48000
#define AUDIO_DMA_BUFFER_SIZE 4096
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)

// audio buffers
static int16_t int_bufProcessedOut[AUDIO_DMA_BUFFER_SIZE];
static uint8_t audioOutBuf[AUDIO_DMA_BUFFER_SIZE];

extern SAI_HandleTypeDef haudio_out_sai;

// header
void initAudio();
void computeAudio();
void drawInterface();
void ConfigureADC();
void ConfigureDMA();

uint32_t g_ADCValue;
int g_MeasurementNumber;
ADC_HandleTypeDef g_AdcHandle;
DMA_HandleTypeDef  g_DmaHandle;
enum{ ADC_BUFFER_LENGTH = 8192 };
uint32_t g_ADCBuffer[ADC_BUFFER_LENGTH];

uint16_t ADCchannelValues[4];

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config(); 
  ConfigureADC();
  ConfigureDMA();
  HAL_ADC_Start_DMA(&g_AdcHandle, g_ADCBuffer, ADC_BUFFER_LENGTH);
  
	//volatile uint32_t counter = 1000;
	//while(counter--);
  
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Off(LED_GREEN);

  // Init LCD and Touchscreen
  BSP_LCD_Init();
  if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_OK) {
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
	  BSP_TS_ITConfig();
  }  

BSP_LCD_Clear(LCD_COLOR_DARKMAGENTA);
  initAudio();

  drawInterface();
  
  sp_create(&sp);
  sp->sr = 48000;
  
  sp_ftbl_create(sp, &ft, 600);
  //sp_osc_create(&osc);
  for(int i=0; i<4;i++)
  {
    sp_posc3_create(&posc3[i]);
  }
  

  sp_blsaw_create(&blsaw);
  sp_blsaw_create(&blsaw2);

  sp_osc_create(&osc);
  sp_osc_create(&osc2);

  sp_blsaw_init(sp, blsaw);
  *blsaw->freq = 200;
  *blsaw->amp = 0.5f;
  
  sp_blsaw_init(sp, blsaw2);
  *blsaw2->freq = 440;
  *blsaw2->amp = 0.5f;
  
  //sp_gen_sine(sp, ft);
  
  float lewave[600] = {0.00665283203125, 0.013336181640625, 0.019989013671875, 0.026641845703125, 0.033294677734375, 0.040008544921875, 0.046661376953125, 0.053314208984375, 0.059967041015625, 0.066619873046875, 0.073333740234375, 0.079986572265625, 0.086669921875, 0.09332275390625, 0.0999755859375, 0.10662841796875, 0.11334228515625, 0.1199951171875, 0.12664794921875, 0.13330078125, 0.13995361328125, 0.14666748046875, 0.1533203125, 0.160003662109375, 0.166656494140625, 0.173309326171875, 0.179962158203125, 0.186676025390625, 0.193328857421875, 0.199981689453125, 0.206634521484375, 0.213287353515625, 0.220001220703125, 0.226654052734375, 0.23333740234375, 0.239990234375, 0.24664306640625, 0.253326416015625, 0.259979248046875, 0.26666259765625, 0.2733154296875, 0.27996826171875, 0.28662109375, 0.2933349609375, 0.29998779296875, 0.306671142578125, 0.313323974609375, 0.32000732421875, 0.32666015625, 0.33331298828125, 0.339996337890625, 0.346649169921875, 0.353302001953125, 0.359954833984375, 0.366668701171875, 0.373321533203125, 0.379974365234375, 0.38665771484375, 0.393341064453125, 0.399993896484375, 0.406646728515625, 0.413330078125, 0.41998291015625, 0.4266357421875, 0.433319091796875, 0.439971923828125, 0.4466552734375, 0.45330810546875, 0.46002197265625, 0.4666748046875, 0.47332763671875, 0.47998046875, 0.486663818359375, 0.493316650390625, 0.5, 0.50665283203125, 0.5133056640625, 0.519989013671875, 0.526641845703125, 0.533355712890625, 0.540008544921875, 0.546661376953125, 0.553314208984375, 0.55999755859375, 0.566650390625, 0.573333740234375, 0.579986572265625, 0.586639404296875, 0.59332275390625, 0.5999755859375, 0.606689453125, 0.61334228515625, 0.6199951171875, 0.62664794921875, 0.63330078125, 0.6400146484375, 0.64666748046875, 0.6533203125, 0.65997314453125, 0.666656494140625, 0.673309326171875, 0.680023193359375, 0.686676025390625, 0.69329833984375, 0.699981689453125, 0.706634521484375, 0.713348388671875, 0.720001220703125, 0.726654052734375, 0.733306884765625, 0.739990234375, 0.74664306640625, 0.753326416015625, 0.760009765625, 0.76666259765625, 0.7733154296875, 0.77996826171875, 0.78668212890625, 0.7933349609375, 0.79998779296875, 0.806640625, 0.81329345703125, 0.82000732421875, 0.82666015625, 0.833343505859375, 0.839996337890625, 0.846649169921875, 0.853302001953125, 0.860015869140625, 0.866668701171875, 0.87335205078125, 0.879974365234375, 0.886627197265625, 0.893341064453125, 0.899993896484375, 0.90667724609375, 0.913330078125, 0.91998291015625, 0.9266357421875, 0.933349609375, 0.94000244140625, 0.9466552734375, 0.95330810546875, 0.9599609375, 0.9666748046875, 0.97332763671875, 0.980010986328125, 0.986663818359375, 0.993316650390625, 0.999969482421875, 0.993316650390625, 0.986663818359375, 0.980010986328125, 0.97332763671875, 0.9666748046875, 0.9599609375, 0.95330810546875, 0.9466552734375, 0.94000244140625, 0.933349609375, 0.9266357421875, 0.91998291015625, 0.913330078125, 0.90667724609375, 0.899993896484375, 0.893341064453125, 0.886627197265625, 0.879974365234375, 0.87335205078125, 0.866668701171875, 0.860015869140625, 0.853302001953125, 0.846649169921875, 0.839996337890625, 0.833343505859375, 0.82666015625, 0.82000732421875, 0.81329345703125, 0.806640625, 0.79998779296875, 0.7933349609375, 0.78668212890625, 0.77996826171875, 0.7733154296875, 0.76666259765625, 0.760009765625, 0.753326416015625, 0.74664306640625, 0.739990234375, 0.733306884765625, 0.726654052734375, 0.720001220703125, 0.713348388671875, 0.706634521484375, 0.699981689453125, 0.69329833984375, 0.686676025390625, 0.680023193359375, 0.673309326171875, 0.666656494140625, 0.65997314453125, 0.6533203125, 0.64666748046875, 0.6400146484375, 0.63330078125, 0.62664794921875, 0.6199951171875, 0.61334228515625, 0.606689453125, 0.5999755859375, 0.59332275390625, 0.586639404296875, 0.579986572265625, 0.573333740234375, 0.566650390625, 0.55999755859375, 0.553314208984375, 0.546661376953125, 0.540008544921875, 0.533355712890625, 0.526641845703125, 0.519989013671875, 0.5133056640625, 0.50665283203125, 0.5, 0.493316650390625, 0.486663818359375, 0.47998046875, 0.47332763671875, 0.4666748046875, 0.46002197265625, 0.45330810546875, 0.4466552734375, 0.439971923828125, 0.433319091796875, 0.4266357421875, 0.41998291015625, 0.413330078125, 0.406646728515625, 0.399993896484375, 0.393341064453125, 0.38665771484375, 0.379974365234375, 0.373321533203125, 0.366668701171875, 0.359954833984375, 0.353302001953125, 0.346649169921875, 0.339996337890625, 0.33331298828125, 0.32666015625, 0.32000732421875, 0.313323974609375, 0.306671142578125, 0.29998779296875, 0.2933349609375, 0.28662109375, 0.27996826171875, 0.2733154296875, 0.26666259765625, 0.259979248046875, 0.253326416015625, 0.24664306640625, 0.239990234375, 0.23333740234375, 0.226654052734375, 0.220001220703125, 0.213287353515625, 0.206634521484375, 0.199981689453125, 0.193328857421875, 0.186676025390625, 0.179962158203125, 0.173309326171875, 0.166656494140625, 0.160003662109375, 0.1533203125, 0.14666748046875, 0.13995361328125, 0.13330078125, 0.12664794921875, 0.1199951171875, 0.11334228515625, 0.10662841796875, 0.0999755859375, 0.09332275390625, 0.086669921875, 0.079986572265625, 0.073333740234375, 0.066619873046875, 0.059967041015625, 0.053314208984375, 0.046661376953125, 0.040008544921875, 0.033294677734375, 0.026641845703125, 0.019989013671875, 0.013336181640625, 0.00665283203125, 0.0, -0.00665283203125, -0.013336181640625, -0.019989013671875, -0.026641845703125, -0.033294677734375, -0.040008544921875, -0.046661376953125, -0.053314208984375, -0.059967041015625, -0.066619873046875, -0.073333740234375, -0.079986572265625, -0.086669921875, -0.09332275390625, -0.0999755859375, -0.10662841796875, -0.11334228515625, -0.1199951171875, -0.12664794921875, -0.13330078125, -0.13995361328125, -0.14666748046875, -0.1533203125, -0.160003662109375, -0.166656494140625, -0.173309326171875, -0.179962158203125, -0.186676025390625, -0.193328857421875, -0.199981689453125, -0.206634521484375, -0.213287353515625, -0.220001220703125, -0.226654052734375, -0.23333740234375, -0.239990234375, -0.24664306640625, -0.253326416015625, -0.259979248046875, -0.26666259765625, -0.2733154296875, -0.27996826171875, -0.28662109375, -0.2933349609375, -0.29998779296875, -0.306671142578125, -0.313323974609375, -0.32000732421875, -0.32666015625, -0.33331298828125, -0.339996337890625, -0.346649169921875, -0.353302001953125, -0.359954833984375, -0.366668701171875, -0.373321533203125, -0.379974365234375, -0.38665771484375, -0.393341064453125, -0.399993896484375, -0.406646728515625, -0.413330078125, -0.41998291015625, -0.4266357421875, -0.433319091796875, -0.439971923828125, -0.4466552734375, -0.45330810546875, -0.46002197265625, -0.4666748046875, -0.47332763671875, -0.47998046875, -0.486663818359375, -0.493316650390625, -0.5, -0.50665283203125, -0.5133056640625, -0.519989013671875, -0.526641845703125, -0.533355712890625, -0.540008544921875, -0.546661376953125, -0.553314208984375, -0.55999755859375, -0.566650390625, -0.573333740234375, -0.579986572265625, -0.586639404296875, -0.59332275390625, -0.5999755859375, -0.606689453125, -0.61334228515625, -0.6199951171875, -0.62664794921875, -0.63330078125, -0.6400146484375, -0.64666748046875, -0.6533203125, -0.65997314453125, -0.666656494140625, -0.673309326171875, -0.680023193359375, -0.686676025390625, -0.69329833984375, -0.699981689453125, -0.706634521484375, -0.713348388671875, -0.720001220703125, -0.726654052734375, -0.733306884765625, -0.739990234375, -0.74664306640625, -0.753326416015625, -0.760009765625, -0.76666259765625, -0.7733154296875, -0.77996826171875, -0.78668212890625, -0.7933349609375, -0.79998779296875, -0.806640625, -0.81329345703125, -0.82000732421875, -0.82666015625, -0.833343505859375, -0.839996337890625, -0.846649169921875, -0.853302001953125, -0.860015869140625, -0.866668701171875, -0.87335205078125, -0.879974365234375, -0.886627197265625, -0.893341064453125, -0.899993896484375, -0.90667724609375, -0.913330078125, -0.91998291015625, -0.9266357421875, -0.933349609375, -0.94000244140625, -0.9466552734375, -0.95330810546875, -0.9599609375, -0.9666748046875, -0.97332763671875, -0.980010986328125, -0.986663818359375, -0.993316650390625, -0.999969482421875, -0.993316650390625, -0.986663818359375, -0.980010986328125, -0.97332763671875, -0.9666748046875, -0.9599609375, -0.95330810546875, -0.9466552734375, -0.94000244140625, -0.933349609375, -0.9266357421875, -0.91998291015625, -0.913330078125, -0.90667724609375, -0.899993896484375, -0.893341064453125, -0.886627197265625, -0.879974365234375, -0.87335205078125, -0.866668701171875, -0.860015869140625, -0.853302001953125, -0.846649169921875, -0.839996337890625, -0.833343505859375, -0.82666015625, -0.82000732421875, -0.81329345703125, -0.806640625, -0.79998779296875, -0.7933349609375, -0.78668212890625, -0.77996826171875, -0.7733154296875, -0.76666259765625, -0.760009765625, -0.753326416015625, -0.74664306640625, -0.739990234375, -0.733306884765625, -0.726654052734375, -0.720001220703125, -0.713348388671875, -0.706634521484375, -0.699981689453125, -0.69329833984375, -0.686676025390625, -0.680023193359375, -0.673309326171875, -0.666656494140625, -0.65997314453125, -0.6533203125, -0.64666748046875, -0.6400146484375, -0.63330078125, -0.62664794921875, -0.6199951171875, -0.61334228515625, -0.606689453125, -0.5999755859375, -0.59332275390625, -0.586639404296875, -0.579986572265625, -0.573333740234375, -0.566650390625, -0.55999755859375, -0.553314208984375, -0.546661376953125, -0.540008544921875, -0.533355712890625, -0.526641845703125, -0.519989013671875, -0.5133056640625, -0.50665283203125, -0.5, -0.493316650390625, -0.486663818359375, -0.47998046875, -0.47332763671875, -0.4666748046875, -0.46002197265625, -0.45330810546875, -0.4466552734375, -0.439971923828125, -0.433319091796875, -0.4266357421875, -0.41998291015625, -0.413330078125, -0.406646728515625, -0.399993896484375, -0.393341064453125, -0.38665771484375, -0.379974365234375, -0.373321533203125, -0.366668701171875, -0.359954833984375, -0.353302001953125, -0.346649169921875, -0.339996337890625, -0.33331298828125, -0.32666015625, -0.32000732421875, -0.313323974609375, -0.306671142578125, -0.29998779296875, -0.2933349609375, -0.28662109375, -0.27996826171875, -0.2733154296875, -0.26666259765625, -0.259979248046875, -0.253326416015625, -0.24664306640625, -0.239990234375, -0.23333740234375, -0.226654052734375, -0.220001220703125, -0.213287353515625, -0.206634521484375, -0.199981689453125, -0.193328857421875, -0.186676025390625, -0.179962158203125, -0.173309326171875, -0.166656494140625, -0.160003662109375, -0.1533203125, -0.14666748046875, -0.13995361328125, -0.13330078125, -0.12664794921875, -0.1199951171875, -0.11334228515625, -0.10662841796875, -0.0999755859375, -0.09332275390625, -0.086669921875, -0.079986572265625, -0.073333740234375, -0.066619873046875, -0.059967041015625, -0.053314208984375, -0.046661376953125, -0.040008544921875, -0.033294677734375, -0.026641845703125, -0.019989013671875, -0.013336181640625, -0.00665283203125, 0.0};
  //sp_gen_composite(sp, ft, "0.5 0.5 270 0.5
  
  for(int i = 0; i < ft->size; i++){
      ft->tbl[i] = lewave[i];
  }
  
  sp_osc_init(sp, osc, ft, 0);
  osc->freq = 200;
  osc->amp = 0.5;
  
  sp_osc_init(sp, osc2, ft, 0);
  osc2->freq = 250;
  osc2->amp = 0.3;
  
  // for(int i=0; i<4;i++)
//   {
//     sp_posc3_init(sp, posc3[i], ft);
//     posc3[i]->freq = 200;
//   }

  while (1) {		
    // determine audioReady moment
    // reserve 700ms for initialisation
    if(audioReady==0 && HAL_GetTick()>700) {
      audioReady=1;
    }
    //if (HAL_ADC_PollForConversion(&g_AdcHandle, 1000000) == HAL_OK)
    //        {
        //        g_ADCValue = HAL_ADC_GetValue(&g_AdcHandle);
      //          g_MeasurementNumber++;
               char a[] = "";
                uint32_t digu = (uint32_t)(ADCchannelValues[0]);
              //uint16_t newval = g_ADCValue - (uint16_t)*blsaw->freq;
                      sprintf(a, "%ld", digu);
                                      BSP_LCD_DisplayStringAt(50, 50, (uint8_t *)a, LEFT_MODE);
                                                             osc->freq=(ADCchannelValues[0] / 10);
    // //     }
                                //if(g_ADCValue>1000) {
                                //  *blsaw->freq = g_ADCValue;
                                //}
  }

  return 0;
}


void ConfigureADC()
{
    GPIO_InitTypeDef gpioInit;
 
    // MCU pin for A1 = PF10
    // Function for PF10 = ADC3_IN8
    // 5v compatible port
    __GPIOF_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();
    __ADC3_CLK_ENABLE();
 
    gpioInit.Pin = GPIO_PIN_10|GPIO_PIN_9|GPIO_PIN_8;
    gpioInit.Mode = GPIO_MODE_ANALOG;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpioInit);

    gpioInit.Pin = GPIO_PIN_0;
    gpioInit.Mode = GPIO_MODE_ANALOG;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpioInit);
 
    HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
 
    ADC_ChannelConfTypeDef adcChannel;
 
    g_AdcHandle.Instance = ADC3;
 
    g_AdcHandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
    g_AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
    g_AdcHandle.Init.ScanConvMode = ENABLE;
    g_AdcHandle.Init.ContinuousConvMode = ENABLE;
    g_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    g_AdcHandle.Init.NbrOfDiscConversion = 0;
    g_AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    g_AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    g_AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_AdcHandle.Init.NbrOfConversion = 4;
    g_AdcHandle.Init.DMAContinuousRequests = ENABLE;
    g_AdcHandle.Init.EOCSelection = DISABLE;
 
    HAL_ADC_Init(&g_AdcHandle);
    
    adcChannel.Channel = ADC_CHANNEL_0;
    adcChannel.Rank = 1;
    adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&g_AdcHandle, &adcChannel) != HAL_OK)
    {
      Error_Handler();
    }

      /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
      */
    adcChannel.Channel = ADC_CHANNEL_8;
    adcChannel.Rank = 2;
    if (HAL_ADC_ConfigChannel(&g_AdcHandle, &adcChannel) != HAL_OK)
    {
      Error_Handler();
    }

      /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
      */
    adcChannel.Channel = ADC_CHANNEL_7;
    adcChannel.Rank = 3;
    if (HAL_ADC_ConfigChannel(&g_AdcHandle, &adcChannel) != HAL_OK)
    {
      Error_Handler();
    }

      /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
      */
    adcChannel.Channel = ADC_CHANNEL_6;
    adcChannel.Rank = 4;
    if (HAL_ADC_ConfigChannel(&g_AdcHandle, &adcChannel) != HAL_OK)
    {
      Error_Handler();
    }
    
}

void ConfigureDMA()
{
    __DMA2_CLK_ENABLE(); 
    g_DmaHandle.Instance = DMA2_Stream1;
  
    g_DmaHandle.Init.Channel  = DMA_CHANNEL_2;
    g_DmaHandle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_DmaHandle.Init.PeriphInc = DMA_PINC_DISABLE;
    g_DmaHandle.Init.MemInc = DMA_MINC_ENABLE;
    g_DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_DmaHandle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    g_DmaHandle.Init.Mode = DMA_CIRCULAR;
    g_DmaHandle.Init.Priority = DMA_PRIORITY_HIGH;
    g_DmaHandle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;         
    g_DmaHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    g_DmaHandle.Init.MemBurst = DMA_MBURST_SINGLE;
    g_DmaHandle.Init.PeriphBurst = DMA_PBURST_SINGLE; 
    
    HAL_DMA_Init(&g_DmaHandle);
    
    __HAL_LINKDMA(&g_AdcHandle, DMA_Handle, g_DmaHandle);

    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);   
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
    {
      // uint32_t a2 = 0;
      // for(int i=1000;i<1400;i=i+4) {
      //   a2 = a2 + g_ADCBuffer[i+2];
      // }
      // a2 = a2 / 100;
      
//      if(abs(g_ADCValue-a2)>2) {g_ADCValue = a2;}
      
//      {
        uint32_t channels[4] = {0,0,0,0};
        for(int i=1000;i<1800;i=i+4) {
          channels[0] = channels[0] + g_ADCBuffer[i+0];
          channels[1] = channels[1] + g_ADCBuffer[i+1];
          channels[2] = channels[2] + g_ADCBuffer[i+2];
          channels[3] = channels[3] + g_ADCBuffer[i+3];
        }
      
        channels[0] = channels[0] / 200;
        channels[1] = channels[1] / 200;
        channels[2] = channels[2] / 200;
        channels[3] = channels[3] / 200;

        if(abs(ADCchannelValues[0]-channels[0])>1) {ADCchannelValues[0] = channels[0];}
        if(abs(ADCchannelValues[1]-channels[1])>1) {ADCchannelValues[1] = channels[1];}
        if(abs(ADCchannelValues[2]-channels[2])>1) {ADCchannelValues[2] = channels[2];}
        if(abs(ADCchannelValues[3]-channels[3])>1) {ADCchannelValues[3] = channels[3];}
      
        //g_ADCValue = ADCchannelValues[2];
      
      //g_ADCValue = g_ADCBuffer[2];//std::accumulate(g_ADCBuffer, g_ADCBuffer + ADC_BUFFER_LENGTH, 0) / ADC_BUFFER_LENGTH;
        //g_MeasurementNumber += ADC_BUFFER_LENGTH;
        //if(abs(g_ADCValue - (uint16_t)*blsaw->freq)>5) {
        //  *blsaw->freq = g_ADCValue;
        //}
       // 
    }
 
void DMA2_Stream1_IRQHandler()
{
    HAL_DMA_IRQHandler(&g_DmaHandle);
}

void ADC_IRQHandler()
{
    HAL_ADC_IRQHandler(&g_AdcHandle);
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
  
  BSP_LED_Toggle(LED_GREEN);
}

void computeAudio() {
  if(audioReady==1) {
  
  // compute 2048 samples -> 512 audiosamples
  for(int i = 0; i < 1024; i+=2) {
    SPFLOAT tmp = 0, tmp2=0, tmp3=0, tmp4=0, tmp5=0, tmp6=0;
    
    
    sp_osc_compute(sp, osc, NULL, &tmp);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_osc_compute(sp, osc2, NULL, &tmp3);
    sp_blsaw_compute(sp, blsaw, NULL, &tmp2);

    //sp_posc3_compute(sp, posc3[0], NULL, &tmp2);
    SPFLOAT mixOut = (0.5 * tmp3 + 0.5 * tmp);

    // channel outputs in 2's comp / signed int
    int_bufProcessedOut[i] = (mixOut * 32767);
    int_bufProcessedOut[i+1] = (mixOut * 32767);
  
    }
  }
}

void drawInterface() {
  BSP_LCD_Clear(LCD_COLOR_DARKMAGENTA);

  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_DisplayStringAt(5, 5, (uint8_t *)"stm32f7 and soundpipe", LEFT_MODE);

}