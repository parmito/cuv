/*
 * Sd.c
 *
 *  Created on: 20/09/2018
 *      Author: danilo
 */
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <rom/spi_flash.h>

#include "defines.h"
#include "State.h"
#include "Gsm.h"
#include "Sd.h"
#include "http_client.h"
#include "Ble.h"


void vTaskSd( void *pvParameters );
unsigned char TaskSd_Writing(sMessageType *psMessage);
unsigned char TaskSd_IgnoreEvent(sMessageType *psMessage);

unsigned long u32TimeToSleep;
unsigned long u32TimeToWakeup;
unsigned long u32PeriodLog = 10;
unsigned long u32PeriodTx;
static unsigned char boBuzzerStatus;

char cWifiApName[RX_BUF_SIZE_REDUCED];
char cWifiApPassword[RX_BUF_SIZE_REDUCED];
char cApn[RX_BUF_SIZE_REDUCED];
char cApnLogin[RX_BUF_SIZE_REDUCED];
char cApnPassword[RX_BUF_SIZE_REDUCED];
char cHttpDns[RX_BUF_SIZE_REDUCED];
char cWebPage[RX_BUF_SIZE_REDUCED];


const char cConfigText[]= {"CONFIG/CONFIG.txt"};
const char cTimeToSleep[] = {"TS="};
const char cPeriodLog[]  = {"PL="};
const char cBuzzerStatus[]  = {"BZ="};
const char cWifiSettings[]  = {"WIFI="};
const char cHttpSettings[]  = {"HTTP="};
const char cPageUrl[]  = {"PAGE="};
const char cModemApn[]  = {"APN="};
const char cModemPeriodTx[] = {"PTX="};


const char cConfigData[]=
{
		"\
		TS=300\r\n\
		PL=30\r\n\
		BZ=OFF\r\n\
		WIFI=Iphone4_EXT,poliana90\r\n\
		HTTP=gpslogger.esy.es\r\n\
		PAGE=http://gpslogger.esy.es/pages/upload/index.php\r\n\
		APN=vodafone.br, , \r\n\
		PTX=120\r\n"
};

/*const char cConfigTimeToSleep[] = {"TS=120\r\n"};
const char cConfigPeriodLog[]  = {"PL=30\r\n"};
const char cConfigBuzzerStatus[]  = {"BZ=OFF\r\n"};
const char cConfigWifiSettings[]  = {"WIFI=Iphone4_EXT,poliana90\r\n"};
const char cConfigHttpSettings[]  = {"HTTP=gpslogger.esy.es\r\n"};
const char cConfigPageUrl[]  = {"PAGE=http://gpslogger.esy.es/pages/upload/index.php\r\n"};
const char cConfigModemApn[]  = {"APN=zap.vivo.com.br, , \r\n"};
const char cConfigModemPeriodTx[] = {"PTX=120\r\n"};*/


char szFilenameToBeRead[RX_BUF_SIZE+1];
char  cConfigAndData[RX_BUF_SIZE+1];
extern char szFilenameToBeWritten[RX_BUF_SIZE+1];
extern char cGpsDataCopiedToSd[RX_BUF_SIZE+1];
extern unsigned char ucCurrentStateGsm;

static long int liFilePointerPositionBeforeReading = 0;
static long int liFilePointerPositionAfterReading = 0;

/*extern tstIo stIo;*/
sMessageType stSdMsg;

static const char *SD_TASK_TAG = "SD_TASK";

