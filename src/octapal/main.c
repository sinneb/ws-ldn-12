/*

octapal
===================

a midi controlled soundmodule

*/

#include <stdio.h>
#include <stdlib.h>

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_sd.h"
#include "stm32f7xx_hal.h"

#include "ff.h"
#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "sd_diskio.h"

#include "ct-gui/gui_stm32.h"
#include "ct-head/random.h"
#include "common/clockconfig.h"

#include "wavfile.h"

uint8_t currentLCDcolor = 0;
const uint32_t LCDColorarray[] = { LCD_COLOR_YELLOW, LCD_COLOR_GREEN, LCD_COLOR_ORANGE, LCD_COLOR_MAGENTA };

#define VOLUME 60
#define SAMPLE_RATE 96000
#define AUDIO_DMA_BUFFER_SIZE 4096
#define AUDIO_DMA_BUFFER_SIZE2 (AUDIO_DMA_BUFFER_SIZE >> 1)
#define AUDIO_DMA_BUFFER_SIZE4 (AUDIO_DMA_BUFFER_SIZE >> 2)
#define AUDIO_DMA_BUFFER_SIZE8 (AUDIO_DMA_BUFFER_SIZE >> 3)

char *audioFilename[4] = {"0:sound.wav", "0:sound2.wav", "0:sound3.wav", "0:sound4.wav"};

