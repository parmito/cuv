/*
 * Io.c
 *
 *  Created on: 02/10/2018
 *      Author: danilo
 */

/* Kernel includes. */
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_sleep.h"

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <rom/spi_flash.h>
#include "driver/gpio.h"
#include "driver/can.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "UartGsm.h"
#include "State.h"
#include "defines.h"
#include "Io.h"
#include "Sd.h"
#include "Ble.h"
#include "Gsm.h"

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"


#define GPIO_DS18B20_0       (GPIO_ONE_WIRE)
#define MAX_DEVICES          (8)
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD (1000) // milliseconds

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

#define TASK_PERIOD 	10			/* 10ms*/

#define NUM_OF(x) (sizeof(x)/sizeof(x[0]))

//////////////////////////////////////////////
//
//
//            FUNCTION PROTOTYPES
//
//
//////////////////////////////////////////////
void vTaskIo( void *pvParameters );
void Io_Sleeping(void);
void TaskIo_TransmitGenericCanMessage(tstCanMessage stCanMessage[]);

//////////////////////////////////////////////
//
//
//            VARIABLES
//
//
//////////////////////////////////////////////

#if 0
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
	

	/*ACC_02	0x30C	780	Classical	8	60*/		
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x030C,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},	
	/*Airbag_01	0x040	64	Classical	8	50*/	
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0040,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Allrad_03	0x118	280	Classical	4	100*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0118,.stCan.data_length_code=4,.stCan.data={0,0,0,0,0,0,0,0}},
	/*BEM_02	0x663	1635	Classical	8	100*/			
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0663,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Blinkmodi_02	0x366	870	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0366,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Diagnose_01	0x6B2	1714	Classical	8	1000*/	
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B2,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Dimmung_01	0x5F0	1520	Classical	8	200*/	
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x05F0,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Einheiten_01	0x643	1603	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0643,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*ESP_02	0x101	257	Classical	8	20*/	
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0101,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*ESP_10	0x116	278	Classical	8	200*/	
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0116,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}
	/*ESP_20	0x65D	1629	Classical	8	1000*/	
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x065D,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}
	/*ESP_21	0x0FD	253	Classical	8	20*/	
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x00FD,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}
	/*ESP_24	0x31B	795	Classical	8	50*/	
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x031B,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}
	/*FDR_04	0x2C3	707	Classical	8	100*/	
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x02C3,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}
	/*Gateway_71	0x3DA	986	Classical	8	100*/	
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03DA,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}	
	/*Gateway_72	0x3DB	987	Classical	8	100*/	
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03DB,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}	
	/*Gateway_73	0x3DC	988	Classical	8	50	*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03DC,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}		
	/*Gateway_77	0x3EA	1002	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03EA,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}			
	/*Klima_12	0x668	1640	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0668,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}			
	/*Kombi_01	0x30B	779	Classical	8	50*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x030B,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}			
	/*Kombi_02	0x6B7	1719	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B7,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}			
	/*Kombi_03	0x6B8	1720	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B8,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}			
	/*Kombi_08	0x16A95497	380195991	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x16A95497,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Motor_04	0x107	263	Classical	8	20*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0107,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Motor_14	0x3BE	958	Classical	8	100	*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03BE,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Motor_18	0x670	1648	Classical	8	500	*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0670,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Motor_25	0x56F	1391	Classical	8	500	*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x056F,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Motor_26	0x3C7	967	Classical	8	100	*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03C7,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
	/*Motor_29	0x147	327	Classical	8	20*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0147,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
	/*Motor_Code_01	0x641	1601	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0641,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
	/*NMH_AMP	0x1B00006F	452984943	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x1B00006F,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*NMH_Gateway	0x1B000010	452984848	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x1B000010,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Personalisierung_01	0x16A9540A	380195850	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x16A9540A,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Personalisierung_02	0x12DD5485	316494981	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x12DD5485,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
	/*Systeminfo_01	0x585	1413	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0585,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}						
	/*VIN_01	0x6B4	1716	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B4,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
	/*WBA_03	0x394	916	Classical	8	160*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0394,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
	/*WFS_01	0x551	1361	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0551,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
	/*Klemmen_Status_01	0x3C0	960	Classical	4	100*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03C0,.stCan.data_length_code=4,.stCan.data={0,0,0,0,0,0,0,0}}					
#endif	
OneWireBus * owb;
owb_rmt_driver_info rmt_driver_info;
OneWireBus_ROMCode device_rom_codes[MAX_DEVICES] = {0};
int num_devices = 0;
OneWireBus_SearchState search_state = {0};
bool found = false;


extern tstIo stIo;
sMessageType stIoMsg;
static const char *IO_TASK_TAG = "IO_TASK";

/*static esp_adc_cal_characteristics_t *adc_chars;*/
static const adc_channel_t channel = ADC_CHANNEL_3;     /*GPIO39 if ADC1*/
static const adc_atten_t atten = ADC_ATTEN_DB_11;
/*static const adc_unit_t unit = ADC_UNIT_1;*/
TickType_t Elapsed_Time1,Elapsed_Time2,Elapsed_Time3;
unsigned char ucCurrentStateIo = TASKIO_IDLING;

tstCanMessage	stCanMessage0ms[3]={

{.u32Timeout = 0,.u32Period = 0,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31}},
{.u32Timeout = 0,.u32Period = 0,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31}},
{.u32Timeout = 0,.u32Period = 0,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31}}
};

tstCanMessage	stCanMessage20ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
	
	/*ESP_02	0x101	257	Classical	8	20*/	
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0101,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*ESP_21	0x0FD	253	Classical	8	20*/	
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x00FD,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Motor_04	0x107	263	Classical	8	20*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0107,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Motor_29	0x147	327	Classical	8	20*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0147,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 20,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}							
};