//////////////////////////////////////////////
//
//
//              Sd_Init
//
//
//////////////////////////////////////////////
void SdInit(void)
{
	/*esp_log_level_set(SD_TASK_TAG, ESP_LOG_INFO);*/
#if 0
	xQueueSd = xQueueCreate(sdQUEUE_LENGTH,			/* The number of items the queue can hold. */
							sizeof( sMessageType ) );	/* The size of each item the queue holds. */
#endif

    xTaskCreate(vTaskSd, "vTaskSd", 1024*4, NULL, configMAX_PRIORITIES-1, NULL);
	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */


	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_SD;
	stSdMsg.ucEvent = EVENT_SD_INIT;
	xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Init
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Init(sMessageType *psMessage)
{
	unsigned char boError = true;

	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	char *pcChar;

    ESP_LOGI(SD_TASK_TAG, "INIT\r\n");
    ESP_LOGI(SD_TASK_TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        boError = false;
        return(boError);
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SD_TASK_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(SD_TASK_TAG, "Partition size: total: %d, used: %d", total, used);
    }

    /*********************************************
     *				DELETE FILE
     *********************************************/
#if 0
    if (remove("/spiffs/DATA_-100010000.TXT") == 0)
    	ESP_LOGI(SD_TASK_TAG,"Deleted successfully");
     else
    	 ESP_LOGI(SD_TASK_TAG,"Unable to delete the file");
#endif

#if 1
    /*********************************************
     *		READ FILES INSIDE FOLDER
     *********************************************/
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/spiffs/")) != NULL)
    {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL)
		{
		  ESP_LOGI(SD_TASK_TAG,"%s\n", ent->d_name);
		}
		closedir (dir);
    }
    else
    {
      /* could not open directory */
    	ESP_LOGE(SD_TASK_TAG, "Error opening directory");
        boError = false;
        return(boError);
    }
#endif

    FILE *f;

#if 0
    /*********************************************
     *			WRITING CONFIG FILE
     *********************************************/

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(SD_TASK_TAG, "Opening file");
    f = fopen("/spiffs/CONFIG.TXT", "w");
    if (f == NULL) {
        ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
        return(boError);
    }

    ESP_LOGI(SD_TASK_TAG, "\r\n%s\r\n",cConfigData);
    fprintf(f, "%s",cConfigData);
    fclose(f);
#endif

    /*********************************************
     *			READING CONFIG FILE
     *********************************************/
    // Open renamed file for reading
    ESP_LOGI(SD_TASK_TAG, "Reading Config file");
    f = fopen("/spiffs/CONFIG.TXT", "r");
    if (f == NULL)
    {
		ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		boError = false;
		free(cLocalBuffer);
		return(boError);
    }

    memset(cLocalBuffer,0,RX_BUF_SIZE+1);
	fread (cLocalBuffer,1,RX_BUF_SIZE,f);
	fclose(f);
	ESP_LOGI(SD_TASK_TAG, "Read from file: '%s'", cLocalBuffer);

	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_BLE;
	stSdMsg.ucEvent = (int)NULL;
	stSdMsg.pcMessageData = &cLocalBuffer[0];

	xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);

#if 0
    /*********************************************
     *			UNMOUNT SPIFFS
     *********************************************/
    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(SD_TASK_TAG, "SPIFFS unmounted");