// waves
uint8_t sswave[100][50] = {
  {24, 28, 31, 34, 37, 39, 42, 44, 46, 47, 48, 49, 49, 49, 49, 48, 47, 46, 44, 42, 39, 37, 34, 31, 28, 24, 21, 18, 15, 12, 10, 7, 5, 3, 2, 1, 0, 0, 0, 0, 1, 2, 3, 5, 7, 10, 12, 15, 18, 21},
  {34, 49, 46, 45, 44, 41, 40, 38, 34, 34, 30, 26, 26, 23, 22, 21, 21, 20, 20, 19, 19, 18, 17, 17, 17, 17, 17, 17, 17, 17, 29, 29, 28, 27, 27, 27, 25, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17},
  {25, 27, 27, 27, 27, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 26, 26, 26, 27, 33, 24, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 22, 22, 22, 22, 22, 23, 23, 23, 22, 16},
  {25, 25, 25, 26, 26, 27, 27, 28, 29, 30, 30, 31, 32, 33, 34, 34, 34, 33, 33, 33, 33, 32, 32, 32, 32, 31, 31, 31, 30, 22, 7, 0, 2, 10, 12, 14, 15, 16, 17, 18, 19, 19, 20, 21, 21, 22, 23, 23, 24, 24},
  {25, 26, 29, 32, 35, 41, 46, 49, 49, 48, 46, 45, 43, 42, 41, 39, 38, 37, 36, 35, 34, 33, 31, 28, 24, 19, 11, 5, 1, 0, 2, 3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 23},
  {25, 48, 49, 44, 37, 34, 31, 30, 31, 30, 24, 15, 11, 13, 18, 22, 24, 24, 25, 26, 28, 31, 33, 27, 24, 24, 32, 28, 24, 24, 24, 24, 23, 18, 17, 12, 10, 11, 13, 18, 16, 19, 21, 22, 27, 27, 22, 20, 18, 20},
  {25, 33, 36, 36, 36, 37, 38, 49, 49, 47, 40, 34, 31, 29, 29, 29, 26, 17, 12, 11, 15, 20, 22, 21, 17, 15, 14, 15, 17, 17, 13, 11, 13, 17, 16, 17, 22, 28, 32, 34, 32, 30, 26, 23, 20, 18, 16, 12, 14, 19},
  {25, 27, 31, 36, 41, 45, 48, 49, 48, 47, 48, 47, 46, 42, 39, 35, 33, 33, 33, 32, 29, 27, 26, 25, 23, 20, 16, 12, 7, 3, 1, 0, 0, 0, 1, 4, 8, 11, 14, 17, 20, 21, 20, 20, 19, 17, 16, 19, 22, 23},
  {25, 32, 39, 45, 49, 49, 48, 44, 39, 32, 27, 24, 24, 26, 26, 25, 22, 22, 22, 28, 32, 35, 31, 23, 16, 10, 6, 4, 2, 1, 2, 5, 13, 23, 30, 33, 34, 33, 32, 34, 31, 30, 26, 20, 13, 11, 9, 8, 11, 17},
  {24, 27, 30, 32, 33, 35, 37, 38, 40, 41, 42, 43, 43, 44, 44, 45, 44, 44, 42, 42, 40, 39, 37, 35, 32, 29, 26, 23, 20, 17, 14, 11, 8, 6, 3, 2, 0, 0, 0, 0, 1, 2, 3, 5, 8, 11, 13, 16, 19, 22},
  {25, 28, 30, 30, 31, 33, 35, 34, 32, 30, 30, 32, 35, 37, 39, 39, 40, 36, 32, 29, 27, 23, 19, 19, 17, 15, 12, 9, 10, 9, 6, 5, 6, 8, 10, 10, 8, 4, 3, 1, 0, 0, 3, 8, 10, 13, 17, 20, 21, 23},
  {25, 31, 37, 42, 47, 49, 48, 46, 44, 39, 36, 37, 38, 36, 37, 40, 40, 37, 30, 24, 16, 10, 7, 8, 10, 12, 17, 21, 20, 16, 15, 13, 8, 5, 4, 3, 7, 12, 15, 18, 18, 19, 20, 15, 9, 6, 6, 7, 15, 20},
  {24, 25, 26, 28, 22, 22, 19, 23, 25, 24, 25, 31, 33, 30, 29, 29, 26, 22, 18, 17, 14, 11, 10, 12, 13, 14, 12, 12, 10, 7, 0, 0, 0, 0, 3, 6, 12, 12, 15, 19, 17, 18, 18, 19, 18, 21, 20, 21, 21, 22},
  {25, 28, 32, 34, 37, 40, 43, 45, 46, 48, 49, 49, 49, 47, 46, 43, 43, 42, 41, 40, 39, 36, 33, 29, 24, 21, 19, 17, 14, 12, 11, 8, 6, 4, 2, 2, 0, 1, 2, 3, 4, 6, 6, 7, 9, 11, 12, 15, 19, 21},
  {25, 28, 30, 31, 32, 34, 35, 37, 38, 41, 43, 45, 48, 49, 47, 47, 46, 44, 40, 38, 36, 33, 32, 30, 28, 26, 25, 23, 22, 20, 18, 16, 14, 13, 13, 11, 11, 10, 9, 7, 6, 6, 6, 6, 7, 10, 13, 15, 18, 22},
  {25, 28, 31, 37, 40, 42, 45, 46, 47, 48, 49, 48, 48, 48, 46, 44, 43, 39, 37, 36, 35, 33, 33, 30, 28, 26, 21, 16, 14, 11, 6, 4, 3, 2, 1, 0, 0, 1, 2, 2, 4, 7, 9, 11, 14, 14, 15, 18, 18, 20},
  {25, 29, 33, 35, 38, 42, 44, 45, 45, 45, 44, 44, 46, 46, 47, 49, 49, 48, 46, 42, 37, 33, 29, 26, 23, 20, 18, 17, 16, 14, 14, 12, 12, 11, 11, 10, 8, 6, 5, 4, 5, 5, 8, 10, 12, 12, 14, 17, 19, 21},
  {25, 35, 40, 39, 41, 38, 30, 30, 30, 34, 31, 34, 35, 38, 38, 38, 41, 39, 48, 49, 46, 42, 32, 30, 28, 19, 14, 9, 11, 18, 23, 26, 18, 11, 6, 9, 10, 13, 15, 11, 12, 15, 14, 12, 4, 5, 4, 7, 9, 16}
};

char *wavenames[18] = {"akwf1","akwf2","akwf3","akwf4","akwf5","akwf6","akwf7","akwf8","akwf9","akwf10","akwf11","akwf12","akwf13","akwf14","akwf15","akwf16","akwf17","akwf18"};