tstCanMessage	stCanMessage50ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
		
	/*Airbag_01	0x040	64	Classical	8	50*/	
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0040,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*ESP_24	0x31B	795	Classical	8	50*/	
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x031B,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Gateway_73	0x3DC	988	Classical	8	50	*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03DC,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},		
	/*Kombi_01	0x30B	779	Classical	8	50*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x030B,.stCan.data_length_code=8,.stCan.data={0,0,0x80,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 50,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}							
};

tstCanMessage	stCanMessage60ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/	
	/*ACC_02	0x30C	780	Classical	8	60*/		
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x030C,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},	
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 60,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}								
	
};	


tstCanMessage	stCanMessage100ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
	
	/*Allrad_03	0x118	280	Classical	4	100*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0118,.stCan.data_length_code=4,.stCan.data={0,0,0,0,0,0,0,0}},
	/*BEM_02	0x663	1635	Classical	8	100*/			
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0663,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*FDR_04	0x2C3	707	Classical	8	100*/	
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x02C3,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Gateway_71	0x3DA	986	Classical	8	100*/	
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03DA,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},	
	/*Gateway_72	0x3DB	987	Classical	8	100*/	
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03DB,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},	
	/*Motor_14	0x3BE	958	Classical	8	100	*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03BE,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Motor_26	0x3C7	967	Classical	8	100	*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03C7,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Klemmen_Status_01	0x3C0	960	Classical	4	100*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03C0,.stCan.data_length_code=4,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},						
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},						
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},						
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},							
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 100,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}
};

tstCanMessage	stCanMessage160ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
	
	/*WBA_03	0x394	916	Classical	8	160*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0394,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 160,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
};



tstCanMessage	stCanMessage200ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
		
	/*Dimmung_01	0x5F0	1520	Classical	8	200*/	
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x05F0,.stCan.data_length_code=8,.stCan.data={0xF3,0x40,0,0,0,0,0,0}},
	/*ESP_10	0x116	278	Classical	8	200*/	
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0116,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Gateway_77	0x3EA	1002	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x03EA,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},		
	/*Klima_12	0x668	1640	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0668,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*NMH_AMP	0x1B00006F	452984943	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x1B00006F,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*NMH_Gateway	0x1B000010	452984848	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x1B000010,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*WFS_01	0x551	1361	Classical	8	200*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0551,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*VIN_01	0x6B4	1716	Classical	8	200 - REMOVIDO*/
	/*{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B4,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}					
};

tstCanMessage	stCanMessage500ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
	
	/*Motor_18	0x670	1648	Classical	8	500	*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0670,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Motor_25	0x56F	1391	Classical	8	500	*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x056F,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 500,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}			
	
};


tstCanMessage	stCanMessage1000ms[SIZE_OF_STRUCT]=
{
	/******************************************************************/
	/*Message	Id(Hex)		Id(Dec)		Flag		Length		Period*/			
	/******************************************************************/
	/*Blinkmodi_02	0x366	870	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0366,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Diagnose_01	0x6B2	1714	Classical	8	1000*/	
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B2,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Einheiten_01	0x643	1603	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0643,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*ESP_20	0x65D	1629	Classical	8	1000*/	
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x065D,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Kombi_02	0x6B7	1719	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B7,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Kombi_03	0x6B8	1720	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 200,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x06B8,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},			
	/*Kombi_08	0x16A95497	380195991	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x16A95497,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Motor_Code_01	0x641	1601	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0641,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},					
	/*Personalisierung_01	0x16A9540A	380195850	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x16A9540A,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Personalisierung_02	0x12DD5485	316494981	Classical	8	1000	*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_EXTD,.stCan.identifier=0x12DD5485,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},				
	/*Systeminfo_01	0x585	1413	Classical	8	1000*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0585,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},						
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}},
	/*Reserved*/
	{.u32Timeout = 0,.u32Period = 1000,.stCan.flags=CAN_MSG_FLAG_NONE,.stCan.identifier=0x0000,.stCan.data_length_code=8,.stCan.data={0,0,0,0,0,0,0,0}}				
};


#define cb_CRC_POLY 0x2F
//////////////////////////////////////////////
//
//
//           CalculateCrc8
//
//
//////////////////////////////////////////////
char CalculateCrc8 (char* data_byte_array,char cb_DATA_BYTE_SIZE) 
{
	char Databyte1 = 0x00;
	char crc = 0xFF;
	uint8_t byte_index;
	char bit_index;

	/* VW CRC-8 based on Polynonmial C2*/
	for( byte_index=Databyte1; byte_index<cb_DATA_BYTE_SIZE; ++byte_index ) 
	{ 
		crc ^= data_byte_array[byte_index]; 
			
		for( bit_index=0; bit_index<8; ++bit_index ) 
		{ 
			if( (crc & 0x80) != 0 ) 
			crc = (crc << 1) ^ cb_CRC_POLY; 
			else
			{
			 	crc = (crc << 1); 
			}
		} 
	} 	
	crc = ~crc; 
	return((char)crc);
}

//////////////////////////////////////////////
//
//
//              	atoh
//
//
//////////////////////////////////////////////
void My_atoh(char *ascii_ptr, char *hex_ptr,int len)
{
    int i;

    for(i = 0; i < (len / 2); i++)
    {

        *(hex_ptr+i)   = (*(ascii_ptr+(2*i)) <= '9') ? ((*(ascii_ptr+(2*i)) - '0') * 16 ) :  (((*(ascii_ptr+(2*i)) - 'A') + 10) << 4);
        *(hex_ptr+i)  |= (*(ascii_ptr+(2*i)+1) <= '9') ? (*(ascii_ptr+(2*i)+1) - '0') :  (*(ascii_ptr+(2*i)+1) - 'A' + 10);

    }
}

unsigned int atoh(char *cIn,char *cOut)
{
	printf("***********************************\r\n");
	printf("                                   \r\n");	 	      	
	printf("  atoh(char *cIn,char *cOut)       \r\n");
    printf("                                   \r\n");	 	      		
	printf("***********************************\r\n");	 	      
	char c = 0;
	unsigned int i = 0;


	while(*cIn != 0)
	{
		for(char i = 0; i < 2; i++)		
		{
			*cOut <<=4;
			if(*cIn >= '0' && *cIn <= '9')
			{
				c = (*cIn)  - 0x30;
			}
			else
			{
				toupper((int)*cIn);
				if(*cIn >= 'A' && *cIn <= 'F')
				{
					c = (*cIn)  - 0x37;
				}			
			}
			*cOut += c;		
			cIn++;			
		}
		*cOut = *cOut + (*cOut<<8);
		cOut++;
		i++;
	}
	return(i);
}