#endif
    /* TimeToSleep*/
	pcChar = strstr (cLocalBuffer,cTimeToSleep);
	if(pcChar != NULL)
	{
		pcChar = pcChar + 3;
		if(pcChar != NULL)
		{
			u32TimeToSleep = atoi(pcChar);
		}
	}


	/* Period Log*/
	pcChar = strstr (cLocalBuffer,cPeriodLog);
	if(pcChar != NULL)
	{
		pcChar = pcChar + 3;
		if(pcChar != NULL)
		{
			u32PeriodLog = atoi(pcChar);
		}
	}


    /* Buzzer Status*/
	pcChar = strstr (cLocalBuffer,cBuzzerStatus);
	if(pcChar != NULL)
	{
		pcChar = pcChar + 3;
		if(pcChar != NULL)
		{
			if( (strcmp("ON\r\n",pcChar)) == 0)
			{
				boBuzzerStatus = true;
			}
			else
			{
				boBuzzerStatus = false;
			}
		}
	}

	/*Wifi Settings*/
	pcChar = strstr (cLocalBuffer,cWifiSettings);
	if(pcChar != NULL)
	{
		pcChar = pcChar + 5;
		pcChar = strtok((char*)pcChar,",");
		if(pcChar != NULL)
		{
			strcpy(cWifiApName,pcChar);
		}

		pcChar = strtok(NULL,"\r");
		if(pcChar != NULL)
		{
			strcpy(cWifiApPassword,pcChar);
		}
	}

	/*Http Settings*/
	pcChar = strtok (NULL,"=");
	pcChar = strtok (NULL,"\r");
	if(pcChar != NULL)
	{
		strcpy(cHttpDns,pcChar);

	}

	/* Page Url*/
	pcChar = strtok (NULL,"=");
	pcChar = strtok (NULL,"\r");
	if(pcChar != NULL)
	{
		strcpy(cWebPage,pcChar);
	}

	/* Modem Settings*/
	pcChar = strtok (NULL,"=");
	pcChar = strtok (NULL,",");
	if(pcChar != NULL)
	{
		strcpy(cApn,pcChar);
	}
	pcChar = strtok(NULL,",");
	if(pcChar != NULL)
	{
		strcpy(cApnLogin,pcChar);
	}

	pcChar = strtok(NULL,"\r");
	if(pcChar != NULL)
	{
		strcpy(cApnPassword,pcChar);
	}

	pcChar = strtok(NULL,"=");
	pcChar = strtok(NULL,"\r");
	if(pcChar != NULL)
	{
		u32PeriodTx = atoi(pcChar);
	}


    ESP_LOGI(SD_TASK_TAG, "\r\nu32TimeToSleep=%d\r\n",(int)u32TimeToSleep);
    ESP_LOGI(SD_TASK_TAG, "u32PeriodLog=%d\r\n",(int)u32PeriodLog);
    ESP_LOGI(SD_TASK_TAG, "WifiAPName=%s\r\nWifiPwd=%s\r\n",cWifiApName,cWifiApPassword);
    ESP_LOGI(SD_TASK_TAG, "cHttpDns=%s\r\n",cHttpDns);
    ESP_LOGI(SD_TASK_TAG, "cWebPage=%s\r\n",cWebPage);
    ESP_LOGI(SD_TASK_TAG, "cApn=%s\r\ncApnLogin=%s\r\ncApnPassword=%s\r\n",cApn,cApnLogin,cApnPassword);

    free(cLocalBuffer);

	/*stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_SD;
	stSdMsg.ucEvent = EVENT_SD_OPENING;
	xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);*/


    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Opening
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Opening(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<OPENING>>>>\r\n\r\n<<<<>>>>\r\n");
	#endif

    /*********************************************
     *		READ FILES INSIDE FOLDER
     *********************************************/
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir ("/spiffs/")) != NULL)
    {
        rewinddir (dir);

		/* print all the files and directories within directory */
		for(;;)
		{
			if((ent = readdir (dir)) != NULL)
			{
				ESP_LOGI(SD_TASK_TAG,"%s,FileNumber=%d\n", ent->d_name,ent->d_ino);

				if(strstr(ent->d_name,"DATA_") != NULL)
				{
					memset(cLocalBuffer,0,RX_BUF_SIZE+1);

					strcpy(cLocalBuffer,"/spiffs/");
					strcat(cLocalBuffer,ent->d_name);

					f = fopen((const char*)cLocalBuffer, "r");
				    if(f == NULL )
				    {
						ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
						boError = false;
				    }
				    else
				    {
					    fclose(f);
						strcpy(szFilenameToBeRead,cLocalBuffer);

						liFilePointerPositionBeforeReading = 0;
						liFilePointerPositionAfterReading = 0;

						stSdMsg.ucSrc = SRC_SD;
						stSdMsg.ucDest = SRC_SD;
						stSdMsg.ucEvent = EVENT_SD_READING;
						xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);
				    }
					break;
				}
			}
			else
			{
				ESP_LOGE(SD_TASK_TAG,"No more DATA files\n");
		        boError = false;

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_GSM;
				stSdMsg.ucEvent = EVENT_GSM_ENDING;
				xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);

				break;
			}
		}
		closedir (dir);
    }
    else
    {
      /* could not open directory */
    	ESP_LOGE(SD_TASK_TAG, "Error opening directory");
        boError = false;
    }
    free(cLocalBuffer);
	return(boError);
}


//////////////////////////////////////////////
//
//
//              TaskSd_Writing
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Writing(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<WRITING>>>>\r\n%s\r\n<<<<>>>>\r\n",szFilenameToBeWritten);
#endif


#if 0
    f = fopen((const char*)szFilenameToBeWritten, "w+" );
#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "DATA GPS:%s",cGpsDataCopiedToSd);
#endif
	fprintf(f,"%s",(const char*)"DANILO");
	fclose(f);

    f = fopen((const char*)szFilenameToBeWritten, "r" );
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	memset(cLocalBuffer,0,RX_BUF_SIZE+1);
	fread (cLocalBuffer,1,RX_BUF_SIZE,f);
	/*fgets (cLocalBuffer,RX_BUF_SIZE,f);*/

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "DATA:%s",cLocalBuffer);
#endif
	free(cLocalBuffer);
	(void)fclose(f);