// audio buffers
static uint8_t int_bufProcessedOut[AUDIO_DMA_BUFFER_SIZE];
static uint8_t audioOutBuf[AUDIO_DMA_BUFFER_SIZE];
static uint8_t audioBufferFile[4][AUDIO_DMA_BUFFER_SIZE];
float f_bufPre_left[4][1024];
float f_bufPost_left[4][1024];
float f_bufPre_right[4][1024];
float f_bufPost_right[4][1024];

uint16_t taptempo[4];
uint8_t taptempocounter;
uint32_t taptempo_prev = 0;
float global_tempo = 100;

uint32_t prevTick = 0;

uint16_t *varToUpdate;

float f_bufPost_mixdown_left[1024];
float f_bufPost_mixdown_right[1024];

float voice_frequency[4] = {0,0,0,0};

float f_ssample[4][600];
uint8_t active_ssample = 1;
uint8_t receiving_note_on = 0;
// max 32 cycle = 32*600 = 19200
float f_ssample_freq_specific[19200];
float f_ssample_outChannel[4][600];
float f_ssample_outChannel_Volume[4] = {0.6, 0.6, 0.6, 0.6};
float f_ssample_temp[600];
float f_ssample_right[600];
uint16_t ssample_lastpos = 0;
uint16_t ssample_small_lastpos[4] = {0,0,0,0};
uint16_t ssample_samplespertransfer[4] = {300,350,400,450};
int16_t ssamples_remaining_idx[4] = {600,600,600,600};

short resleft[512];
short resright[512];

extern HCD_HandleTypeDef hhcd;
extern USBH_HandleTypeDef hUSBH;
extern SAI_HandleTypeDef haudio_out_sai;

UART_HandleTypeDef uart_config;

uint8_t midicounter=0;
uint16_t mididata[10];
uint8_t rx_byte[1];

uint16_t vco1wave = 0;
uint16_t vco2wave = 1;
uint16_t vco3wave = 2;

FIL audio_file[4];
FIL ssample[4];
UINT bytes_read[4];
UINT bytes_read2;
UINT bytes_read3;
UINT bytes_read4;
UINT prev_bytes_read;
FATFS fs;
FATFS SDFatFs;

static char usb_drive_path[4];

static TS_StateTypeDef rawTouchState;

uint16_t runOnce = 0;

char *touchMap = "main";

void drawSSample(uint16_t sampleID, uint16_t xstart, uint16_t ystart);
void UART6_Config();
static void init_after_USB();
void drawInterface();
void drawWaveSelector();
static void initAudio();
void computeAudio();
void computeVoice(int16_t freq, uint8_t voiceID);
void computeOscillatorOut(uint8_t ssampleBufferID, uint8_t channelID, uint16_t samplesPerTransfer);
void inter1parray( float aaaa[], int n, float bbbb[], int m );
void interp2array( float aaaa[], int n, float bbbb[], int m );
void interp5( float aaaa[], int n, float bbbbb[], int m );

int main() {
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config(); 
  
  // config UART for MIDI communication
  UART6_Config();
  
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Off(LED_GREEN);
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
  
  // Init LCD and Touchscreen
  BSP_LCD_Init();
  if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_OK) {
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    // BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));
    // BSP_LCD_SelectLayer(1);
    // BSP_LCD_Clear(LCD_COLOR_TRANSPARENT);
    // BSP_LCD_SetTransparency(1, 100);
    // BSP_LCD_SelectLayer(0);
    // BSP_LCD_Clear(LCD_COLOR_BLACK);
    // BSP_LCD_SetLayerVisible(0,ENABLE);
    // BSP_LCD_SetLayerVisible(1,DISABLE);
	  BSP_TS_ITConfig();
  }  

  // Load SD Driver & mount card
  if (FATFS_LinkDriver(&SD_Driver, usb_drive_path) != 0) {
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)"SD Driver error", CENTER_MODE);
    Error_Handler();
  }
	if(f_mount(&SDFatFs, (TCHAR const*)usb_drive_path, 0) != FR_OK) {
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)"SD Mount error", CENTER_MODE);
		Error_Handler();
	}
		
  init_after_USB();
  
  drawInterface();
  
  while (1) {		
	  //nothing is done here	
  }

  return 0;
}

