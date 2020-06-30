/* ESP HTTP SERVER Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include "freertos/queue.h"
#include "defines.h"
#include "Sd.h"
#include "Io.h"

#include "http_server.h"

static const char *TAG = "HTTP_SERVER";
sMessageType stHttpSrvMsg;
extern char cConfigHttpRxBuffer[RX_BUF_SIZE];
extern tstConfiguration stConfigData;
extern tstCanMessage stCanMessage20ms[];
extern tstCanMessage stCanMessage50ms[];
extern tstCanMessage stCanMessage60ms[];
extern tstCanMessage stCanMessage100ms[];
extern tstCanMessage stCanMessage160ms[];
extern tstCanMessage stCanMessage200ms[];
extern tstCanMessage stCanMessage500ms[];
extern tstCanMessage stCanMessage1000ms[];

/* An HTTP GET handler */
static esp_err_t Read_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /*static char cKey[128];*/

            memset(cConfigHttpRxBuffer,0x00,sizeof(cConfigHttpRxBuffer));

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ap", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => ap=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    char *cWebpage,*cHtml;

    cWebpage = (char*) malloc (12288);
	cHtml = (char*) malloc (256);
    strcpy(cWebpage,"<!DOCTYPE html> <html>\n");
    strcat(cWebpage,"<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n");
    strcat(cWebpage,"<title>CUV WEB SERBER</title>\n");
    strcat(cWebpage,"<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;font-size: 14px}\n");
	strcat(cWebpage,"input[type=text] {");
	  strcat(cWebpage,"width: 75%;");
	  strcat(cWebpage,"padding: 4px 4px;");
	  strcat(cWebpage,"margin: 4px 4px;");
	  strcat(cWebpage,"box-sizing: border-box;");
	  strcat(cWebpage,"border: none;");
	  strcat(cWebpage,"background-color: #34495e;");
	  strcat(cWebpage,"color: white;");
 	  strcat(cWebpage,"display: inline-block;");
  	  strcat(cWebpage,"float: left;");
  	  strcat(cWebpage,"clear: left;\n");
	strcat(cWebpage,"}");
	strcat(cWebpage,"label{\n");
	strcat(cWebpage,"display: inline-block;\n");
	strcat(cWebpage,"float: left;\n");
	strcat(cWebpage,"clear: left;\n");
	strcat(cWebpage,"width: 100px;\n");
	strcat(cWebpage,"text-align: left;\n");
	strcat(cWebpage,"padding: 4px 4px;\n");
	strcat(cWebpage,"}");
	strcat(cWebpage,"body{margin-top: 20px;} h1 {color: #444444;margin: 20px auto 30px;} h3 {color: #444444;margin-bottom: 20px;}\n");
	strcat(cWebpage,".button {\n");
	strcat(cWebpage,"width: 60px;\n");
	strcat(cWebpage,"background-color: #3498db;\n");
	strcat(cWebpage,"border: none;\n");
	strcat(cWebpage,"color: white;\n");
	strcat(cWebpage,"padding: 4px 4px;\n");
	strcat(cWebpage,"text-decoration: none;\n");
	strcat(cWebpage,"font-size: 14px;\n");
	strcat(cWebpage,"margin: 2px 2px;\n");
	strcat(cWebpage,"cursor: pointer;\n");
	strcat(cWebpage,"border-radius: 4px;\n");
	strcat(cWebpage,"display: inline-block;\n");
	strcat(cWebpage,"float: left;\n");
	strcat(cWebpage,"}\n");
	strcat(cWebpage,".button-on {background-color: #3498db;}\n");
	strcat(cWebpage,".button-on:active {background-color: #2980b9;}\n");
	strcat(cWebpage,".button-off {background-color: #34495e;}\n");
	strcat(cWebpage,".button-off:active {background-color: #2c3e50;}\n");
	strcat(cWebpage,".font-custom-type {font-size: 14px;color: #888;margin-bottom: 0px;}\n");
	strcat(cWebpage,"</style>\n");
	strcat(cWebpage,"</head>\n");
	strcat(cWebpage,"<body>\n");
	strcat(cWebpage,"<h1>CUV Web Server</h1>\n");
	strcat(cWebpage,"<h3>Using SOFT AP Mode</h3>");
	strcat(cWebpage,"<label for=\"sw\">SW:</label>");


	sprintf(cHtml,"<input type=\"text\" id=\"sw\" name=\"sw\" value=\"%s\">",SOFTWARE_VERSION);
	strcat(cWebpage,cHtml);

	/*********************************************************************************/
	/* ESP_02*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"esp_02\">ESP_02:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"esp_02\" name=\"esp_02\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			  stCanMessage20ms[0].stCan.data[0],\
																			  stCanMessage20ms[0].stCan.data[1],\
																			  stCanMessage20ms[0].stCan.data[2],\
																			  stCanMessage20ms[0].stCan.data[3],\
																			  stCanMessage20ms[0].stCan.data[4],\
																			  stCanMessage20ms[0].stCan.data[5],\
																			  stCanMessage20ms[0].stCan.data[6],\
																			  stCanMessage20ms[0].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* ESP_21*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"esp_21\">ESP_21:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"esp_21\" name=\"esp_21\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			  stCanMessage20ms[1].stCan.data[0],\
																			  stCanMessage20ms[1].stCan.data[1],\
																			  stCanMessage20ms[1].stCan.data[2],\
																			  stCanMessage20ms[1].stCan.data[3],\
																			  stCanMessage20ms[1].stCan.data[4],\
																			  stCanMessage20ms[1].stCan.data[5],\
																			  stCanMessage20ms[1].stCan.data[6],\
																			  stCanMessage20ms[1].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* MOTOR_04*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"motor_04\">MOTOR_04:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"motor_04\" name=\"motor_04\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			  stCanMessage20ms[2].stCan.data[0],\
																			  stCanMessage20ms[2].stCan.data[1],\
																			  stCanMessage20ms[2].stCan.data[2],\
																			  stCanMessage20ms[2].stCan.data[3],\
																			  stCanMessage20ms[2].stCan.data[4],\
																			  stCanMessage20ms[2].stCan.data[5],\
																			  stCanMessage20ms[2].stCan.data[6],\
																			  stCanMessage20ms[2].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* MOTOR_29*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"motor_29\">MOTOR_29:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"motor_29\" name=\"motor_29\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			  stCanMessage20ms[3].stCan.data[0],\
																			  stCanMessage20ms[3].stCan.data[1],\
																			  stCanMessage20ms[3].stCan.data[2],\
																			  stCanMessage20ms[3].stCan.data[3],\
																			  stCanMessage20ms[3].stCan.data[4],\
																			  stCanMessage20ms[3].stCan.data[5],\
																			  stCanMessage20ms[3].stCan.data[6],\
																			  stCanMessage20ms[3].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Airbag_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Airbag_01\">AIRBAG_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Airbag_01\" name=\"Airbag_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage50ms[0].stCan.data[0],\
																			stCanMessage50ms[0].stCan.data[1],\
																			stCanMessage50ms[0].stCan.data[2],\
																			stCanMessage50ms[0].stCan.data[3],\
																			stCanMessage50ms[0].stCan.data[4],\
																			stCanMessage50ms[0].stCan.data[5],\
																			stCanMessage50ms[0].stCan.data[6],\
																			stCanMessage50ms[0].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* ESP_24*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"esp_24\">ESP_24:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"esp_24\" name=\"esp_24\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage50ms[1].stCan.data[0],\
																			stCanMessage50ms[1].stCan.data[1],\
																			stCanMessage50ms[1].stCan.data[2],\
																			stCanMessage50ms[1].stCan.data[3],\
																			stCanMessage50ms[1].stCan.data[4],\
																			stCanMessage50ms[1].stCan.data[5],\
																			stCanMessage50ms[1].stCan.data[6],\
																			stCanMessage50ms[1].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Gateway_73*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Gateway_73\">GATEWAY_73:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Gateway_73\" name=\"Gateway_73\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage50ms[2].stCan.data[0],\
																			stCanMessage50ms[2].stCan.data[1],\
																			stCanMessage50ms[2].stCan.data[2],\
																			stCanMessage50ms[2].stCan.data[3],\
																			stCanMessage50ms[2].stCan.data[4],\
																			stCanMessage50ms[2].stCan.data[5],\
																			stCanMessage50ms[2].stCan.data[6],\
																			stCanMessage50ms[2].stCan.data[7]);
	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Kombi_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Kombi_01\">KOMBI_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Kombi_01\" name=\"Kombi_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage50ms[3].stCan.data[0],\
																			stCanMessage50ms[3].stCan.data[1],\
																			stCanMessage50ms[3].stCan.data[2],\
																			stCanMessage50ms[3].stCan.data[3],\
																			stCanMessage50ms[3].stCan.data[4],\
																			stCanMessage50ms[3].stCan.data[5],\
																			stCanMessage50ms[3].stCan.data[6],\
																			stCanMessage50ms[3].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* ACC_02*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"acc_02\">ACC_02:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"acc_02\" name=\"acc_02\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage60ms[0].stCan.data[0],\
																			stCanMessage60ms[0].stCan.data[1],\
																			stCanMessage60ms[0].stCan.data[2],\
																			stCanMessage60ms[0].stCan.data[3],\
																			stCanMessage60ms[0].stCan.data[4],\
																			stCanMessage60ms[0].stCan.data[5],\
																			stCanMessage60ms[0].stCan.data[6],\
																			stCanMessage60ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Allrad_03*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Allrad_03\">ALLRAD_03:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Allrad_03\" name=\"Allrad_03\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[0].stCan.data[0],\
																			stCanMessage100ms[0].stCan.data[1],\
																			stCanMessage100ms[0].stCan.data[2],\
																			stCanMessage100ms[0].stCan.data[3],\
																			stCanMessage100ms[0].stCan.data[4],\
																			stCanMessage100ms[0].stCan.data[5],\
																			stCanMessage100ms[0].stCan.data[6],\
																			stCanMessage100ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* BEM_02*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"bem_02\">BEM_02:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"bem_02\" name=\"bem_02\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[1].stCan.data[0],\
																			stCanMessage100ms[1].stCan.data[1],\
																			stCanMessage100ms[1].stCan.data[2],\
																			stCanMessage100ms[1].stCan.data[3],\
																			stCanMessage100ms[1].stCan.data[4],\
																			stCanMessage100ms[1].stCan.data[5],\
																			stCanMessage100ms[1].stCan.data[6],\
																			stCanMessage100ms[1].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");



	/*********************************************************************************/
	/* FDR_04*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"fdr_04\">FDR_04:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"fdr_04\" name=\"fdr_04\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[2].stCan.data[0],\
																			stCanMessage100ms[2].stCan.data[1],\
																			stCanMessage100ms[2].stCan.data[2],\
																			stCanMessage100ms[2].stCan.data[3],\
																			stCanMessage100ms[2].stCan.data[4],\
																			stCanMessage100ms[2].stCan.data[5],\
																			stCanMessage100ms[2].stCan.data[6],\
																			stCanMessage100ms[2].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Gateway_71*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"gateway_71\">GATEWAY_71:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"gateway_71\" name=\"gateway_71\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[3].stCan.data[0],\
																			stCanMessage100ms[3].stCan.data[1],\
																			stCanMessage100ms[3].stCan.data[2],\
																			stCanMessage100ms[3].stCan.data[3],\
																			stCanMessage100ms[3].stCan.data[4],\
																			stCanMessage100ms[3].stCan.data[5],\
																			stCanMessage100ms[3].stCan.data[6],\
																			stCanMessage100ms[3].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Gateway_72*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"gateway_72\">GATEWAY_72:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"gateway_72\" name=\"gateway_72\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[4].stCan.data[0],\
																			stCanMessage100ms[4].stCan.data[1],\
																			stCanMessage100ms[4].stCan.data[2],\
																			stCanMessage100ms[4].stCan.data[3],\
																			stCanMessage100ms[4].stCan.data[4],\
																			stCanMessage100ms[4].stCan.data[5],\
																			stCanMessage100ms[4].stCan.data[6],\
																			stCanMessage100ms[4].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Motor_14*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"motor_14\">MOTOR_14:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"motor_14\" name=\"motor_14\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[5].stCan.data[0],\
																			stCanMessage100ms[5].stCan.data[1],\
																			stCanMessage100ms[5].stCan.data[2],\
																			stCanMessage100ms[5].stCan.data[3],\
																			stCanMessage100ms[5].stCan.data[4],\
																			stCanMessage100ms[5].stCan.data[5],\
																			stCanMessage100ms[5].stCan.data[6],\
																			stCanMessage100ms[5].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Motor_26*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"motor_26\">MOTOR_26:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"motor_26\" name=\"motor_26\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[6].stCan.data[0],\
																			stCanMessage100ms[6].stCan.data[1],\
																			stCanMessage100ms[6].stCan.data[2],\
																			stCanMessage100ms[6].stCan.data[3],\
																			stCanMessage100ms[6].stCan.data[4],\
																			stCanMessage100ms[6].stCan.data[5],\
																			stCanMessage100ms[6].stCan.data[6],\
																			stCanMessage100ms[6].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Klemmen_Status_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"klemmen_status_01\">KLEMMEN_STATUS_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"klemmen_ktatus_01\" name=\"klemmen_status_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage100ms[7].stCan.data[0],\
																			stCanMessage100ms[7].stCan.data[1],\
																			stCanMessage100ms[7].stCan.data[2],\
																			stCanMessage100ms[7].stCan.data[3],\
																			stCanMessage100ms[7].stCan.data[4],\
																			stCanMessage100ms[7].stCan.data[5],\
																			stCanMessage100ms[7].stCan.data[6],\
																			stCanMessage100ms[7].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* WBA_03*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"wba_03\">WBA_03:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"wba_03\" name=\"wba_03\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage160ms[0].stCan.data[0],\
																			stCanMessage160ms[0].stCan.data[1],\
																			stCanMessage160ms[0].stCan.data[2],\
																			stCanMessage160ms[0].stCan.data[3],\
																			stCanMessage160ms[0].stCan.data[4],\
																			stCanMessage160ms[0].stCan.data[5],\
																			stCanMessage160ms[0].stCan.data[6],\
																			stCanMessage160ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Dimmung_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"dimmung_01\">DIMMUNG_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"dimmung_01\" name=\"dimmung_01\" value=\"%02X%02X%02X%02X%02X%02X%02X%02X\">",\
																			stCanMessage200ms[0].stCan.data[0],\
																			stCanMessage200ms[0].stCan.data[1],\
																			stCanMessage200ms[0].stCan.data[2],\
																			stCanMessage200ms[0].stCan.data[3],\
																			stCanMessage200ms[0].stCan.data[4],\
																			stCanMessage200ms[0].stCan.data[5],\
																			stCanMessage200ms[0].stCan.data[6],\
																			stCanMessage200ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* ESP_10*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"esp_10\">ESP_10:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"esp_10\" name=\"esp_10\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[1].stCan.data[0],\
																			stCanMessage200ms[1].stCan.data[1],\
																			stCanMessage200ms[1].stCan.data[2],\
																			stCanMessage200ms[1].stCan.data[3],\
																			stCanMessage200ms[1].stCan.data[4],\
																			stCanMessage200ms[1].stCan.data[5],\
																			stCanMessage200ms[1].stCan.data[6],\
																			stCanMessage200ms[1].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Gateway_77*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Gateway_77\">GATEWAY_77:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Gateway_77\" name=\"Gateway_77\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[2].stCan.data[0],\
																			stCanMessage200ms[2].stCan.data[1],\
																			stCanMessage200ms[2].stCan.data[2],\
																			stCanMessage200ms[2].stCan.data[3],\
																			stCanMessage200ms[2].stCan.data[4],\
																			stCanMessage200ms[2].stCan.data[5],\
																			stCanMessage200ms[2].stCan.data[6],\
																			stCanMessage200ms[2].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");


	/*********************************************************************************/
	/* Klima_12*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Klima_12\">KLIMA_12:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Klima_12\" name=\"Klima_12\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[3].stCan.data[0],\
																			stCanMessage200ms[3].stCan.data[1],\
																			stCanMessage200ms[3].stCan.data[2],\
																			stCanMessage200ms[3].stCan.data[3],\
																			stCanMessage200ms[3].stCan.data[4],\
																			stCanMessage200ms[3].stCan.data[5],\
																			stCanMessage200ms[3].stCan.data[6],\
																			stCanMessage200ms[3].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");


	/*********************************************************************************/
	/* NMH_AMP*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"nmh_amp\">NMH_AMP:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"nmh_ampP\" name=\"nmh_amp\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[4].stCan.data[0],\
																			stCanMessage200ms[4].stCan.data[1],\
																			stCanMessage200ms[4].stCan.data[2],\
																			stCanMessage200ms[4].stCan.data[3],\
																			stCanMessage200ms[4].stCan.data[4],\
																			stCanMessage200ms[4].stCan.data[5],\
																			stCanMessage200ms[4].stCan.data[6],\
																			stCanMessage200ms[4].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* NMH_Gateway*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"nmh_gateway\">NMH_GATEWAY:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"nmh_gateway\" name=\"nmh_gateway\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[5].stCan.data[0],\
																			stCanMessage200ms[5].stCan.data[1],\
																			stCanMessage200ms[5].stCan.data[2],\
																			stCanMessage200ms[5].stCan.data[3],\
																			stCanMessage200ms[5].stCan.data[4],\
																			stCanMessage200ms[5].stCan.data[5],\
																			stCanMessage200ms[5].stCan.data[6],\
																			stCanMessage200ms[5].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* VIN_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"vin_01\">VIN_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"vin_01\" name=\"vin_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[6].stCan.data[0],\
																			stCanMessage200ms[6].stCan.data[1],\
																			stCanMessage200ms[6].stCan.data[2],\
																			stCanMessage200ms[6].stCan.data[3],\
																			stCanMessage200ms[6].stCan.data[4],\
																			stCanMessage200ms[6].stCan.data[5],\
																			stCanMessage200ms[6].stCan.data[6],\
																			stCanMessage200ms[6].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* WFS_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"wfs_01\">WFS_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"wfs_01\" name=\"wfs_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage200ms[7].stCan.data[0],\
																			stCanMessage200ms[7].stCan.data[1],\
																			stCanMessage200ms[7].stCan.data[2],\
																			stCanMessage200ms[7].stCan.data[3],\
																			stCanMessage200ms[7].stCan.data[4],\
																			stCanMessage200ms[7].stCan.data[5],\
																			stCanMessage200ms[7].stCan.data[6],\
																			stCanMessage200ms[7].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Motor_18*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Motor_18\">MOTOR_18:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Motor_18\" name=\"Motor_18\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage500ms[0].stCan.data[0],\
																			stCanMessage500ms[0].stCan.data[1],\
																			stCanMessage500ms[0].stCan.data[2],\
																			stCanMessage500ms[0].stCan.data[3],\
																			stCanMessage500ms[0].stCan.data[4],\
																			stCanMessage500ms[0].stCan.data[5],\
																			stCanMessage500ms[0].stCan.data[6],\
																			stCanMessage500ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Motor_25*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Motor_25\">MOTOR_25:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Motor_25\" name=\"Motor_25\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage500ms[0].stCan.data[0],\
																			stCanMessage500ms[0].stCan.data[1],\
																			stCanMessage500ms[0].stCan.data[2],\
																			stCanMessage500ms[0].stCan.data[3],\
																			stCanMessage500ms[0].stCan.data[4],\
																			stCanMessage500ms[0].stCan.data[5],\
																			stCanMessage500ms[0].stCan.data[6],\
																			stCanMessage500ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Blinkmodi_02*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Blinkmodi_02\">BLINKMODI_02:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Blinkmodi_02\" name=\"Blinkmodi_02\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[0].stCan.data[0],\
																			stCanMessage1000ms[0].stCan.data[1],\
																			stCanMessage1000ms[0].stCan.data[2],\
																			stCanMessage1000ms[0].stCan.data[3],\
																			stCanMessage1000ms[0].stCan.data[4],\
																			stCanMessage1000ms[0].stCan.data[5],\
																			stCanMessage1000ms[0].stCan.data[6],\
																			stCanMessage1000ms[0].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Diagnose_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Diagnose_01\">DIAGNOSE_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Diagnose_01\" name=\"Diagnose_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[1].stCan.data[0],\
																			stCanMessage1000ms[1].stCan.data[1],\
																			stCanMessage1000ms[1].stCan.data[2],\
																			stCanMessage1000ms[1].stCan.data[3],\
																			stCanMessage1000ms[1].stCan.data[4],\
																			stCanMessage1000ms[1].stCan.data[5],\
																			stCanMessage1000ms[1].stCan.data[6],\
																			stCanMessage1000ms[1].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Einheiten_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"Einheiten_01\">EINHEITEN_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"Einheiten_01\" name=\"Einheiten_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[2].stCan.data[0],\
																			stCanMessage1000ms[2].stCan.data[1],\
																			stCanMessage1000ms[2].stCan.data[2],\
																			stCanMessage1000ms[2].stCan.data[3],\
																			stCanMessage1000ms[2].stCan.data[4],\
																			stCanMessage1000ms[2].stCan.data[5],\
																			stCanMessage1000ms[2].stCan.data[6],\
																			stCanMessage1000ms[2].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* ESP_20*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"esp_20\">ESP_20:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"esp_20\" name=\"esp_20\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[3].stCan.data[0],\
																			stCanMessage1000ms[3].stCan.data[1],\
																			stCanMessage1000ms[3].stCan.data[2],\
																			stCanMessage1000ms[3].stCan.data[3],\
																			stCanMessage1000ms[3].stCan.data[4],\
																			stCanMessage1000ms[3].stCan.data[5],\
																			stCanMessage1000ms[3].stCan.data[6],\
																			stCanMessage1000ms[3].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Kombi_02*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"kombi_02\">KOMBI_02:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"kombi_02\" name=\"kombi_02\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[4].stCan.data[0],\
																			stCanMessage1000ms[4].stCan.data[1],\
																			stCanMessage1000ms[4].stCan.data[2],\
																			stCanMessage1000ms[4].stCan.data[3],\
																			stCanMessage1000ms[4].stCan.data[4],\
																			stCanMessage1000ms[4].stCan.data[5],\
																			stCanMessage1000ms[4].stCan.data[6],\
																			stCanMessage1000ms[4].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Kombi_03*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"kombi_03\">KOMBI_03:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"kombi_03\" name=\"kombi_03\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[5].stCan.data[0],\
																			stCanMessage1000ms[5].stCan.data[1],\
																			stCanMessage1000ms[5].stCan.data[2],\
																			stCanMessage1000ms[5].stCan.data[3],\
																			stCanMessage1000ms[5].stCan.data[4],\
																			stCanMessage1000ms[5].stCan.data[5],\
																			stCanMessage1000ms[5].stCan.data[6],\
																			stCanMessage1000ms[5].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");
	/*********************************************************************************/
	/* Kombi_08*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"kombi_08\">KOMBI_08:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"kombi_08\" name=\"kombi_08\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[6].stCan.data[0],\
																			stCanMessage1000ms[6].stCan.data[1],\
																			stCanMessage1000ms[6].stCan.data[2],\
																			stCanMessage1000ms[6].stCan.data[3],\
																			stCanMessage1000ms[6].stCan.data[4],\
																			stCanMessage1000ms[6].stCan.data[5],\
																			stCanMessage1000ms[6].stCan.data[6],\
																			stCanMessage1000ms[6].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");


	/*********************************************************************************/
	/* Motor_Code_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"motor_code_01\">MOTOR_CODE_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"motor_code_01\" name=\"motor_code_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[7].stCan.data[0],\
																			stCanMessage1000ms[7].stCan.data[1],\
																			stCanMessage1000ms[7].stCan.data[2],\
																			stCanMessage1000ms[7].stCan.data[3],\
																			stCanMessage1000ms[7].stCan.data[4],\
																			stCanMessage1000ms[7].stCan.data[5],\
																			stCanMessage1000ms[7].stCan.data[6],\
																			stCanMessage1000ms[7].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");


	/*********************************************************************************/
	/* Personalisierung_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"personalisierung_01\">PERSONALISIERUNG_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"personalisierung_01\" name=\"personalisierung_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[8].stCan.data[0],\
																			stCanMessage1000ms[8].stCan.data[1],\
																			stCanMessage1000ms[8].stCan.data[2],\
																			stCanMessage1000ms[8].stCan.data[3],\
																			stCanMessage1000ms[8].stCan.data[4],\
																			stCanMessage1000ms[8].stCan.data[5],\
																			stCanMessage1000ms[8].stCan.data[6],\
																			stCanMessage1000ms[8].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");

	/*********************************************************************************/
	/* Personalisierung_02*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"personalisierung_02\">PERSONALISIERUNG_02:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"personalisierung_02\" name=\"personalisierung_02\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[9].stCan.data[0],\
																			stCanMessage1000ms[9].stCan.data[1],\
																			stCanMessage1000ms[9].stCan.data[2],\
																			stCanMessage1000ms[9].stCan.data[3],\
																			stCanMessage1000ms[9].stCan.data[4],\
																			stCanMessage1000ms[9].stCan.data[5],\
																			stCanMessage1000ms[9].stCan.data[6],\
																			stCanMessage1000ms[9].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");


	/*********************************************************************************/
	/* Systeminfo_01*/
	/*********************************************************************************/
	strcat(cWebpage,"<form action=\"/response\"  method=\"get\">");
	strcat(cWebpage,"<label for=\"systeminfo_01\">SYSTEMINFO_01:</label>");
	sprintf(cHtml,"<input type=\"text\" id=\"systeminfo_01\" name=\"systeminfo_01\" value=\"0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\">",\
																			stCanMessage1000ms[10].stCan.data[0],\
																			stCanMessage1000ms[10].stCan.data[1],\
																			stCanMessage1000ms[10].stCan.data[2],\
																			stCanMessage1000ms[10].stCan.data[3],\
																			stCanMessage1000ms[10].stCan.data[4],\
																			stCanMessage1000ms[10].stCan.data[5],\
																			stCanMessage1000ms[10].stCan.data[6],\
																			stCanMessage1000ms[10].stCan.data[7]);

	strcat(cWebpage,cHtml);
	strcat(cWebpage,"<button class=\"button button-off\">Send</button>");
	strcat(cWebpage,"</form>");


    strcat(cWebpage,"</body>\n");
	strcat(cWebpage,"</html>\n");


    const char* resp_str = (const char*) /*req->user_ctx*/cWebpage;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }

    free (cWebpage);
    free (cHtml);

    return ESP_OK;
}