//////////////////////////////////////////////
//
//
//      	FindIndexOfMessage
//
//
//////////////////////////////////////////////
uint8_t FindIndexOfMessage(tstCanMessage stCanMessage[],unsigned long u32Identifier,uint8_t *pu8Index)
{
	unsigned char u8Return = false;
	uint8_t u8 = 0;

	while(u8 < SIZE_OF_STRUCT)
	{	
		if(stCanMessage[u8].stCan.identifier != 0)
		{
			if(stCanMessage[u8].stCan.identifier == u32Identifier)
			{
				*pu8Index = u8;
				u8Return = true;
				break;
			}
		}
		u8++;
	}	
	return(u8Return);
}


//////////////////////////////////////////////
//
//
//      	FindIndexOfEmptyMessage
//
//
//////////////////////////////////////////////
unsigned char FindIndexOfEmptyMessage(tstCanMessage stCanMessage[],unsigned char *pu8Index)
{
	unsigned char u8Return = false;
	unsigned char u8 = 0;

	while(u8 < SIZE_OF_STRUCT)
	{	
		if(stCanMessage[u8].stCan.identifier == 0)
		{
			*pu8Index = u8;
			u8Return = true;
			break;
		}
		u8++;
	}	
	return(u8Return);
}

//////////////////////////////////////////////
//
//
//              Io_Configuration
//
//
//////////////////////////////////////////////
void Io_Configuration(void)
{
	/////////////////
	// IGNITION PIN
	/////////////////
	gpio_config_t io_conf;
	//////////////////////////
	// ADC MAIN BATTERY
	/////////////////////////
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_INPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_INPUT_IGNITION_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 1;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);


    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel, atten);

#if 0
    // Create a 1-Wire bus, using the RMT timeslot driver
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    // Find all connected devices
    printf("Find devices:\n");
    owb_search_first(owb, &search_state, &found);
    while (found)
    {
        char rom_code_s[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_s, sizeof(rom_code_s));
        printf("  %d : %s\n", num_devices, rom_code_s);
        device_rom_codes[num_devices] = search_state.rom_code;
        ++num_devices;
        owb_search_next(owb, &search_state, &found);
    }
    printf("Found %d device%s\n", num_devices, num_devices == 1 ? "" : "s");

	// In this example, if a single device is present, then the ROM code is probably
	// not very interesting, so just print it out. If there are multiple devices,
	// then it may be useful to check that a specific device is present.

	if (num_devices == 1)
	{
		// For a single device only:
		OneWireBus_ROMCode rom_code;
		owb_status status = owb_read_rom(owb, &rom_code);
		if (status == OWB_STATUS_OK)
		{
			char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
			owb_string_from_rom_code(rom_code, rom_code_s, sizeof(rom_code_s));
			printf("Single device %s present\n", rom_code_s);
		}
		else
		{
			printf("An error occurred reading ROM code: %d", status);
		}
	}
	else
	{
		// Search for a known ROM code (LSB first):
		// For example: 0x1502162ca5b2ee28
		OneWireBus_ROMCode known_device = {
			.fields.family = { 0x28 },
			.fields.serial_number = { 0xee, 0xb2, 0xa5, 0x2c, 0x16, 0x02 },
			.fields.crc = { 0x15 },
		};
		char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
		owb_string_from_rom_code(known_device, rom_code_s, sizeof(rom_code_s));
		bool is_present = false;

		owb_status search_status = owb_verify_rom(owb, known_device, &is_present);
		if (search_status == OWB_STATUS_OK)
		{
			printf("Device %s is %s\n", rom_code_s, is_present ? "present" : "not present");
		}
		else
		{
			printf("An error occurred searching for known device: %d", search_status);
		}
	}

	//    // Read temperatures from all sensors sequentially
	//    while (1)
	//    {
	//        printf("\nTemperature readings (degrees C):\n");
	//        for (int i = 0; i < num_devices; ++i)
	//        {
	//            float temp = ds18b20_get_temp(devices[i]);
	//            printf("  %d: %.3f\n", i, temp);
	//        }
	//        vTaskDelay(1000 / portTICK_PERIOD_MS);
	//    }
#endif

	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_OUTPUT_GSM_DIAG_PIN_SEL ;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);

	//Initialize configuration structures using macro initializers
	can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NO_ACK);
	can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
	can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();


	//Install CAN driver
	if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
		printf("CAN:Driver installed\n");
	} else {
		printf("CAN:Failed to install driver\n");
		return;
	}

	//Start CAN driver
	if (can_start() == ESP_OK) {
		printf("CAN:Driver started\n");
	} else {
		printf("CAN:Failed to start driver\n");
		return;
	}
}
//////////////////////////////////////////////
//
//
//              Io_Sleeping
//
//
//////////////////////////////////////////////
void Io_Sleeping(void)
{
#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"SLEEPING\r\n");
#endif

    const int ext_wakeup_pin_1 = 36;
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;

	gpio_set_level(GPIO_OUTPUT_GPS_ENABLE, 1);

    printf("Enabling EXT1 wakeup on pins GPIO%d\r\n", ext_wakeup_pin_1);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    esp_deep_sleep_start();
}

TaskHandle_t xHandle;
//////////////////////////////////////////////
//
//
//              Io_Init
//
//
//////////////////////////////////////////////
void Io_Init(void)
{
    Io_Configuration();

    xTaskCreate(vTaskIo, "vTaskIo", 1024*2, NULL, configMAX_PRIORITIES, &xHandle);
	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */
	
	stIoMsg.ucSrc = SRC_IO;
	stIoMsg.ucDest = SRC_IO;
	stIoMsg.ucEvent = EVENT_IO_INIT;
	xQueueSend( xQueueIo, ( void * )&stIoMsg, 0);

}


