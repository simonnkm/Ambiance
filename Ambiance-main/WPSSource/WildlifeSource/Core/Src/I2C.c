/*
 * Name I2C.c
 * Brief: provides a library to initialize and access the MCU's I2C hardware
 * Author: Caitlin Bonesio
 * Created: 3/4/25
 * Modified: 4/23/25
 */

//----------------------------------------Private Includes---------------------------------------
#include "I2C.h"
#include "BOARD.h"
#include "stdbool.h"
#include "I2C-UART_Manager.h"
#include "TIMERS.h"
#include "OledDriver.h"
//----------------------------------------Private Defines----------------------------------------

#define CIRCBUFFERSIZE 1536//Slightly less than 1.5 OLED prints

#define I2CBUFFERTIMEOUT 1000 // number of millisecond I2C transmit will wait for a spot in the buffer to appear


//#define I2CTESTHARNESS
//----------------------------------------Private Typedefs---------------------------------------
typedef struct MemAdd{//memory address packet, used for both reading and writing memory operations
	uint8_t targetadr;
	uint8_t registeradr;
	uint8_t data;
	uint8_t posterindex;
}MemAdd_t;

typedef struct MemAddBuffer{
	MemAdd_t data[CIRCBUFFERSIZE];
	uint16_t head;
	uint16_t tail;
}MemAddBuffer_t;

//----------------------------------------Private Variables--------------------------------------

I2C_HandleTypeDef hi2c1 = {I2C1};
void (*PosterList[NUMPOSTERS])(Event_t event) = POSTERS;
static MemAddBuffer_t MemBuff;//Receive and transmit request

static uint8_t initialized = 0;


//----------------------------------------Private functions--------------------------------------
#ifdef I2CTESTHARNESS
void fakeposter(Event_t event);
#endif
HAL_StatusTypeDef I2C_Post(MemAdd_t* post){
	if(post->posterindex == 0){
		return HAL_I2C_Mem_Write_IT(&hi2c1,
									post->targetadr<<1,
									post->registeradr,
									I2C_MEMADD_SIZE_8BIT,
									&(post->data),
									1);
	} else {
		return HAL_I2C_Mem_Read_IT(&hi2c1,
								   post->targetadr<<1,
								   post->registeradr,
								   I2C_MEMADD_SIZE_8BIT,
								   &(post->data),
								   1);

	}
}

HAL_StatusTypeDef I2C_Enqueue(MemAdd_t Post){
	if(!initialized){return HAL_ERROR;}
		HAL_StatusTypeDef status = HAL_OK;

		uint32_t start = TIMERS_GetMilliSeconds();
		while((MemBuff.tail == ((MemBuff.head+1)%CIRCBUFFERSIZE)) && (((start + I2CBUFFERTIMEOUT) > TIMERS_GetMilliSeconds())));
		if(MemBuff.tail == (MemBuff.head+1)%CIRCBUFFERSIZE){
			BSP_LED_On(LED_RED);//indicate a fatal buffer overflow
			return HAL_ERROR;
		}
		BSP_LED_Off(LED_RED);
		//place the transmit request into the buffer
		//The I2C-UARTmanager will disable the module but leave the state as ready, the data will still be loaded into the register
		//The module will not transmit until arbitration is complete


		MemBuff.data[MemBuff.head] = Post;
		MemBuff.head++;
		MemBuff.head %= CIRCBUFFERSIZE;
		if((MemBuff.tail+1)%CIRCBUFFERSIZE == MemBuff.head && hi2c1.State == HAL_I2C_STATE_READY){
			I2CUARTtoI2C(1);
			status = I2C_Post(MemBuff.data+MemBuff.tail);

		}
		return status;
}

