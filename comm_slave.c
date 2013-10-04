#include "stm32f4_discovery.h"
#include "stm32f4xx_i2c.h"


#define MASTER_ADDRESS 0x1F
#define SLAVE_ADDRESS 0x3F
#define I2CFORDATATX I2C2

int inHandler = 0;
int inClearAdd = 0;
int inReceive = 0;
int inStop = 0;
int inWhile = 0;
uint8_t buttonPressedForMe = 0;
//static uint8_t canReceieve = 0;
//uint16_t* internalBufferHead;
uint16_t temp;

void handle_event(void);
int init_comm(void);
int I2C_starter(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction);
int send_data_i2c(I2C_TypeDef* I2Cx,uint8_t data);
int I2C_stopper(I2C_TypeDef* I2Cx);
void blink(void);

int main(){

  STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);
	
	init_comm();
	while(1){
	inWhile++;
	STM_EVAL_LEDToggle(LED6);
	}
}


int init_comm(){
	//get typeDefs for GPIO and I2C
	I2C_InitTypeDef I2CComm;
	GPIO_InitTypeDef GPIOComm;
	NVIC_InitTypeDef NVICInit;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE); //Enable I2C2 clock.
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

 	GPIOComm.GPIO_Pin  = GPIO_Pin_10 | GPIO_Pin_11;
	GPIOComm.GPIO_Mode = GPIO_Mode_AF ;
	GPIOComm.GPIO_Speed = GPIO_Speed_50MHz;
	GPIOComm.GPIO_OType = GPIO_OType_OD;
	GPIOComm.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_I2C2);
  GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_I2C2);
	GPIO_Init(GPIOB,&GPIOComm);
	
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	NVICInit.NVIC_IRQChannel = I2C2_EV_IRQn;
    NVICInit.NVIC_IRQChannelCmd = ENABLE;
    NVICInit.NVIC_IRQChannelPreemptionPriority = 2;
    NVICInit.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVICInit);	
	
	
	I2CComm.I2C_ClockSpeed = 100000;
	I2CComm.I2C_Mode = I2C_Mode_I2C;
	I2CComm.I2C_DutyCycle = I2C_DutyCycle_16_9;
	I2CComm.I2C_OwnAddress1 = SLAVE_ADDRESS; //own address = MASTER_ADDRESS, to be happy and safe.
	I2CComm.I2C_Ack = I2C_Ack_Enable;
	I2CComm.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_AcknowledgeConfig(I2C2, ENABLE);
	I2C_ITConfig(I2C2,I2C_IT_EVT|I2C_IT_BUF|I2C_IT_ERR,ENABLE);
    I2C_Init(I2C2,&I2CComm);	
	I2C_Cmd(I2C2,ENABLE);
	

STM_EVAL_LEDOn(LED4);

return 1;
}




int I2C_starter(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction){

  //Generate Start Condition for sending bit
  I2C_GenerateSTART(I2Cx,ENABLE);
  while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)){STM_EVAL_LEDToggle(LED5);}
  
  I2C_Send7bitAddress(I2Cx,address,direction);
  //check for master mode select - check if client has sent for send thumbs up

  //clear addr
  (void)I2C1->SR1;

  while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)){STM_EVAL_LEDToggle(LED5);}

return 0;

}


int send_data_i2c(I2C_TypeDef* I2Cx,uint8_t data){


	I2C_SendData(I2Cx,data);
	//wait till this bad boy is thrown from our bus
	while(!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {STM_EVAL_LEDToggle(LED5);}
	return 0;
}

int I2C_stopper(I2C_TypeDef *I2Cx){

 I2C_GenerateSTOP(I2Cx,DISABLE);
 return 0;
}



void I2C2_EV_IRQHandler(void)
{
   handle_event();
}

void handle_event(){
inHandler++;	
STM_EVAL_LEDOn(LED6);
	
   switch (I2C_GetLastEvent(I2C2))
   {
   case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:
       
        	temp = I2C2->SR1;
        	temp = I2C2->SR2;
			I2C_ClearFlag(I2C2,I2C_FLAG_ADDR);
			inClearAdd++;
	    break;
					
   case I2C_EVENT_SLAVE_BYTE_RECEIVED:

	      temp = (uint8_t)I2C_ReceiveData(I2C2);
	      I2C_ClearFlag(I2C2,I2C_FLAG_BTF);
				inReceive++;
	      break;

   case I2C_EVENT_SLAVE_STOP_DETECTED:
	      blink();
	      if(I2C_GetFlagStatus(I2C2,I2C_FLAG_STOPF)==1){
	              	volatile uint32_t temp;
	              	temp = I2C2->SR1;
	              	I2C2->CR1 |= 0x1;
	              }
				inStop++;
	    break;

   default:
       break;

   }
}	 
	 void blink(){
		uint32_t i = 0;
		while(i != 10000000) {
            STM_EVAL_LEDOn(LED5);
            i++;
		}
     }