//////////////////////////////////////////////
//
//
//              TaskIo_Init
//
//
//////////////////////////////////////////////
unsigned char TaskIo_Init(sMessageType *psMessage)
{
    unsigned char boError = true;

	#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"TaskIo_Init\r\n");
	#endif

	/*memset(stCanMessage,0,sizeof(stCanMessage)/sizeof(char));*/

	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskIo_ReadIo
//
//
//////////////////////////////////////////////
extern unsigned long u32TimeToSleep;

unsigned char TaskIo_ReadIo(void)
{
	unsigned char boError = true;
	static unsigned char ucCurrIgnition = 0xFF;
	static long i32EnterSleepMode = (120*(1000 / portTICK_PERIOD_MS));
	static unsigned long u32CurrentTime,u32PreviousTime = 0xFFFFFFFF,u32DeltaTime;

    uint32_t adc_reading = 0;
    static char cLocalBuffer[64];

	#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"TaskIo_ReadIo\r\n");
	#endif


    /* Multisampling */
    for (int i = 0; i < NO_OF_SAMPLES; i++)
    {
    	adc_reading += adc1_get_raw((adc1_channel_t)channel);
    }
    adc_reading /= NO_OF_SAMPLES;
/*
 	R11 = 2M5,R13=750, MULTIPLICADOR = 8192 , 2 exp 11
    Regressão

	y = ax + b
	y = 23,2164979828982*x + 19392,2479948266


*/
    stIo.flAdMainBatteryVoltage = (adc_reading*23.2164979828982);
    stIo.flAdMainBatteryVoltage += 19392.2479948266;
    stIo.flAdMainBatteryVoltage /= (((1ULL<<13)));


	#if DEBUG_IO
	/*ESP_LOGI(IO_TASK_TAG,"AD Voltage=%d\r\n",adc_reading);
	ESP_LOGI(IO_TASK_TAG,"MainBattery Voltage=%.2f\r\n",stIo.flAdMainBatteryVoltage);*/
	#endif


	//    // Read temperatures from all sensors sequentially
	//    while (1)
	//    {
	//        printf("\nTemperature readings (degrees C):\n");
	//        for (int i = 0; i < num_devices; ++i)
	//        {
	//            float temp = ds18b20_get_temp(devices[i]);
	//            printf("  %d: %.3f\n", i, temp);
	//        }
	//        vTaskDelay(1000 / portTICK_PERIOD_MS);
	//    }
#if 0
	// Create DS18B20 devices on the 1-Wire bus
	DS18B20_Info * devices[MAX_DEVICES] = {0};
	for (int i = 0; i < num_devices; ++i)
	{
		DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
		devices[i] = ds18b20_info;

		if (num_devices == 1)
		{
			#if DEBUG_IO
			/*ESP_LOGI(IO_TASK_TAG,"Single device optimisations enabled\n");*/
			#endif

			ds18b20_init_solo(ds18b20_info, owb);          // only one device on bus
		}
		else
		{
			ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
		}
		ds18b20_use_crc(ds18b20_info, true);           // enable CRC check for temperature readings
		ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
	}

	// Read temperatures more efficiently by starting conversions on all devices at the same time
	int errors_count[MAX_DEVICES] = {0};
	int sample_count = 0;
	if (num_devices > 0)
	{
		ds18b20_convert_all(owb);

		// In this application all devices use the same resolution,
		// so use the first device to determine the delay
		ds18b20_wait_for_conversion(devices[0]);

		// Read the results immediately after conversion otherwise it may fail
		// (using printf before reading may take too long)
		float readings[MAX_DEVICES] = { 0 };
		DS18B20_ERROR errors[MAX_DEVICES] = { 0 };

		for (int i = 0; i < num_devices; ++i)
		{
			errors[i] = ds18b20_read_temp(devices[i], &readings[i]);
		}

		// Print results in a separate loop, after all have been read
		#if DEBUG_IO
		/*ESP_LOGI(IO_TASK_TAG,"\nTemperature readings (degrees C): sample %d\n", ++sample_count);*/
		#endif

		for (int i = 0; i < num_devices; ++i)
		{
			if (errors[i] != DS18B20_OK)
			{
				++errors_count[i];
			}
			stIo.flI2cTemperature = readings[i];
		}
	}

#endif
		#if DEBUG_IO
		/*ESP_LOGI(IO_TASK_TAG,"  %d: %.1f    %d errors\n", i, readings[i], errors_count[i]);*/
		/*ESP_LOGI(IO_TASK_TAG,"Temperature=%.1f C\n", stIo.flI2cTemperature);*/


		memset(cLocalBuffer,0,sizeof(cLocalBuffer));
		sprintf(cLocalBuffer,"IO:SW,Bat,Temp,Ign=%s,%.1f,%.1f,%d\r\n",SOFTWARE_VERSION,stIo.flAdMainBatteryVoltage,stIo.flI2cTemperature,stIo.ucIgnition);

		stIoMsg.ucSrc = SRC_IO;
		stIoMsg.ucDest = SRC_BLE;
		stIoMsg.ucEvent = (int)NULL;
		stIoMsg.pcMessageData = &cLocalBuffer[0];

		xQueueSend(xQueueBle,( void * )&stIoMsg,NULL);
		#endif

	// clean up dynamically allocated data
#if 0
	for (int i = 0; i < num_devices; ++i)
	{
		ds18b20_free(&devices[i]);
	}
	/*owb_uninitialize(owb);*/
#endif
	if(gpio_get_level(GPIO_INPUT_IGNITION) == 1)
	{
		stIo.ucIgnition = 1;
		i32EnterSleepMode = (120*(1000/TASK_PERIOD));/*(u32TimeToSleep);*/

		uint8_t u8Status = 0;
		uint8_t u8Index = 0;
		u8Status = FindIndexOfMessage(stCanMessage100ms,0x03C0,&u8Index);
		
		if(u8Status != 0)
		{
			stCanMessage100ms[u8Index].stCan.data[2] = 0x03;
		}
		#if DEBUG_IO
		ESP_LOGI(IO_TASK_TAG,"Ignition ON\r\n");
		#endif

		if(ucCurrIgnition != stIo.ucIgnition)
		{
			#if DEBUG_IO
			/*ESP_LOGI(IO_TASK_TAG,"Status=%d,Index=%d\r\n",u8Status,u8Index);*/
			#endif

			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"Ignition ON\r\n");
			#endif
			ucCurrIgnition = stIo.ucIgnition;
		}
	}
	else
	{
		stIo.ucIgnition = 0;

		uint8_t u8Status = 0;
		uint8_t u8Index = 0;
		u8Status = FindIndexOfMessage(stCanMessage100ms,0x03C0,&u8Index);
		if(u8Status != 0)
		{
			stCanMessage100ms[u8Index].stCan.data[2] = 0;
		}

		#if DEBUG_IO
		ESP_LOGI(IO_TASK_TAG,"Ignition OFF\r\n");
		#endif

		u32CurrentTime = (unsigned long)(xTaskGetTickCount());
		#if DEBUG_IO
		ESP_LOGI(IO_TASK_TAG,"u32CurrentTime=%ld\r\n",u32CurrentTime);
		#endif
		if(u32PreviousTime != 0xFFFFFFFF){
			u32DeltaTime = u32CurrentTime - u32PreviousTime;
			u32PreviousTime = u32CurrentTime;
		}else{
			u32PreviousTime = u32CurrentTime;
		}
		#if DEBUG_IO
		ESP_LOGI(IO_TASK_TAG,"ElapsedTime=%ld\r\n",u32DeltaTime);
		#endif

		if(ucCurrIgnition != stIo.ucIgnition)
		{
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"Status=%d,Index=%d\r\n",u8Status,u8Index);
			#endif

			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"Ignition OFF\r\n");
			#endif
			ucCurrIgnition = stIo.ucIgnition;
		}

		if(i32EnterSleepMode > 0)
		{
			i32EnterSleepMode-= u32DeltaTime;
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"Sleep=%d\r\n",(int)i32EnterSleepMode);
			#endif
		}
		else
		{
			Io_Sleeping();
		}
	}

	return(boError);
}
	