#endif


	f = fopen(szFilenameToBeWritten, "a" );
	if(f != NULL)
	{
#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "Reading for update...");
#endif
    	int i;
		if((i = strlen(cGpsDataCopiedToSd))>0)
		{
		    ESP_LOGI(SD_TASK_TAG, "Qty:%d,%s",i,cGpsDataCopiedToSd);

			int i = fprintf(f,"%s",cGpsDataCopiedToSd);
			if(i > 0)
			{
				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Writing OK!");
				#endif
			}
			else
			{
				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Writing NOK!");
				#endif
			}
		}
		fclose(f);
	}
	else
	{
		f = fopen(szFilenameToBeWritten, "w");
		if(f != NULL)
		{
			if(strlen(cGpsDataCopiedToSd)>0)
			{
				int i = fprintf(f,"%s",cGpsDataCopiedToSd);
				if(i > 0)
				{
					#if DEBUG_SDCARD
					ESP_LOGI(SD_TASK_TAG, "Writing new file");
					#endif
				}
				else
				{
					#if DEBUG_SDCARD
					ESP_LOGE(SD_TASK_TAG, "Error writing new file");
					#endif
				}
			}
			fclose(f);
		}
	}
	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Reading
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Reading(sMessageType *psMessage)
{
	unsigned char boError = true;
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	FILE *f;

#if DEBUG_SDCARD
	ESP_LOGI(SD_TASK_TAG, "<<<<READING>>>>\r\n%s\r\n<<<<>>>>\r\n",szFilenameToBeRead);
#endif


	f = fopen(szFilenameToBeRead, "r");
	if(f != NULL)
	{
		#if DEBUG_SDCARD
			ESP_LOGI(SD_TASK_TAG, "Opening for Reading OK!");
		#endif

		for(;;)
		{
			fseek(f,liFilePointerPositionBeforeReading,SEEK_SET);

			#if DEBUG_SDCARD
			ESP_LOGI(SD_TASK_TAG, "PositionBeforeReading=%d\r\n",(int)liFilePointerPositionBeforeReading);
			#endif

			/*Reading file until finds \n or RX_BUF_SIZE, whichever happens first*/
			memset(cLocalBuffer,0,RX_BUF_SIZE+1);
			int i;
			if((fgets ( cLocalBuffer, RX_BUF_SIZE, f )) != NULL )
			{

				liFilePointerPositionAfterReading =  ftell (f);

				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "PositionAfterReading=%d\r\n",(int)liFilePointerPositionAfterReading);
				#endif

				i = strlen(cLocalBuffer);

				ESP_LOGI(SD_TASK_TAG, "Content of file with %d Bytes: %s",i, cLocalBuffer);
				/* Verify if file is OK*/
				if((cLocalBuffer[0] == 'S') && (cLocalBuffer[1] == '=') && (cLocalBuffer[i-1] == '\n') && (cLocalBuffer[i-2] == '\r'))
				{
					memset(cConfigAndData,0,RX_BUF_SIZE+1);
					strncpy(cConfigAndData,cLocalBuffer,i-2);

					if(ucCurrentStateGsm == TASKGSM_COMMUNICATING)
					{
						stSdMsg.ucSrc = SRC_SD;
						stSdMsg.ucDest = SRC_GSM;
						stSdMsg.ucEvent = EVENT_GSM_SEND_HTTPURL/*EVENT_GSM_SEND_HTTPPREPAREDATA*//*EVENT_GSM_SEND_HTTPPREPAREDATA*//*EVENT_GSM_SEND_START*//*EVENT_GSM_SEND_CIPSEND*//*EVENT_GSM_SEND_DATA*/;
						stSdMsg.pcMessageData = "";
						xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);
					}
					else
					{
						stSdMsg.ucSrc = SRC_SD;
						stSdMsg.ucDest = SRC_HTTPCLI;
						stSdMsg.ucEvent = EVENT_HTTPCLI_POST;
						stSdMsg.pcMessageData = "";
						xQueueSend( xQueueHttpCli, ( void * )&stSdMsg, 0);
					}

					fclose(f);
					break;
				}
				else
				{
					liFilePointerPositionBeforeReading = liFilePointerPositionAfterReading;
				}
			}
			else
			{
				#if DEBUG_SDCARD
					ESP_LOGE(SD_TASK_TAG, "No possible to retrieve data");
				#endif


				if(feof(f))
				{
					ESP_LOGI(SD_TASK_TAG, "End of File");


					int ret = remove(szFilenameToBeRead);

					if(ret == 0)
					{
						#if DEBUG_SDCARD
						ESP_LOGE(SD_TASK_TAG, "File deleted successfully");
						#endif
					 }
					else
					{
						#if DEBUG_SDCARD
						ESP_LOGE(SD_TASK_TAG, "Error: unable to delete the file");
						#endif
					}

					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_GSM;
					stSdMsg.ucEvent = EVENT_GSM_ENDING/*EVENT_GSM_LIST_SMS_MSG*/;
					xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);

					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_HTTPCLI;
					stSdMsg.ucEvent = EVENT_HTTPCLI_ENDING/*EVENT_GSM_LIST_SMS_MSG*/;
					xQueueSend( xQueueHttpCli, ( void * )&stSdMsg, 0);



				}

				fclose(f);
				break;
			}
		}
	}
	else
	{
		#if DEBUG_SDCARD
			ESP_LOGE(SD_TASK_TAG, "No more files...");
		#endif

		stSdMsg.ucSrc = SRC_SD;
		stSdMsg.ucDest = SRC_GSM;
		stSdMsg.ucEvent = EVENT_GSM_ENDING/*EVENT_GSM_LIST_SMS_MSG*/;
		xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);

		stSdMsg.ucSrc = SRC_SD;
		stSdMsg.ucDest = SRC_SD;
		stSdMsg.ucEvent = EVENT_SD_OPENING;
		xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);



	}
	free(cLocalBuffer);
	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Marking
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Marking(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;

#if DEBUG_SDCARD
	ESP_LOGI(SD_TASK_TAG, "<<<<MARKING>>>>\r\n");
#endif

	if((f = fopen(szFilenameToBeRead, "r+")) != NULL)
	{
		fseek(f,liFilePointerPositionBeforeReading,SEEK_SET);
		fputs("*",f);
		liFilePointerPositionBeforeReading = liFilePointerPositionAfterReading;
		fclose(f);

	}

	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_SD;
	stSdMsg.ucEvent = EVENT_SD_READING;
	xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);

	return(boError);
}



