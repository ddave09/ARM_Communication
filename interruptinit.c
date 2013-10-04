
#include "stm32f4_discovery.h"
#include "AudioRecord.h"
#include "stm32f4_discovery_audio_codec.h"
#include "pdm_filter.h"

#define SPI_SCK_PIN                       GPIO_Pin_10
#define SPI_SCK_GPIO_PORT                 GPIOB
#define SPI_SCK_GPIO_CLK                  RCC_AHB1Periph_GPIOB
#define SPI_SCK_SOURCE                    GPIO_PinSource10
#define SPI_SCK_AF                        GPIO_AF_SPI2

#define SPI_MOSI_PIN                      GPIO_Pin_3
#define SPI_MOSI_GPIO_PORT                GPIOC
#define SPI_MOSI_GPIO_CLK                 RCC_AHB1Periph_GPIOC
#define SPI_MOSI_SOURCE                   GPIO_PinSource3
#define SPI_MOSI_AF                       GPIO_AF_SPI2

/* GPIO INIT CALL CHECK & SPI INIT CALL CHECK */
int GPIO_FLAG = 0;
int SPI_FLAG = 0;
int inInterrupt = 0;
uint8_t volume = 100;
/* Audio recording Samples format (from 8 to 16 bits) */
uint32_t AudioRecBitRes = 16; 
/* Audio recording number of channels (1 for Mono or 2 for Stereo) */
uint32_t AudioRecChnlNbr = 1;
/* Current state of the audio recorder interface intialization */
static uint32_t AudioRecInited = 0;
int not_reset = 0 ;									/* variable to check interrupt generation */
PDMFilter_InitStruct Filter;

/* Temporary data sample */
InternalBufferType InternalBuffer[INTERNAL_BUFF_ARRAY_CNT];
uint32_t InternalBuffCnt = 0;



static void WaveRecorder_GPIO_Init(void);
static void WaveRecorder_SPI_Init(uint32_t Freq);
static void WaveRecorder_NVIC_Init(void);
static void AudioInit(void);
/**
  * @brief  This function handles AUDIO_REC_SPI global interrupt request.
  * @param  None
  * @retval None
*/
void SPI2_IRQHandler(void)
{  
	
  uint16_t app;
	InternalBufferType *pBuff = &(InternalBuffer[InternalBuffCnt]);
	inInterrupt ++;
  /* Check if data are available in SPI Data register */
  if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET)
  {
		not_reset++;								/*microphone is fetching data */
    app = SPI_I2S_ReceiveData(SPI2);
    pBuff->Buffer[pBuff->CurrentPos++] = HTONS(app);
    
    /* Check to prevent overflow condition */
    if (pBuff->CurrentPos >= INTERNAL_BUFF_SIZE)
    {
      pBuff->IsReady = 1;
			pBuff->CurrentPos = 0;
			InternalBuffCnt++;
			if(InternalBuffCnt >= INTERNAL_BUFF_ARRAY_CNT)
				InternalBuffCnt = 0;
    }
  }
}

/**
  * @brief  Initialize wave recording
  * @param  AudioFreq: Sampling frequency
  *         BitRes: Audio recording Samples format (from 8 to 16 bits)
  *         ChnlNbr: Number of input microphone channel
  * @retval None
  */
uint32_t WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr)
{ 
  /* Check if the interface is already initialized */
  if (AudioRecInited)
  {
    /* No need for initialization */
    return 0;
  }
  else
  {
    /* Enable CRC module */
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    
    /* Filter LP & HP Init */
    Filter.LP_HZ = 8000;
    Filter.HP_HZ = 10;
    Filter.Fs = 16000;
    Filter.Out_MicChannels = 1;
    Filter.In_MicChannels = 1;
    
    PDM_Filter_Init((PDMFilter_InitStruct *)&Filter);
    
    /* Configure the GPIOs */
    WaveRecorder_GPIO_Init();
    
    /* Configure the interrupts (for timer) */
    WaveRecorder_NVIC_Init();
    
    /* Configure the SPI */
    WaveRecorder_SPI_Init(AudioFreq);
    
		/* Configure the audio coded */
		AudioInit();
    /* Set the local parameters */
    AudioRecBitRes = BitRes;
    AudioRecChnlNbr = ChnlNbr;
//		PlayBufCnt = 0;
    
    /* Return 0 if all operations are OK */
    return 0;
  }  
}

/**
  * @brief  Initialize GPIO for wave recorder.
  * @param  None
  * @retval None
  */
static void WaveRecorder_GPIO_Init(void)
{  
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
  GPIO_Init(SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
  
  /* Connect SPI pins to AF5 */  
  GPIO_PinAFConfig(SPI_SCK_GPIO_PORT, SPI_SCK_SOURCE, SPI_SCK_AF);
  
  /* SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  SPI_MOSI_PIN;
  GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF);
	
	/* CONFIGURES USER BUTTON */
	STM_EVAL_PBInit(BUTTON_USER,BUTTON_MODE_GPIO);
	GPIO_FLAG = 1;
}

/**
  * @brief  Initialize SPI peripheral.
  * @param  Freq :Audio frequency
  * @retval None
  */
static void WaveRecorder_SPI_Init(uint32_t Freq)
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the SPI clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
  
  /* SPI configuration */
  SPI_I2S_DeInit(SPI2);
  I2S_InitStructure.I2S_AudioFreq = Freq;
  I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(SPI2, &I2S_InitStructure);

  /* Enable the Rx buffer not empty interrupt */
  SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
  SPI_FLAG = 1;
}


/**
  * @brief  Initialize the NVIC.
  * @param  None
  * @retval None
  */
static void WaveRecorder_NVIC_Init(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3); 
  /* Configure the SPI interrupt priority */
  NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

static void AudioInit(void){
	EVAL_AUDIO_SetAudioInterface(AUDIO_INTERFACE_I2S);
	EVAL_AUDIO_Init(OUTPUT_DEVICE_SPEAKER, volume, I2S_AudioFreq_48k);
}