//////////////////////////////////////////////
//
//
//              TaskIo_AddCanMessage
//
//
//////////////////////////////////////////////
unsigned char TaskIo_AddCanMessage(sMessageType *psMessage)
{	
	unsigned char boError = true;
	char *pcData;
	char cIdentifier[16];

	uint8_t u8Flags;/* CAN FLAG*/
	uint8_t u8Index;/* INDEX OF MESSAGE INSIDE BUFFER*/
	uint8_t u8Status;
	uint8_t u8Length = 0;/* CAN MESSAGE LENGTH*/
	
	unsigned long u32Identifier = 0;/* CAN IDENTIFIER*/
	char cData[16];
	unsigned int c = 0;
	unsigned int i = 0;
	unsigned long u32Period = 0;	/* CAN PERIOD OF MESSAGE*/

	memset(cIdentifier,0,sizeof(cIdentifier));
	memset(cData,0,sizeof(cData));
	
	#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"IO:ADD CAN MESSAGE\r\n");
	#endif

	pcData = psMessage->pcMessageData;
	
	/* There will be no index in the message sent over BLE*/
	
	/* Get Period of message*/
	pcData = pcData + strlen("CAN:");
	pcData = strtok((char*)pcData,",");
	if(pcData != NULL)
	{
		u32Period =  atoi(pcData);
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"IO:u32Period=%d\r\n",(int)u32Period);
			#endif
	}
	
	/* Get Identifier of message*/
	pcData = strtok(NULL,",");
	if(pcData != NULL)
	{		
		#if DEBUG_IO
		ESP_LOGI(IO_TASK_TAG,"IO:CanIdentifier=%s\r\n",pcData);
		#endif

		/* return the quantity of numbers, and numbers must be odd*/
		/* in case of 30C, then add 1*/
		i  = atoh(pcData,cIdentifier);

		/*My_atoh(pcData,cIdentifier,strlen(pcData));*/
		/*u32Identifier = (unsigned long)(*cIdentifier);*/
		c = 0;
		while(i > 0)
		{			
			u32Identifier = ((u32Identifier<<8) + (cIdentifier[c] & 0x000000FF));
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"IO:CanIdentifier=0x%04lX\r\n",u32Identifier);
			#endif
			c++;
			i--;
		}

		u8Flags = CAN_MSG_FLAG_NONE;
		if(c > 2)
		{
			u8Flags = CAN_MSG_FLAG_EXTD;
		}		
		
		pcData = strtok(NULL,"\0");
		if(pcData != NULL)
		{
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"IO:CanData=%s\r\n",pcData);
			#endif

			u8Length = atoh(pcData,cData);
		}										
		/*Once identified the Period of message, we have to 
		select the struct for that specific group of messages
		which it belongs to*/
		
		switch(u32Period)		
		{

			case 0:/*On demand*/
				/* Allocate this new message on the buffer*/
				stCanMessage0ms[u8Index].stCan.identifier = u32Identifier;
				stCanMessage0ms[u8Index].stCan.flags = u8Flags;
				stCanMessage0ms[u8Index].stCan.data_length_code = u8Length;
				c = 0;
				while(u8Length > 0)
				{
					stCanMessage0ms[u8Index].stCan.data[c] = cData[c];
					#if DEBUG_IO
					ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage0ms[u8Index].stCan.data[c]);
					#endif
					c++;
					u8Length--;
				}
				break;


			case 20:/*20ms*/
				u8Status = FindIndexOfMessage(stCanMessage20ms,u32Identifier,&u8Index);
				if(u8Status != true)
				{
					u8Status = FindIndexOfEmptyMessage(stCanMessage20ms,&u8Index);
					if(u8Status != true)
					{
						/* No empty message to allocate a new one*/
					}
					else
					{
						/* Allocate this new message on the buffer*/
						stCanMessage20ms[u8Index].stCan.identifier = u32Identifier;
						stCanMessage20ms[u8Index].stCan.flags = u8Flags;
						stCanMessage20ms[u8Index].stCan.data_length_code = u8Length;
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage20ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage20ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
				}
				else
				{
					/* Change the message already allocated on the buffer*/	
					c = 0;
					while(u8Length > 0)
					{
						stCanMessage20ms[u8Index].stCan.data[c] = cData[c];
						#if DEBUG_IO
						ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage20ms[u8Index].stCan.data[c]);
						#endif
						c++;
						u8Length--;
					}	
				}
				break;

			case 50:/*50ms*/
				u8Status = FindIndexOfMessage(stCanMessage50ms,u32Identifier,&u8Index);
				if(u8Status != true)
				{
					u8Status = FindIndexOfEmptyMessage(stCanMessage50ms,&u8Index);
					if(u8Status != true)
					{
						/* No empty message to allocate a new one*/
					}
					else
					{
						/* Allocate this new message on the buffer*/
						stCanMessage50ms[u8Index].stCan.identifier = u32Identifier;
						stCanMessage50ms[u8Index].stCan.flags = u8Flags;
						stCanMessage50ms[u8Index].stCan.data_length_code = u8Length;
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage50ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage50ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
				}
				else
				{
					/* Change the message already allocated on the buffer*/	
					c = 0;
					while(u8Length > 0)
					{
						stCanMessage50ms[u8Index].stCan.data[c] = cData[c];
						#if DEBUG_IO
						ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage50ms[u8Index].stCan.data[c]);
						#endif
						c++;
						u8Length--;
					}	
				}
				break;	
				
			case 60:/*60ms*/
				u8Status = FindIndexOfMessage(stCanMessage60ms,u32Identifier,&u8Index);
				if(u8Status != true)
				{
					u8Status = FindIndexOfEmptyMessage(stCanMessage60ms,&u8Index);
					if(u8Status != true)
					{
						/* No empty message to allocate a new one*/
					}
					else
					{
						/* Allocate this new message on the buffer*/
						stCanMessage60ms[u8Index].stCan.identifier = u32Identifier;
						stCanMessage60ms[u8Index].stCan.flags = u8Flags;
						stCanMessage60ms[u8Index].stCan.data_length_code = u8Length;
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage60ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage60ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
				}
				else
				{
					/* Change the message already allocated on the buffer*/	
					c = 0;
					while(u8Length > 0)
					{
						stCanMessage60ms[u8Index].stCan.data[c] = cData[c];
						#if DEBUG_IO
						ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage60ms[u8Index].stCan.data[c]);
						#endif
						c++;
						u8Length--;
					}	
				}
				break;	

			case 100:/*100ms*/
				u8Status = FindIndexOfMessage(stCanMessage100ms,u32Identifier,&u8Index);
				if(u8Status != true)
				{
					u8Status = FindIndexOfEmptyMessage(stCanMessage100ms,&u8Index);
					if(u8Status != true)
					{
						/* No empty message to allocate a new one*/
					}
					else
					{
						/* Allocate this new message on the buffer*/
						stCanMessage100ms[u8Index].stCan.identifier = u32Identifier;
						stCanMessage100ms[u8Index].stCan.flags = u8Flags;
						stCanMessage100ms[u8Index].stCan.data_length_code = u8Length;
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage100ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage100ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
				}
				else
				{
					/* Change the message already allocated on the buffer*/	
					c = 0;
					while(u8Length > 0)
					{
						stCanMessage100ms[u8Index].stCan.data[c] = cData[c];
						#if DEBUG_IO
						ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage100ms[u8Index].stCan.data[c]);
						#endif
						c++;
						u8Length--;
					}	
				}
				break;	


				case 160:/*160ms*/
				u8Status = FindIndexOfMessage(stCanMessage160ms,u32Identifier,&u8Index);
				if(u8Status != true)
				{
					u8Status = FindIndexOfEmptyMessage(stCanMessage160ms,&u8Index);
					if(u8Status != true)
					{
						/* No empty message to allocate a new one*/
					}
					else
					{
						/* Allocate this new message on the buffer*/
						stCanMessage160ms[u8Index].stCan.identifier = u32Identifier;
						stCanMessage160ms[u8Index].stCan.flags = u8Flags;
						stCanMessage160ms[u8Index].stCan.data_length_code = u8Length;
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage160ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage160ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
				}
				else
				{
					/* Change the message already allocated on the buffer*/	
					c = 0;
					while(u8Length > 0)
					{
						stCanMessage160ms[u8Index].stCan.data[c] = cData[c];
						#if DEBUG_IO
						ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage160ms[u8Index].stCan.data[c]);
						#endif
						c++;
						u8Length--;
					}	
				}
				break;	
				
				case 200:/*200ms*/
					u8Status = FindIndexOfMessage(stCanMessage200ms,u32Identifier,&u8Index);
					if(u8Status != true)
					{
						u8Status = FindIndexOfEmptyMessage(stCanMessage200ms,&u8Index);
						if(u8Status != true)
						{
							/* No empty message to allocate a new one*/
						}
						else
						{
							/* Allocate this new message on the buffer*/
							stCanMessage200ms[u8Index].stCan.identifier = u32Identifier;
							stCanMessage200ms[u8Index].stCan.flags = u8Flags;
							stCanMessage200ms[u8Index].stCan.data_length_code = u8Length;
							c = 0;
							while(u8Length > 0)
							{
								stCanMessage200ms[u8Index].stCan.data[c] = cData[c];
								#if DEBUG_IO
								ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage200ms[u8Index].stCan.data[c]);
								#endif
								c++;
								u8Length--;
							}
						}
					}
					else
					{
						/* Change the message already allocated on the buffer*/
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage200ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage200ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
					break;


				case 500:/*500ms*/
					u8Status = FindIndexOfMessage(stCanMessage500ms,u32Identifier,&u8Index);
					if(u8Status != true)
					{
						u8Status = FindIndexOfEmptyMessage(stCanMessage500ms,&u8Index);
						if(u8Status != true)
						{
							/* No empty message to allocate a new one*/
						}
						else
						{
							/* Allocate this new message on the buffer*/
							stCanMessage500ms[u8Index].stCan.identifier = u32Identifier;
							stCanMessage500ms[u8Index].stCan.flags = u8Flags;
							stCanMessage500ms[u8Index].stCan.data_length_code = u8Length;
							c = 0;
							while(u8Length > 0)
							{
								stCanMessage500ms[u8Index].stCan.data[c] = cData[c];
								#if DEBUG_IO
								ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage500ms[u8Index].stCan.data[c]);
								#endif
								c++;
								u8Length--;
							}
						}
					}
					else
					{
						/* Change the message already allocated on the buffer*/
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage500ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage500ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
					break;

				case 1000:/*1000ms*/
					u8Status = FindIndexOfMessage(stCanMessage1000ms,u32Identifier,&u8Index);
					if(u8Status != true)
					{
						u8Status = FindIndexOfEmptyMessage(stCanMessage1000ms,&u8Index);
						if(u8Status != true)
						{
							/* No empty message to allocate a new one*/
						}
						else
						{
							/* Allocate this new message on the buffer*/
							stCanMessage1000ms[u8Index].stCan.identifier = u32Identifier;
							stCanMessage1000ms[u8Index].stCan.flags = u8Flags;
							stCanMessage1000ms[u8Index].stCan.data_length_code = u8Length;
							c = 0;
							while(u8Length > 0)
							{
								stCanMessage1000ms[u8Index].stCan.data[c] = cData[c];
								#if DEBUG_IO
								ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage1000ms[u8Index].stCan.data[c]);
								#endif
								c++;
								u8Length--;
							}
						}
					}
					else
					{
						/* Change the message already allocated on the buffer*/
						c = 0;
						while(u8Length > 0)
						{
							stCanMessage1000ms[u8Index].stCan.data[c] = cData[c];
							#if DEBUG_IO
							ESP_LOGI(IO_TASK_TAG,"\r\nIO:CanData=0x%02x\r\n",stCanMessage1000ms[u8Index].stCan.data[c]);
							#endif
							c++;
							u8Length--;
						}	
					}
					break;

				default:
					break;
		}
	}
	return(boError);
}