/*
 * @Function: HAL_I2C_MemTxCpltCallback
 * @Brief: Overwrites a weak HAL function. Called at the end of an i2c memory transmit operation
 * @param: hi2c: I2C module that is in use
 * @return: none
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c){
	//operation complete, the data at tail is now out-dated;
	MemBuff.tail++;
	MemBuff.tail %= CIRCBUFFERSIZE;
	//if there is more in the buffer
	if(MemBuff.tail != MemBuff.head){
		I2CUARTtoI2C(1);
		I2C_Post(MemBuff.data+MemBuff.tail);

	}
}


/*
 * @Function: HAL_I2C_MemRxCpltCallback
 * @Brief: Overwrites a weak HAL function. Called at the end of an i2c memory receive operation
 * @param: hi2c: I2C module that is in use
 * @return: none
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c){

	//use the received data construct and event to post
	Event_t ret = {EVENT_I2C, MemBuff.data[MemBuff.tail].registeradr << 8 | MemBuff.data[MemBuff.tail].data};
	//post the event to the function given by the poster index

#ifdef I2CTESTHARNESS
	fakeposter(ret);
#else
	PosterList[MemBuff.data[MemBuff.tail].posterindex-1](ret);
#endif
	//operation complete, the data at tail is now out-dated;
	MemBuff.tail++;
	MemBuff.tail %= CIRCBUFFERSIZE;
	//if there is more in the buffer
	if(MemBuff.tail != MemBuff.head){
		I2CUARTtoI2C(1);
		I2C_Post(MemBuff.data+MemBuff.tail);

	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c){
	if(hi2c->ErrorCode&HAL_I2C_ERROR_AF){//NACK Error
		if(MemBuff.tail != MemBuff.head){
			uint32_t start = TIMERS_GetMilliSeconds();
			while(start+10 > TIMERS_GetMilliSeconds());
			I2CUARTtoI2C(1);
			I2C_Post(MemBuff.data+MemBuff.tail);//Repost the same data packet that got NACKed, without moving the tail forward

		}
	}
}
/*
 * @Function:I2C1_IRQHandler
 * @Brief: Overwrites a weak HAL function. handles I2C1 interrupts
 * @param: none
 * @return: none
 */
void I2C1_IRQHandler(void)
{
	if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR))
	{
		HAL_I2C_ER_IRQHandler(&hi2c1);//error handler
	}
	else
	{
		HAL_I2C_EV_IRQHandler(&hi2c1);//event handler
	}
}

//----------------------------------------Public Functions---------------------------------------

/*
 * @function: I2C_Init()
 * @brief: initialize the I2C peripheral to receive and transmit
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
int I2C_Init(void){
	if(initialized){return INIT_OK;}
	TIMERS_Init();
	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x00305B5B;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	hi2c1.State = HAL_I2C_STATE_RESET;
	HAL_StatusTypeDef ret = HAL_I2C_Init(&hi2c1);
	if (ret != HAL_OK)
	{
		return INIT_ERROR;
	}
	// Configure Analog filter
	ret = HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
	if (ret != HAL_OK)
	{
		return INIT_ERROR;
	}

	// Configure Digital filter
	ret = HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
	if (ret != HAL_OK)
	{
		return INIT_ERROR;
	}
	//set up circular buffers
	MemBuff.tail = 0;
	MemBuff.head = 0;
	initialized = 1;
	I2CUARTtoI2C(1);
	//enables interrupts
	HAL_NVIC_SetPriority(I2C1_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(I2C1_IRQn);
	return INIT_OK;
}

/*
 * @Function: I2C_Transmit
 * @Brief: Gives access to transmit a byte
 * @param: uint8_t targetadr: Address of the device to communicate
 * 		   uint8_t registeradr: Address of the register to write to
 * 		   uint8_t data: to to be transmitted
 * @return: a flag if the data has been transmitted
 */
HAL_StatusTypeDef I2C_Transmit(uint8_t targetadr, uint8_t registeradr, uint8_t data){
	if(!initialized){return HAL_ERROR;}
	HAL_StatusTypeDef status = HAL_OK;
	static MemAdd_t Post;
	Post.targetadr = targetadr;
	Post.registeradr = registeradr;
	Post.posterindex = 0;//null pointer, no function is posted to after the transmit is complete
	Post.data = data;
	status = I2C_Enqueue(Post);
	return status;
}

void I2C_Flushbuffer(){
	if(hi2c1.State == HAL_I2C_STATE_READY && MemBuff.tail != MemBuff.head){
		//error recovery
		I2C_Post(MemBuff.data+MemBuff.tail);
	}
}
/*
 * @Function: I2C_Recieve
 * @Brief: transmits requesting data. when the data is received it posts an event with the data to the PostTo function
 * @param: uint8_t targetadr: Address of the device to communicate
 * 		   uint8_t registeradr: Address of the register to read from
 * 		   void (*PostTo)(Event_t): which service to post to when the data is received
 * @return: -1 if error, 1 if success
 */
HAL_StatusTypeDef I2C_Recieve(uint8_t targetadr, uint8_t registeradr, uint8_t posterindex){
	if(!initialized){return HAL_ERROR;}
	HAL_StatusTypeDef status = HAL_OK;
	static MemAdd_t Post;
	Post.targetadr = targetadr;
	Post.registeradr = registeradr;
	if(posterindex == 0 || posterindex > NUMPOSTERS){
		return HAL_ERROR;
	}
	Post.posterindex = posterindex;
	Post.data = 0;//will store future recieved data
	status = I2C_Enqueue(Post);
	return status;

}

