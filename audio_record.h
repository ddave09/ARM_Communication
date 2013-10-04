
#ifndef __RECORD_H
#define __RECORD_H

#include "pdm_filter.h"

/* PDM buffer input size */
#define INTERNAL_BUFF_SIZE      64
#define INTERNAL_BUFF_ARRAY_CNT 16

typedef struct
{
	uint16_t Buffer[INTERNAL_BUFF_SIZE];
	 uint16_t IsReady;
	 uint32_t CurrentPos;
}InternalBufferType;

extern  uint8_t DataReady;
extern  uint32_t InternalBuffCnt;
extern InternalBufferType InternalBuffer[];
extern PDMFilter_InitStruct Filter;
extern uint16_t AUDIO_SAMPLE;
uint32_t WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr);
#endif