// MIDI HANDLING --------------------------------------------

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  BSP_LED_Toggle(LED_GREEN);

  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                          (uint8_t *)"MIDI in", CENTER_MODE);
    
  if(midicounter==2) {
    mididata[midicounter]=rx_byte[0];
    char a[] = "";
    sprintf(a, "%d %d %d", mididata[0],mididata[1],mididata[2]);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 + 20, (uint8_t *)a, CENTER_MODE);
    midicounter=0;
  }
  
  if(midicounter==1) {
    mididata[midicounter]=rx_byte[0];
    midicounter++;
  }
                            
  // received 144: start receiving note_on (3 bytes)
  // e.g. 144 56 40
  if(rx_byte[0]==144 && midicounter==0) {
    //receiving_note_on = 1;
    mididata[midicounter]=rx_byte[0];
    midicounter++;
  }  

  // reset interrupt
  HAL_UART_Receive_IT(&uart_config, rx_byte, 1);
}



static void init_after_USB() {
  // open samples
  for (int j=0; j < 4; j++) {
    if (f_open(&audio_file[j], audioFilename[j], FA_READ) == FR_OK) {
		// f_open ok
      f_lseek(&audio_file[j], f_size(&audio_file[j]));
    } else {
        BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                                (uint8_t *)"Load error", CENTER_MODE);
      Error_Handler();
    }
  }
  
  // open single waveform
  // skip 44 (offset)
  // read 1200 (samplelength = 600 2's complement)
  if (f_open(&ssample[0], "0:single1.wav", FA_READ) == FR_OK) {
	// f_open ok
    f_lseek(&ssample[0], 44);
		//char a[] = "";
		//sprintf(a, "%ld", f_size(&ssample));
    //BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)a, CENTER_MODE);
  } else {
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                              (uint8_t *)"Load error ssample", CENTER_MODE);
    Error_Handler();
  }
  
	// read chunks from USB
  // single use of audiobuffer 0, will be reused in context later on
  // single use of bytes_read 0
	f_read(&ssample[0], audioBufferFile[0], 1200, &bytes_read[0]);

	// 2's-complement signed integers -> short (-32k -> +32k) -> float (-1 -> +1)
	for (int j=0; j < 1200; j=j+2) {
	
		// to short
    // read 2 values for 2's-complement
    // convert to 1 short value
		short tempshort = (short)((audioBufferFile[0][j+1])<<8 | ((audioBufferFile[0][j]) & 0xFF));
	
		// to float
    // use j/2 because of conversion from 2's comp to short
		f_ssample[0][j/2] = ((float)tempshort/32768);
  }
  
  // 2
  if (f_open(&ssample[1], "0:single2.wav", FA_READ) == FR_OK) {
	  f_lseek(&ssample[1], 44);
	} else {
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                              (uint8_t *)"Load error ssample 2", CENTER_MODE);
    Error_Handler();
  }
  f_read(&ssample[1], audioBufferFile[0], 1200, &bytes_read[0]);
	for (int j=0; j < 1200; j=j+2) {
		short tempshort = (short)((audioBufferFile[0][j+1])<<8 | ((audioBufferFile[0][j]) & 0xFF));
		f_ssample[1][j/2] = ((float)tempshort/32768);
  }
  
  // 3
  if (f_open(&ssample[2], "0:single3.wav", FA_READ) == FR_OK) {
	  f_lseek(&ssample[2], 44);
	} else {
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                              (uint8_t *)"Load error ssample 3", CENTER_MODE);
    Error_Handler();
  }
  f_read(&ssample[2], audioBufferFile[0], 1200, &bytes_read[0]);
	for (int j=0; j < 1200; j=j+2) {
		short tempshort = (short)((audioBufferFile[0][j+1])<<8 | ((audioBufferFile[0][j]) & 0xFF));
		f_ssample[2][j/2] = ((float)tempshort/32768);
  }
  
  
  // 4
  if (f_open(&ssample[3], "0:single4.wav", FA_READ) == FR_OK) {
	  f_lseek(&ssample[3], 44);
	} else {
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8,
                              (uint8_t *)"Load error ssample 4", CENTER_MODE);
    Error_Handler();
  }
  f_read(&ssample[3], audioBufferFile[0], 1200, &bytes_read[0]);
	for (int j=0; j < 1200; j=j+2) {
		short tempshort = (short)((audioBufferFile[0][j+1])<<8 | ((audioBufferFile[0][j]) & 0xFF));
		f_ssample[3][j/2] = ((float)tempshort/32768);
  }
  
  initAudio();
}

