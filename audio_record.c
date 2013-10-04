

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio_codec.h"
#include "AudioRecord.h"
#include "pdm_filter.h"

/*PCM buffer output size */
#define PCM_OUT_SIZE            16
#define PLAY_BUF_SIZE 			32768	


uint32_t userButtonStatusFlag = 1;  /*checks for user button press event */
uint16_t volume_pdm = 80;			/*volume*/
uint32_t templaybuf = 1404;
uint16_t pAudioRecBuf[PCM_OUT_SIZE];
uint16_t pAudioPlayBuf[PLAY_BUF_SIZE];
__IO uint32_t AudioPlayBufPos = 0;

__IO uint32_t PlayFlag = 1;
int count =0;			/* temporary variable to check */
int main()
{
	InternalBufferType *pBuff;
	/**
		@brief: INITIALIZATION FUNCTION
		@parameter: Audio frequency
					bits per sample
					number of channels
		@ retval : returns 0 on successs
	**/
	WaveRecorderInit(I2S_AudioFreq_48k,16, 1);
	

	
	/**
		brief: Enables i2s communication when user button is pressed. which enables microphone.
				otherwise it is ready to play received data.
	**/
 
	while(1)
	{
		
		userButtonStatusFlag = STM_EVAL_PBGetState(BUTTON_USER);
		if(userButtonStatusFlag){
				inMain++;
				
				SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
				I2S_Cmd(SPI2, ENABLE);
				pBuff = &(InternalBuffer[InternalBuffCnt]);
				while(pBuff->IsReady == 0);
				PDM_Filter_64_LSB(
						(uint8_t*)(pBuff->Buffer),
						pAudioRecBuf,
						volume_pdm,
						&Filter);
				pBuff->IsReady = 0;
		}
		if(!userButtonStatusFlag){
				I2S_Cmd(SPI2,DISABLE);
				playCheck = EVAL_AUDIO_Play(&AUDIO_SAMPLE,templaybuf);
				
		}
			
	}
}

/** @ function required when DMA buffer is full **/
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t size)
{

}

/* Over ridden functions of stm32f4xx_audio_codec.h */
uint16_t EVAL_AUDIO_GetSampleCallBack(void)
{
  return 0;
}

uint32_t Codec_TIMEOUT_UserCallback(void)
{   
  return (0);
}

uint32_t LIS302DL_TIMEOUT_UserCallback(void)
{
  /* MEMS Accelerometer Timeout error occured */
  while (1)
  {   
  }
}

void EVAL_AUDIO_Error_CallBack(void* pData)
{
  /* Stop the program with an infinite loop */
  while (1)
  {}
}