//////////////////////////////////////////////
//
//
//              TaskSd_IgnoreEvent
//
//
//////////////////////////////////////////////
unsigned char TaskSd_IgnoreEvent(sMessageType *psMessage)
{
	unsigned char boError = false;
	return(boError);
}

static sStateMachineType const gasTaskSd_Initializing[] =
{
        /* Event                            Action routine          Success state               Failure state*/
        //  State specific transitions
        {EVENT_SD_INIT,                     TaskSd_Init,            	TASKSD_INITIALIZING,        TASKSD_INITIALIZING },
		{EVENT_SD_OPENING, 					TaskSd_Opening,			 	TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
		{EVENT_SD_WRITING, 					TaskSd_Writing,	    	 	TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
		{EVENT_SD_READING, 					TaskSd_Reading,				TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
	   	{EVENT_SD_MARKING, 					TaskSd_Marking,				TASKSD_INITIALIZING,		TASKSD_INITIALIZING },
	    {EVENT_SD_NULL,                     TaskSd_IgnoreEvent,			TASKSD_INITIALIZING,		TASKSD_INITIALIZING }
};

static sStateMachineType const * const gpasTaskSd_StateMachine[] =
{
    gasTaskSd_Initializing
};

static unsigned char ucCurrentStateSd = TASKSD_INITIALIZING;

void vTaskSd( void *pvParameters )
{

	for( ;; )
	{
	    /*ESP_LOGI(SD_TASK_TAG, "Running...");*/

		if( xQueueReceive( xQueueSd, &( stSdMsg ),0 ) )
		{
            (void)eEventHandler ((unsigned char)SRC_SD,gpasTaskSd_StateMachine[ucCurrentStateSd], &ucCurrentStateSd, &stSdMsg);
		}

		vTaskDelay(500/portTICK_PERIOD_MS);
	}




}