static void initAudio() {
    if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, VOLUME, SAMPLE_RATE) != 0) {
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
//  uint32_t tikker = HAL_GetTick();
	computeAudio();
    // char a[] = "";
    // sprintf(a, "%ld", tikker);
    // BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)a, CENTER_MODE);
	memcpy(audioOutBuf, int_bufProcessedOut, AUDIO_DMA_BUFFER_SIZE2);  
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
  memset(int_bufProcessedOut, 0, sizeof int_bufProcessedOut);
	computeAudio();
	memcpy(&audioOutBuf[AUDIO_DMA_BUFFER_SIZE2], int_bufProcessedOut, AUDIO_DMA_BUFFER_SIZE2);
}

void computeAudio() {
  
  for(int k=0; k<4; k++){
    if (voice_frequency[k]!=0) {
      // play voice
      computeVoice(voice_frequency[k], k);
    }
  }
  //computeVoice(440);
  
	// the mix
	// float to short
	for (int k=0; k<2048; k=k+4) {
    
    // mix
    /*
    f_bufPost_mixdown_left[k/4] = (0.8 * f_bufPost_left[0][k/4])
                          + (0.8 * f_bufPost_left[1][k/4])
                          + (0.8 * f_bufPost_left[2][k/4])
                          + (0.8 * f_bufPost_left[3][k/4])
                            + (0.8 * f_ssample_outChannel[0][k/4]);
    f_bufPost_mixdown_right[k/4] = (0.8 * f_bufPost_right[0][k/4])
                          + (0.8 * f_bufPost_right[1][k/4])
                          + (0.8 * f_bufPost_right[2][k/4])
                          + (0.8 * f_bufPost_right[3][k/4]);
    */
    
    f_bufPost_mixdown_left[k/4] =   (f_ssample_outChannel_Volume[0] * f_ssample_outChannel[0][k/4])
                                    + (f_ssample_outChannel_Volume[1] * f_ssample_outChannel[1][k/4])
                                    + (f_ssample_outChannel_Volume[2] * f_ssample_outChannel[2][k/4])
                                    + (f_ssample_outChannel_Volume[3] * f_ssample_outChannel[3][k/4]);
    
		// to short
		resleft[k/4] = (short)(f_bufPost_mixdown_left[k/4] * 32768);
		resright[k/4] = (short)(f_bufPost_mixdown_right[k/4] * 32768);
		// to 2's-comp
		int_bufProcessedOut[k+1] = resleft[k/4]>>8;
		int_bufProcessedOut[k] = resleft[k/4]&0xff;
		int_bufProcessedOut[k+3] = resright[k/4]>>8;
		int_bufProcessedOut[k+2] = resright[k/4]&0xff;
		
	}
}

void computeVoice(int16_t freq, uint8_t voiceID) {
  //compute oscillator outputs for note
  // required frequency * 3.2 outputs samples per Transfer
  
  computeOscillatorOut(voiceID, voiceID, freq); // 109.4 Hz
  //computeOscillatorOut(1,1, 4000); //1250 Hz
  //computeOscillatorOut(2,2, 449); // 140.3 Hz
  //computeOscillatorOut(3,3, 1408); // 1562.1 Hz
}