/** I2C_WriteReg(I2CAddress, deviceRegisterAddress, data)
 *
 * Writes one device register on chosen I2C device.
 *
 * @param   I2CAddress              (unsigned char) 7-bit address of I2C device
 *                                                  wished to interact with.
 * @param   deviceRegisterAddress   (unsigned char) 8-bit address of register on
 *                                                  device.
 * @param   data                    (uint8_t)       Data wished to be written to
 *                                                  register.
 * @return                          (unsigned char) [SUCCESS, ERROR]
 */
unsigned char I2C_WriteReg(
    unsigned char I2CAddress,
    unsigned char deviceRegisterAddress,
    uint8_t data
)
{
    HAL_StatusTypeDef ret;
//    I2CAddress = I2CAddress << 1; // Use 8-bit address.
//
//    ret = HAL_I2C_Mem_Write(
//        &hi2c1,
//        I2CAddress,
//        deviceRegisterAddress,
//        I2C_MEMADD_SIZE_8BIT,
//        &data,
//        1,
//        HAL_MAX_DELAY
//    );
    ret = I2C_Transmit(I2CAddress, deviceRegisterAddress, data);
    if (ret != HAL_OK)
    {
        return ERROR;
    }

    return SUCCESS;
}


//----------------------------------------Private Test Harness-----------------------------------
//#define is at the top of the page
#ifdef I2CTESTHARNESS
#include "BOARD.h"
#include "Oled.h"
#include "DiscountIO.h"
#include "UART.h"
#include "TIMERS.h"

HAL_StatusTypeDef status;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t stat;
uint8_t start;

void fakeposter(Event_t event){
	if(event.status == EVENT_I2C){
		switch (event.data>>8){
		case RTCMNTHADDR:
			month = ((event.data & 0x10)>>4)*10 + (event.data & 0x0F);
			break;
		case RTCDAYADDR:
			day = ((event.data & 0x30)>>4)*10 + (event.data & 0x0F);
			break;
		case RTCHOURADDR:
			if(event.data & 0x40){//AM/PM
				hour = 12*((event.data &0x20)>>5)+ 10*((event.data &0x10)>>4) + ((event.data &0x0F));
			}else {//24Hr
				hour = 10*((event.data &0x30)>>4) + ((event.data &0x0F));
			}
			break;
		case RTCMINADDR:
			//MINTEN2 MINTEN1 MINTEN0 MINONE3 MINONE2 MINONE1 MINONE0
			minute = 10*((event.data & 0x70)>>4) +((event.data &0x0F));

			break;
		case RTCSTATADDR:
			stat = event.data;
			break;
		case RTCSECADDR:
			start = event.data>>7;
			break;
		}
	}
	return;
}

int main(){

	BOARD_Init();
	TIMERS_Init();
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Init(LED_RED);
	I2C_Init();
	I2C_Transmit(RTCADDRESS, RTCSECADDR, 0x80);
	I2C_Transmit(RTCADDRESS, RTCSTATADDR, 0x08);
//	I2C_Transmit(RTCADDRESS, RTCSECADDR, 0x00);
//	I2C_Transmit(RTCADDRESS, RTCSTATADDR, 0x00);

	I2C_Transmit(RTCADDRESS, RTCMINADDR, 0x16);
	I2C_Transmit(RTCADDRESS, RTCHOURADDR, 0x10);
	I2C_Transmit(RTCADDRESS, RTCDAYADDR, 0x28);
	I2C_Transmit(RTCADDRESS, RTCMNTHADDR, 0x08);
	HAL_Delay(100);
	OledInit();
	HAL_Delay(1000);
	while (1){
		if(TIMERS_GetMilliSeconds()%500 == 0){
			I2C_Recieve(RTCADDRESS, RTCDAYADDR, 1);
			I2C_Recieve(RTCADDRESS, RTCHOURADDR, 1);
			I2C_Recieve(RTCADDRESS, RTCMINADDR, 1);
			I2C_Recieve(RTCADDRESS, RTCMNTHADDR, 1);
			I2C_Recieve(RTCADDRESS, RTCSECADDR, 1);
			I2C_Recieve(RTCADDRESS, RTCSTATADDR, 1);

			char text[88];
			sprintf(text, "current time:\n"
						  "%d/%d %d:%.2d\n"
						  "status: %X, start: %d", month, day, hour, minute, stat, start);
			OledDrawString(text);
			OledUpdate();
		}


	}

}
#endif