//////////////////////////////////////////////
//
//
//     TaskIo_TransmitCustomCanMessage
//
//
//////////////////////////////////////////////
static unsigned char ucCustomCANMsg = false;
void TaskIo_TransmitCustomCanMessage(tstCanMessage *pst,unsigned char ucSize)
{
	#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"TaskIo_TransmitCustomCanMessage\r\n");
	#endif

	/*tstCanMessage *pst = stCanMessage0ms;*/
	/*unsigned char ucSize = sizeof stCanMessage0ms / sizeof stCanMessage0ms[0];*/


	for(unsigned char i = 0; i <ucSize; i++)
	{
		/*Check whether can message exists*/
		if(pst[i].stCan.identifier != 0)
		{
			if (can_transmit(&pst[i].stCan, pdMS_TO_TICKS(10)) == ESP_OK)
			{
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);
				#if DEBUG_IO
				ESP_LOGI(IO_TASK_TAG,"CAN TX ID:0x%04X\r\n",pst[i].stCan.identifier);
				ESP_LOGI(IO_TASK_TAG,"CAN TX DATA:");
				#endif

				#if DEBUG_IO
				for (unsigned int u16 = 0; u16 < pst[i].stCan.data_length_code; u16++)
				{
					ESP_LOGI(IO_TASK_TAG,"0x%02X ",pst[i].stCan.data[u16]);
				}
				#endif
			}
			else
			{
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 1);
				#if DEBUG_IO
				ESP_LOGI(IO_TASK_TAG,"CAN TX FAILED:0x%04X\r\n",pst[i].stCan.identifier);
				#endif

				can_clear_transmit_queue();
				can_clear_receive_queue();
				can_stop();
				can_initiate_recovery();
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);
				can_start();
			}
		}
	}
}