// ssampleBufferID = last used sample from main ssample, per oscillator
// channelID = f_ssample_outChannel per oscillator
// samplesPerTranfer = nbr of samples used from main ssample, per processing block
void computeOscillatorOut(uint8_t ssampleBufferID, uint8_t channelID, uint16_t samplesPerTransfer) {
  // clear array
  // set index to 0
  uint16_t ssamples_current_index = 0;
  memset(f_ssample_freq_specific, 0, sizeof f_ssample_freq_specific);
  
  // smaller ssample than 600 samples
  // take part of ssample and interpolate to ssamplesamplespertransfer
  if (samplesPerTransfer < 600) {
    
    if ((600 - ssample_small_lastpos[ssampleBufferID]) > samplesPerTransfer) {  
      // enough samples, take part of ssample
      memcpy(&f_ssample_freq_specific[0], 
             &f_ssample[ssampleBufferID][ssample_small_lastpos[ssampleBufferID]], 
             samplesPerTransfer * 4);
    } else {
      // copy rest
      memcpy(&f_ssample_freq_specific[0], 
             &f_ssample[ssampleBufferID][ssample_small_lastpos[ssampleBufferID]], 
             (600 - ssample_small_lastpos[ssampleBufferID]) * 4); 
      // start new sample, copy rest from that sample, set lastpos
      memcpy(&f_ssample_freq_specific[600 - ssample_small_lastpos[ssampleBufferID]], 
             &f_ssample[ssampleBufferID][0], 
             (ssample_small_lastpos[ssampleBufferID]) * 4);
    }
    
    // interpolate to ssamplesamplespertransfer
    inter1parray( f_ssample_freq_specific, samplesPerTransfer, f_ssample_temp, 600 );
    
    // and interpolate to 512 and send to mixer
    inter1parray( f_ssample_temp, 600, f_ssample_outChannel[channelID], 512 );
    // set ssample_small_lastpos[ssampleBufferID] to lastpos
    ssample_small_lastpos[ssampleBufferID] += samplesPerTransfer; 
    // set overflow
    if (ssample_small_lastpos[ssampleBufferID] > 600) {ssample_small_lastpos[ssampleBufferID] -= 600;}
    
  } else {
  
    // first place remaining samples from last run in f_ssample_freq_specific
    // start copy from ssample_pre_left at idx from last run
    // length = 600 - remaining samples index
    uint16_t ssamples_remaining_length = 600-ssamples_remaining_idx[ssampleBufferID];
    memcpy(&f_ssample_freq_specific[ssamples_current_index], 
           &f_ssample[ssampleBufferID][ssamples_remaining_idx[ssampleBufferID]], 
           ssamples_remaining_length * 4);
    // move index to length
    ssamples_current_index += ssamples_remaining_length;
  
    // build new waveform in f_ssample_freq_specific from ssample which is 600 samples long
    // length of f_ssample_freq_specific is in samplespertransfer
    // determine number of complete waveforms
    // subtract already placed samples from samplespertransfer
    // divide int trough int results in a ceiling rounding
    for(int i=0; i<((samplesPerTransfer - ssamples_remaining_length) / 600); i++) {
      memcpy(&f_ssample_freq_specific[ssamples_current_index], 
             &f_ssample[ssampleBufferID][0], 
             600 * 4);
      ssamples_current_index += 600;
    }
  
    // determine remaining positions
    uint16_t remaining_pos = (samplesPerTransfer - ssamples_current_index);
    // and copy (part of) new ssample into that positions
    memcpy(&f_ssample_freq_specific[ssamples_current_index], 
           &f_ssample[ssampleBufferID][0], 
           remaining_pos * 4); 
  
    // set remaining samples from ssample
    ssamples_remaining_idx[ssampleBufferID] = remaining_pos;
  
    // interpolate to 512 samples
    inter1parray( f_ssample_freq_specific, samplesPerTransfer, f_ssample_outChannel[channelID], 512 ); 
  }
}

void OTG_FS_IRQHandler(void) {
  HAL_HCD_IRQHandler(&hhcd);
}

// Interrupt handler shared between:
// SD_DETECT pin, USER_KEY button and touch screen interrupt
void EXTI15_10_IRQHandler(void) {
  if (__HAL_GPIO_EXTI_GET_IT(SD_DETECT_PIN) != RESET) {
    HAL_GPIO_EXTI_IRQHandler(SD_DETECT_PIN | TS_INT_PIN);
  } else {
    // User button event or Touch screen interrupt
    HAL_GPIO_EXTI_IRQHandler(KEY_BUTTON_PIN);
  }
}

void drawButton(uint16_t x, uint16_t y, uint8_t * textstring) {
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(x,y,110,100);
  BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
  BSP_LCD_FillRect(x+5,y+5,100,90);
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_ORANGE);
	BSP_LCD_DisplayStringAt(x+45, y+40, textstring, LEFT_MODE);
}