static esp_err_t Response_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            static char cKey[128];
            const char *cCAN = "CAN:";

            memset(cConfigHttpRxBuffer,0x00,sizeof(cConfigHttpRxBuffer));


            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "dimmung_01", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => dimmung_01=%s", param);

                if(strlen(param) == 16){
                strcpy(cConfigHttpRxBuffer,"CAN:");

                memset(cKey,0x00,sizeof(cKey));
                sprintf(cKey,"%ld",stCanMessage200ms[0].u32Period);
                strcat(cConfigHttpRxBuffer,cKey);
                strcat(cConfigHttpRxBuffer,",");

                memset(cKey,0x00,sizeof(cKey));
                sprintf(cKey,"%04X",stCanMessage200ms[0].stCan.identifier);
                strcat(cConfigHttpRxBuffer,cKey);
                strcat(cConfigHttpRxBuffer,",");

                strcat(cConfigHttpRxBuffer,param);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ap", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => ap=%s", param);

                if(strlen(param)> 0){
                const char *cWifiApName = "WIFIAPNAME:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cWifiApName);
                strcat(cKey,param);
                strcat(cKey,"\r\n");

                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }
            if (httpd_query_key_value(buf, "appass", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => appass=%s", param);

                if(strlen(param)> 0){
                const char *cWifiApPassword = "WIFIAPPASSWORD:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cWifiApPassword);
                strcat(cKey,param);
                strcat(cKey,"\r\n");


                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }
            if (httpd_query_key_value(buf, "log", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => log=%s", param);

                if(strlen(param)> 0){
                const char *cPeriodLogInSec = "PERIODLOGINSEC:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cPeriodLogInSec);
                strcat(cKey,param);
                strcat(cKey,"\r\n");


                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if (httpd_query_key_value(buf, "tx", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => tx=%s", param);

                if(strlen(param)> 0){
                const char *cPeriodTx = "MODEMPERIODTXINSEC:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cPeriodTx);
                strcat(cKey,param);
                strcat(cKey,"\r\n");

                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if (httpd_query_key_value(buf, "src", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => src=%s", param);

                if(strlen(param)> 0){
                const char *cAppSrc = "APPSOURCE:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cAppSrc);
                strcat(cKey,param);
                strcat(cKey,"\r\n");

                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if (httpd_query_key_value(buf, "apn", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => apn=%s", param);

                if(strlen(param)> 0){
                const char *cModemApn = "MODEMAPN:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cModemApn);
                strcat(cKey,param);
                strcat(cKey,"\r\n");


                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if (httpd_query_key_value(buf, "login", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => login=%s", param);

                if(strlen(param)> 0){
                const char *cModemLogin = "MODEMAPNLOGIN:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cModemLogin);
                strcat(cKey,param);
                strcat(cKey,"\r\n");


                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if (httpd_query_key_value(buf, "mdpass", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => mdpass=%s", param);

                if(strlen(param)> 0){
                const char *cModemPassword = "MODEMAPNPASSWORD:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cModemPassword);
                strcat(cKey,param);
                strcat(cKey,"\r\n");


                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if (httpd_query_key_value(buf, "sleep", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => sleep=%s", param);

                if(strlen(param)> 0){
                const char *cTimeToSleep = "TIMETOSLEEP:";
                memset(cKey,0,sizeof(cKey));
                strcpy(cKey,cTimeToSleep);
                strcat(cKey,param);
                strcat(cKey,"\r\n");


                strcat(cConfigHttpRxBuffer,"CONF:");
                strcat(cConfigHttpRxBuffer,cKey);
                ESP_LOGI(TAG, "%s\r\n", cConfigHttpRxBuffer);
                }
            }

            if(strlen(cConfigHttpRxBuffer) > strlen("CONF:")){
            /* Receive data over BT and pass it over to SD*/
            stHttpSrvMsg.ucSrc = SRC_HTTPSRV;
            stHttpSrvMsg.ucDest = SRC_IO;
            stHttpSrvMsg.ucEvent = EVENT_IO_ADD_CAN_MESSAGE;
            stHttpSrvMsg.pcMessageData = (char*)cConfigHttpRxBuffer;

			xQueueSend( xQueueIo, ( void * )&stHttpSrvMsg, NULL);
            }

        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}


/*
<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n\
<p>LED2 Status: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n\
*/

static const httpd_uri_t response = {
    .uri       = "/response",
    .method    = HTTP_GET,
    .handler   = Response_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  =
    	    "<!DOCTYPE html> <html>\n\
    	    <head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n\
    	    <title>CUV WEB SERBER</title>\n\
    	    <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n\
    		input[type=text] {\
    		  width: 50%;\
    		  padding: 12px 20px;\
    		  margin: 8px 0;\
    		  box-sizing: border-box;\
    		  border: none;\
    		  background-color: #34495e;\
    		  color: white;\
    		}\
    	    body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n\
    	    .button {display: block;width: 120px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n\
    	    .button-on {background-color: #3498db;}\n\
    	    .button-on:active {background-color: #2980b9;}\n\
    	    .button-off {background-color: #34495e;}\n\
    	    .button-off:active {background-color: #2c3e50;}\n\
    	    p {font-size: 18px;color: #888;margin-bottom: 0px;}\n\
    	    </style>\n\
    	    </head>\n\
    	    <body>\n\
    	    <h1>CUV Web Server</h1>\n\
    	    <h2>Using SOFT AP Mode</h2>\n\
    	    <h3>Requested Parameters have been recorded</h3>\n\
    	    </body>\n\
    		</html>\n"
};

static const httpd_uri_t Read = {
    .uri       = "/get",
    .method    = HTTP_GET,
    .handler   = Read_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;


    while (remaining > 0) {
    	ESP_LOGI(TAG, "remaining > 0 ");
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = "HTTP_POST"
};

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/echo");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else {
        ESP_LOGI(TAG, "Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &Read);
        httpd_register_uri_handler(req->handle, &echo);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t ctrl = {
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &Read);
        httpd_register_uri_handler(server, &response);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}