unsigned char TaskIo_TransmitEventCanMessage(sMessageType *psMessage)
{
	unsigned char boError = true;

	ucCustomCANMsg = true;
#if 0
	#if 0
	ESP_LOGI(IO_TASK_TAG,"IO:TX EVENT CAN MESSAGE\r\n");
	#endif

	tstCanMessage *pst = stCanMessage0ms;

	unsigned char ucSize = sizeof stCanMessage0ms / sizeof stCanMessage0ms[0];

	for(unsigned char i = 0; i <ucSize; i++)
	{
		/*Check whether can message exists*/
		if(pst[i].stCan.identifier != 0)
		{
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"CAN PERIOD:%d\r\n",(int)(pst[i].u32Period));
			#endif
			if (can_transmit(&pst[i].stCan, pdMS_TO_TICKS(1)) == ESP_OK)
			{
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);
				#if DEBUG_IO
				ESP_LOGI(IO_TASK_TAG,"CAN TX ID:0x%04X\r\n",pst[i].stCan.identifier);
				ESP_LOGI(IO_TASK_TAG,"CAN TX DATA:");
				#endif

				#if DEBUG_IO
				for (unsigned int u16 = 0; u16 < pst[i].stCan.data_length_code; u16++)
				{
					ESP_LOGI(IO_TASK_TAG,"0x%02X ",pst[i].stCan.data[u16]);
				}
				#endif
			}
			else
			{
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 1);
				#if DEBUG_IO
				ESP_LOGI(IO_TASK_TAG,"CAN TX FAILED:0x%04X\r\n",pst[i].stCan.identifier);
				#endif

				can_clear_transmit_queue();
				can_clear_receive_queue();
				can_stop();
				can_initiate_recovery();
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);
				can_start();
			}
		}
	}
#endif
	return(boError);
}