void drawInterface() {
  BSP_LCD_Clear(LCD_COLOR_BLUE);
  
  BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
  BSP_LCD_FillRect(0,0,480,24);
  
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_ORANGE);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(5, 5, (uint8_t *)"OCTAPAL", LEFT_MODE);
  
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(5, 60, (uint8_t *)"vco1", LEFT_MODE);
  BSP_LCD_DisplayStringAt(5, 140, (uint8_t *)"vco2", LEFT_MODE);
  BSP_LCD_DisplayStringAt(5, 220, (uint8_t *)"vco3", LEFT_MODE);
  
  drawSSample(vco1wave,40,35);
  drawSSample(vco2wave,40,115);
  drawSSample(vco3wave,40,195);
  
  //BSP_LCD_DrawPolygon(wave1,50);
    
  //drawButton(10,29,(uint8_t *)"1");
  //drawButton(125,29,(uint8_t *)"2");
  //drawButton(240,29,(uint8_t *)"3");
  //drawButton(355,29,(uint8_t *)"4");
}

void drawWaveSelector() {  
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_FillRect(10,10,460,257);
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
  BSP_LCD_FillRect(12,12,456,253);
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_FillRect(12,12,456,20);
  
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_RED);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(17, 17, (uint8_t *)"Select wave", LEFT_MODE);
  
  // 18 waves available sofar
  int8_t sid = 0;
  for (int row=0; row < 3; row++) {
    for (int col=0; col < 6; col++) {
      drawSSample(sid, 20 + col * 70 , 40 + 75 * row );
      sid++;
    }
  }
}

void drawSSample(uint16_t sampleID, uint16_t xstart, uint16_t ystart) {
  
  BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
  BSP_LCD_FillRect(xstart,ystart,60,60);
  
  BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
  BSP_LCD_DrawLine(xstart,ystart+30,xstart+59,ystart+30);
  
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  uint16_t y = ystart + 50 - sswave[sampleID][0];
  uint16_t x = xstart + 5;
  for (int j=1; j < 50; j++) {
    BSP_LCD_DrawLine(x,y,x+1,ystart + 5 + 50 - sswave[sampleID][j]);
    x++;
    y = ystart + 5 + 50 - sswave[sampleID][j];
  }
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(xstart, ystart+60, (uint8_t *)wavenames[sampleID], LEFT_MODE);
}