//////////////////////////////////////////////
//
//
//     TaskIo_TransmitGenericCanMessage
//
//
//////////////////////////////////////////////
void TaskIo_TransmitGenericCanMessage(tstCanMessage stCanMessage[])
{
	#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"IO:TX GENERIC CAN MESSAGE\r\n");
	#endif

	/*Check whether can message exists*/
	for( unsigned int i = 0; i < SIZE_OF_STRUCT; i++)
	{
		if(stCanMessage[i].stCan.identifier != 0)
		{
			#if DEBUG_IO
			ESP_LOGI(IO_TASK_TAG,"CAN INDEX:%d\r\n",i);
			ESP_LOGI(IO_TASK_TAG,"CAN PERIOD:%d\r\n",(int)stCanMessage[i].u32Period);
			#endif
			if (can_transmit(&stCanMessage[i].stCan, pdMS_TO_TICKS(10)) == ESP_OK)
			{
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);
				#if DEBUG_IO
				ESP_LOGI(IO_TASK_TAG,"CAN TX ID:0x%04X\r\n",stCanMessage[i].stCan.identifier);
				ESP_LOGI(IO_TASK_TAG,"CAN TX DATA:");
				#endif

				#if DEBUG_IO
				for (unsigned int u16 = 0; u16 < stCanMessage[i].stCan.data_length_code; u16++)
				{
					ESP_LOGI(IO_TASK_TAG,"0x%02X ",stCanMessage[i].stCan.data[u16]);
				}
				#endif
			}
			else
			{
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 1);
				#if DEBUG_IO
				ESP_LOGI(IO_TASK_TAG,"CAN TX FAILED:0x%04X\r\n",stCanMessage[i].stCan.identifier);
				#endif

				can_clear_transmit_queue();
				can_clear_receive_queue();
				can_stop();
				can_initiate_recovery();
				gpio_set_level(GPIO_OUTPUT_GSM_DIAG, 0);
				can_start();


			}
			stCanMessage[i].u32Timeout = 0;
		}
	}
}
//////////////////////////////////////////////
//
//
//              TaskIo_Error
//
//
//////////////////////////////////////////////
unsigned char TaskIo_Error(sMessageType *psMessage)
{
	unsigned char boError = true;

#if DEBUG_IO
	ESP_LOGI(IO_TASK_TAG,"IO:ERROR\r\n");
#endif

	stIoMsg.ucSrc = SRC_IO;
	stIoMsg.ucDest = SRC_IO;
	stIoMsg.ucEvent = EVENT_IO_INIT;
	xQueueSend( xQueueIo, ( void * )&stIoMsg, 0);
	return(boError);
}
//////////////////////////////////////////////
//
//
//              TaskIo_IgnoreEvent
//
//
//////////////////////////////////////////////
unsigned char TaskIo_IgnoreEvent(sMessageType *psMessage)
{
    unsigned char boError = false;
    return(boError);
}
	
//////////////////////////////////////////////
//
//
//             Io State Machine
//
//
//////////////////////////////////////////////
static sStateMachineType const gasTaskIo_Idling[] =
{
    /* Event        Action routine      Next state */
    //  State specific transitions
	{EVENT_IO_INIT,	          TaskIo_Init,                 TASKIO_INITIALIZING,           	TASKIO_IDLING        				},
	{EVENT_IO_ERROR,	      TaskIo_Error,				   TASKIO_IDLING,  			  		TASKIO_IDLING						},
    {EVENT_IO_NULL,           TaskIo_IgnoreEvent,          TASKIO_IDLING,					TASKIO_IDLING						}
};


static sStateMachineType const gasTaskIo_Initializing[] =
{
    /* Event        Action routine      Next state */
    //  State specific transitions
	{EVENT_IO_ADD_CAN_MESSAGE,	      	TaskIo_AddCanMessage,			TASKIO_INITIALIZING,  			TASKIO_INITIALIZING					},
	{EVENT_IO_TRANSMIT_EVENT_CAN_MESSAGE, TaskIo_TransmitEventCanMessage, TASKIO_INITIALIZING,  			TASKIO_INITIALIZING				},
	{EVENT_IO_ERROR,	      			TaskIo_Error,				   	TASKIO_INITIALIZING,  			TASKIO_INITIALIZING					},
    {EVENT_IO_NULL,           			TaskIo_IgnoreEvent,          	TASKIO_INITIALIZING,			TASKIO_INITIALIZING					}
};

static sStateMachineType const * const gpasTaskIo_StateMachine[] =
{
    gasTaskIo_Idling,
	gasTaskIo_Initializing
};

void vTaskIo( void *pvParameters )
{

	static unsigned long u32Timeout = 0;
	for( ;; )
	{
		Elapsed_Time1 = xTaskGetTickCount();
		TaskIo_ReadIo();
		/*TaskIo_TransmitEventCanMessage(stCanMessage0ms);*/

		if(u32Timeout % 20 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage20ms);
		}
		if(u32Timeout % 50 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage50ms);
		}
		if(u32Timeout % 60 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage60ms);
		}
		if(u32Timeout % 100 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage100ms);
		}
		if(u32Timeout % 160 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage160ms);
		}		
		if(u32Timeout % 200 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage200ms);
			if (ucCustomCANMsg != false){
				TaskIo_TransmitCustomCanMessage(stCanMessage0ms,sizeof stCanMessage0ms / sizeof stCanMessage0ms[0]);
			}

		}		
		if(u32Timeout % 500 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage500ms);
		}		
		if(u32Timeout % 1000 == 0)
		{
			TaskIo_TransmitGenericCanMessage(stCanMessage1000ms);
		}		
		
		if( xQueueReceive( xQueueIo, &(stIoMsg ),0 ) )
		{
            (void)eEventHandler ((unsigned char)SRC_IO,gpasTaskIo_StateMachine[ucCurrentStateIo], &ucCurrentStateIo, &stIoMsg);
		}
		u32Timeout += TASK_PERIOD;

		vTaskDelayUntil(&Elapsed_Time1, TASK_PERIOD / portTICK_PERIOD_MS);

	}
}