// void HAL_GPIO_EXTI_IRQHandler points to this:
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// runonce to run this function only once when IRQ fires (fires continuesly)
	runOnce = 0;
  
  //drawInterface();
		
	BSP_TS_GetState(&rawTouchState);
	while (rawTouchState.touchDetected) {
		// run once on (continues) touch
		if (runOnce == 0) {
			
			uint16_t touchx = rawTouchState.touchX[0];
      uint16_t touchy = rawTouchState.touchY[0];
      
      // determine and handle current touchMap
      if (strcmp(touchMap,"main")==0) {
        // vco1 select
        if(touchx > 40 && touchx < 100 && touchy > 35 && touchy < 95) {
          touchMap = "waveselect";
          varToUpdate = &vco1wave;
          drawWaveSelector();
        }
        
        if(touchx > 40 && touchx < 100 && touchy > 115 && touchy < 175) {
          touchMap = "waveselect";
          varToUpdate = &vco2wave;
          drawWaveSelector();
        }
        
        if(touchx > 40 && touchx < 100 && touchy > 195 && touchy < 255) {
          touchMap = "waveselect";
          varToUpdate = &vco3wave;
          drawWaveSelector();
        }
      }
        
      else if (strcmp(touchMap,"waveselect")==0) {
        
        int8_t sid = 0;
        for (int row=0; row < 3; row++) {
          for (int col=0; col < 6; col++) {
            if(touchx > (20 + col * 70) && touchx < (80 + col * 70) && touchy > (40 + 75 * row) && touchy < (100 + 75 * row)) {
              *varToUpdate = sid;
            }
            sid++;
          }
        }
        
        touchMap = "main";
        drawInterface();
      }
        
        // wave select
        //if(touchx > 20 && touchx < 100 && touchy > 35 && touchy < 95) {
          // update pointed to variable (vco1wave, vco2wave or vco3wave)
          // with new waveid
        //  *varToUpdate = 4;
        //  touchMap = "main";
        //  drawInterface();
       // }
        
      //}
			
      /*
			if(touchy > 115) {
			
				int16_t sum = 0;
				for(int i=0; i<4; i++) {
				    sum+=taptempo[i];
				}
				global_tempo = (float)60000 / ((float)sum/4);
			
				uint32_t tikker = HAL_GetTick();
				taptempo[taptempocounter] = tikker - taptempo_prev;
				taptempocounter++;
				if(taptempocounter>4) taptempocounter=0;
				taptempo_prev = tikker;
			
				BSP_LCD_SetBackColor(LCDColorarray[currentLCDcolor]);
			
				currentLCDcolor++;
				if(currentLCDcolor>2) currentLCDcolor=0;			
			
				char a[] = "";
				//sprintf(a, "%ld", tikker - taptempo_prev);
				snprintf(a, 6, "%f",global_tempo);
				BSP_LCD_SetFont(&Font24);
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *)a, CENTER_MODE);
			}
			
			else {
        // restart audio based on touch positions
        if(touchx > 0 && touchx < 120) {
          voice_frequency[0] = 1408;
          //f_lseek(&audio_file[0], 0);
          //f_ssample_outChannel_Volume[0] += 0.1;
          //  if(f_ssample_outChannel_Volume[0] > 0.6) {f_ssample_outChannel_Volume[0]=0;}
        }
        if(touchx > 120 && touchx < 240) {
          voice_frequency[0] = 700;
          //f_lseek(&audio_file[1], 0);
          //f_ssample_outChannel_Volume[1] += 0.1;
          //  if(f_ssample_outChannel_Volume[1] > 0.6) {f_ssample_outChannel_Volume[1]=0;}
        }
        if(touchx > 240 && touchx < 360) {
          f_ssample_outChannel_Volume[2] += 0.1;
            if(f_ssample_outChannel_Volume[2] > 0.6) {f_ssample_outChannel_Volume[2]=0;}
        }
        if(touchx > 360 && touchx < 480) {
          f_ssample_outChannel_Volume[3] += 0.1;
            if(f_ssample_outChannel_Volume[3] > 0.6) {f_ssample_outChannel_Volume[3]=0;}
        }
        //} else {
        //  f_lseek(&audio_file[1], 0);
        //}

      }*/
			
			runOnce = 1;
		}
	// read state and continue with while
	BSP_TS_GetState(&rawTouchState);
	} // end while
}





void UART6_Config() {
  
//************  UART CONFIG  *****************************//

  __USART6_CLK_ENABLE();

  uart_config.Instance=USART6;

  uart_config.Init.BaudRate=31250;
  uart_config.Init.WordLength=UART_WORDLENGTH_8B;
  uart_config.Init.StopBits=UART_STOPBITS_1;
  uart_config.Init.Parity=UART_PARITY_NONE;
  uart_config.Init.Mode=UART_MODE_TX_RX;
  uart_config.Init.HwFlowCtl=UART_HWCONTROL_NONE;
  
  HAL_UART_Init(&uart_config);

  HAL_NVIC_SetPriority(USART6_IRQn,0,1);
  HAL_NVIC_EnableIRQ(USART6_IRQn);

//**********************************************************//

//************ UART GPIO CONFIG  *********************//

  GPIO_InitTypeDef uart_gpio;

  __GPIOC_CLK_ENABLE();

  uart_gpio.Pin=GPIO_PIN_7;
  uart_gpio.Mode=GPIO_MODE_AF_PP;
  uart_gpio.Pull=GPIO_NOPULL;
  uart_gpio.Speed=GPIO_SPEED_FAST;
  uart_gpio.Alternate=GPIO_AF8_USART6;

  HAL_GPIO_Init(GPIOC, &uart_gpio);
  HAL_UART_Receive_IT(&uart_config, rx_byte, 1);
}

void USART6_IRQHandler(void)
{
  HAL_UART_IRQHandler(&uart_config);
}
