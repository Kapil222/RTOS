
/* Play music from Bluetooth device

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "bluetooth_service.h"
#include "mp3_decoder.h"
#include "http_stream.h"
#include "filter_resample.h"
#include "fatfs_stream.h"
#include "periph_sdcard.h"
#include "string.h"

/*------------Header file for UART--------------------*/
#include "driver/uart.h"
#include "driver/gpio.h"
/*****************************************************/

//#define TXD_PIN (GPIO_NUM_4)
//#define RXD_PIN (GPIO_NUM_5)
#define ECHO_TEST_TXD  (GPIO_NUM_4)
#define ECHO_TEST_RXD  (GPIO_NUM_5)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)


#define BUF_SIZE 100000//28
static const int RX_BUF_SIZE = 1024;
char uartRecData[2500]= {0};
char at_command[500]= {0};
int WR_STATUS = 0;

char *buffer[2048] = {0};

int first= 0;
xSemaphoreHandle l = 0;

static const char *TAG = "BLUETOOTH_SOURCE";
static QueueHandle_t uart0_queue;


static const char *mp3_file[] = {
    //"/sdcard/222.mp3",
    "/sdcard/first.mp3",
    "/sdcard/second.mp3",
    "/sdcard/third.mp3"

};
// more files may be added and `MP3_FILE_COUNT` will reflect the actual count
#define MP3_FILE_COUNT sizeof(mp3_file)/sizeof(char*)

#define CURRENT 0
#define NEXT    1
#define NEWFILE     2
static FILE *get_file(int next_file)
{
    static FILE *file;
    static int file_index = 0;
    if (next_file != CURRENT) {
        // advance to the next file
        if (++file_index > MP3_FILE_COUNT - 1) {
            file_index = 0;
        }
        if (file != NULL) {
            fclose(file);
            file = NULL;
        }
        ESP_LOGI(TAG, "[ * ] File index %d", file_index);
    }
    // return a handle to the current file
    if (file == NULL) {
        file = fopen(mp3_file[file_index], "r");
        if (!file) {
            ESP_LOGE(TAG, "Error opening file");
            return NULL;
        }
    }
    return file;
}

/*
 * Callback function to feed audio data stream from sdcard to mp3 decoder element
 */
static int my_sdcard_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    FILE *f = get_file(CURRENT);
    AUDIO_NULL_CHECK(TAG, f, return AEL_IO_FAIL);
    int read_len = fread(buf, 1, len, f);
    if (read_len == 0) {
        read_len = AEL_IO_DONE;
    }
    return read_len;
}



static void echo_task()
{
  int nextsong= 0;

  audio_pipeline_handle_t pipeline;
  audio_element_handle_t mp3_decoder, bt_stream_writer, fatfs_stream_reader; //http_stream_reader, ;

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
  }

  ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
  // Initialize peripherals management
  esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
  esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

  tcpip_adapter_init();

  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  ESP_LOGI(TAG, "[ 1 ] Create Bluetooth service");
  bluetooth_service_cfg_t bt_cfg = {
      .device_name = "ESP-ADF-SOURCE",
      .mode = BLUETOOTH_A2DP_SOURCE,
      .remote_name = CONFIG_BT_REMOTE_NAME,
  };
  bluetooth_service_start(&bt_cfg);
  ESP_LOGI(TAG, "[1.1] Get Bluetooth stream");
  bt_stream_writer = bluetooth_service_create_stream();
  // Initialize SD Card peripheral
  audio_board_sdcard_init(set);
  ESP_LOGI(TAG, "[3.1] Create fatfs stream to read data from sdcard");
  fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
  fatfs_cfg.type = AUDIO_STREAM_READER;
  fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
  ESP_LOGI(TAG, "[ 3 ] Create mp3 decoder to decode mp3 file");
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_decoder = mp3_decoder_init(&mp3_cfg);
  audio_element_set_read_cb(mp3_decoder, my_sdcard_read_cb, NULL);
  ESP_LOGI(TAG, "[4.3] Create resample filter");
  rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
  audio_element_handle_t rsp_handle = rsp_filter_init(&rsp_cfg);
  ESP_LOGI(TAG, "[ 4 ] Create audio pipeline for BT Source");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  pipeline = audio_pipeline_init(&pipeline_cfg);
  ESP_LOGI(TAG, "[4.1] Register all elements to audio pipeline");
  //audio_pipeline_register(pipeline, fatfs_stream_reader, "http");
  audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
  audio_pipeline_register(pipeline, rsp_handle, "filter");
  audio_pipeline_register(pipeline, bt_stream_writer,   "bt");
  ESP_LOGI(TAG, "[4.2] Link it together fileread-->mp3_decoder-->bt_stream_writer");
  audio_pipeline_link(pipeline, (const char *[]) {"mp3", "filter", "bt"}, 3);
  ESP_LOGI(TAG, "[4.3] Set up  uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s)");
  ESP_LOGI(TAG, "[ 5 ] Start and wait for Wi-Fi network");
  ESP_LOGI(TAG, "[5.1] Create Bluetooth peripheral");
  esp_periph_handle_t bt_periph = bluetooth_service_create_periph();
  ESP_LOGI(TAG, "[5.2] Start Bluetooth peripheral");
  esp_periph_start(set, bt_periph);
  ESP_LOGI(TAG, "[ 6 ] Set up  event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
  ESP_LOGI(TAG, "[6.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(pipeline, evt);
  ESP_LOGI(TAG, "[6.2] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);
  ESP_LOGI(TAG, "[ 7 ] Start audio_pipeline");
  audio_pipeline_run(pipeline);
  ESP_LOGI(TAG, "[ 8 ] Listen for all pipeline events");
  for (;;) {
    //https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3
      /* Handle event interface messages from pipeline
         to set music info and to advance to the next song
      */
      audio_event_iface_msg_t msg;
      //ESP_LOGE(TAG, "[*****] msg command : %", msg.cmd);
      esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
      if (ret != ESP_OK) {
          ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
          continue;
      }
      if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
          // Set music info for a new song to be played
          if (msg.source == (void *) mp3_decoder
              && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
              audio_element_info_t music_info = {0};
              audio_element_getinfo(mp3_decoder, &music_info);
              ESP_LOGI(TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                       music_info.sample_rates, music_info.bits, music_info.channels);
              audio_element_setinfo(bt_stream_writer, &music_info);
              nextsong=1;
              //rsp_filter_set_src_info(rsp_handle, music_info.sample_rates, music_info.channels);
              continue;
          }
              if(msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
              audio_element_state_t el_state = audio_element_get_state(bt_stream_writer);
              ESP_LOGI(TAG, "[ * ] Inside AEL MSG CMD REPORT");
              if (nextsong==1){//(el_state == AEL_STATE_FINISHED) {
                  ESP_LOGI(TAG, "[ * ] Finished, advancing to the next song");
                  audio_pipeline_stop(pipeline);
                  audio_pipeline_wait_for_stop(pipeline);
                  if(first==0){
                    get_file(NEXT);
                    first= 1;
                  }
                  if(first==1){
                    get_file(CURRENT);
                    first=0;
                  }
                  audio_pipeline_run(pipeline);
                  nextsong=0;
              }
              continue;
          }
      }

  }
}
void clip1(uint8_t *ptr, uint8_t* start, uint8_t* end, uint32_t leng){
    //static jmp_buf s_jumpBuffer;
    uint8_t * ptr_trav = NULL;
    uint8_t * startpoint = NULL;
    uint8_t * endpoint = NULL;
    ptr_trav = ptr;
    int countout=0;
    int first=0;
    int write=0;
    while(endpoint==NULL && countout<(leng+100)){
        countout++;
        //find startpoint
        if(first==0){
          if(*ptr_trav==start[0] && *(ptr_trav+1)==start[1] && *(ptr_trav+2)==start[2] && *(ptr_trav+3)==start[3] && *(ptr_trav+4)==start[4]){
              startpoint= ptr_trav;
              first=1;
              write=1;
          }
        }else{
          if(*ptr_trav==end[0] && *(ptr_trav+1)==end[1] && *(ptr_trav+2)==end[2] && *(ptr_trav+3)==end[3] && *(ptr_trav+4)==end[4]){
              endpoint= ptr_trav+5;
              while(*endpoint!='\r' && *(endpoint+1)!= '\n'){
                endpoint+=1;
              }
              endpoint+=4;
              write=2;
          }
        }
        if(countout<leng+100){
          ptr_trav+= 1;
        }
    }
    if(write==2){
      memcpy (startpoint,endpoint,(leng+100-countout));
    }
}


void clip2(uint8_t *ptr, uint8_t* start, uint8_t* end, uint32_t leng){
  uint8_t * ptr_trav = NULL;
  uint8_t * startpoint = NULL;
  uint8_t * endpoint = NULL;
  ptr_trav = ptr;
  int countout=0;
  int first=0;
  int write=0;
  while(countout<leng+1){
      countout++;
      //find startpoint
      if(first==0){
        if(*ptr_trav==start[0] && *(ptr_trav+1)==start[1] && *(ptr_trav+2)==start[2] && *(ptr_trav+3)==start[3] && *(ptr_trav+4)==start[4]){
            startpoint= ptr_trav;
            first=1;
            write=1;
        }
      }else{
        if(*ptr_trav==end[0] && *(ptr_trav+1)==end[1] && *(ptr_trav+2)==end[2] && *(ptr_trav+3)==end[3] && *(ptr_trav+4)==end[4]){
            endpoint= ptr_trav+5;
            while(*endpoint!='\r' && *(endpoint+1)!= '\n'){
              endpoint+=1;
            }
            endpoint+=2;
            write=2;
        }
      }
      if(countout<leng+100){
        ptr_trav+= 1;
      }

    if(write==2){
      memcpy(startpoint,endpoint,(leng+1-countout));
      write=0;
      first=0;
      ptr_trav = ptr;
      countout=0;
    }

  }

}




// uint8_t* clip2(uint8_t *ptr, uint8_t* start, uint8_t* end, uint32_t leng){
//   uint8_t * ptr_trav = NULL;
//   uint8_t * startpoint = NULL;
//   uint8_t * endpoint = NULL;
//   ptr_trav = ptr;
//   int countout=0;
//   int first=0;
//   int write=0;
//   while(endpoint==NULL && countout<leng+100){
//       countout++;
//       //find startpoint
//       if(first==0){
//         if(*ptr_trav==start[0] && *(ptr_trav+1)==start[1] && *(ptr_trav+2)==start[2] && *(ptr_trav+3)==start[3] && *(ptr_trav+4)==start[4]){
//             startpoint= ptr_trav;
//             first=1;
//             write=1;
//         }
//       }else{
//         if(*ptr_trav==end[0] && *(ptr_trav+1)==end[1] && *(ptr_trav+2)==end[2] && *(ptr_trav+3)==end[3] && *(ptr_trav+4)==end[4]){
//             endpoint= ptr_trav+5;
//             while(*endpoint!='\r' && *(endpoint+1)!= '\n'){
//               endpoint+=1;
//             }
//             endpoint+=2;
//             write=2;
//         }
//       }
//       if(countout<leng+100){
//         ptr_trav+= 1;
//       }
//   }
//   if(write==2){
//     memcpy (startpoint,endpoint,(leng+100-countout));
//   }
//   return startpoint;
// }





void remove_da(uint8_t* ptr){
  uint8_t * ptr_trav = NULL;
  uint8_t * startpoint = NULL;
  uint8_t * endpoint = NULL;
  int countout=0;
  int write=0;
  ptr_trav= ptr;
  while(endpoint==NULL && countout<200000){
    if(*ptr_trav==0x0d && *(ptr_trav+1)==0x0a){
        startpoint= ptr_trav;
        endpoint= ptr_trav+2;
        write= 1;

    }
    countout++;
    if(countout<200000){
      ptr_trav=ptr_trav+1;
      //printf("%c ", *ptr_trav);
    }
    if(write==1){
      memmove(startpoint,endpoint,200000);
      //write=0;
    }
  }
}




void atcommand(char* command, char* response1, char* response2, int timeout){
    int i, readbyte= 0;
    uint32_t len= strlen(command);
    char* cstr;
    char ch;
    uart_event_t event;
    int datalen;
    uint32_t count_1, count_2 = 0;
    uint8_t* looper;
    //uint8_t* ptr_trav = NULL;


    ESP_LOGI(TAG, "\n\n\n\[ * ] AT Command: %s\n\n\n", command);
    /*If we receive GET command  opn file, read data and write to sd card*/
    if(strstr(command, "GET")){
      uint8_t *data = (uint8_t *) malloc(200000*sizeof(uint8_t));
      memset(data, 0, 200000);
        FILE *fa = fopen("/sdcard/first.mp3", "wb");
        ESP_LOGI(TAG, "\n\n\n\[ * ] Inside GET handler******\n\n\n");
        uart_write_bytes(UART_NUM_1, command, len);
        len = uart_read_bytes(UART_NUM_1, (uint8_t*)data, 200000, 3000/portTICK_RATE_MS);
        ESP_LOGI(TAG, "\n\n\n\[ * ] Read bytes %d******\n\n\n", len);
        //data;//+780;
        clip1(data, (uint8_t*)"GET /", (uint8_t*)"lengt", len);
        //looper=data;
        ESP_LOGI(TAG, "[*] AT command |%s||%d||%s|",command, readbyte, uartRecData);

        clip2(data, (uint8_t*)"\r\n+CH", (uint8_t*)"PACT:", len);

        ESP_LOGI(TAG, "[*] Data cleaned");

        fwrite(data, sizeof(uint8_t), len, fa);

        readbyte+= len;
      fclose(fa);
      free(data);
      }
  else{
      memset(uartRecData,0, 2500);
      uint8_t *data = (uint8_t *) malloc(2490);
      uart_write_bytes(UART_NUM_1, command, len);
      while(!( strstr(uartRecData, response1) ||  strstr(uartRecData, response2) ))
        {
          memset(data, 0, 2490);
          readbyte += uart_read_bytes(UART_NUM_1, (uint8_t*)data, 2490, 1/portTICK_RATE_MS);
          if(readbyte> 0){
            strcat(uartRecData, (char*)data);
          }
        }free(data);
      ESP_LOGI(TAG, "[*] AT command |%s||%d||%s|",command, readbyte, uartRecData);
    }


}


void getsong(char* songname, char* link){
  uint32_t len;
  int readbyte= 0;
  int i;
  char* cstr;
  char ch;
  uart_event_t event;
  int datalen;
  uint32_t count_1, count_2 = 0;
  uint8_t* looper;


  atcommand("\r\nAT+CHTTPACT=\"stream.apps.enmobi.io\",80\r\n",  "REQUEST", "ERROR", 5);
  memset(at_command,0, 500);
  sprintf(at_command, "GET %s HTTP/1.1\nHost: stream.apps.enmobi.io\n\n%c", link, 0x1A);

  ESP_LOGI(TAG, "\n\n\n\[ * ] AT Command: %s\n\n\n", at_command);
  /*If we receive GET command  opn file, read data and write to sd card*/
  if(strstr(at_command, "GET")){
    uint8_t *data = (uint8_t *) malloc(200000*sizeof(uint8_t));
    memset(data, 0, 200000);
      FILE *fa = fopen(songname, "wb");
      ESP_LOGI(TAG, "\n\n\n\[ * ] Inside GET handler******\n\n\n");
      uart_write_bytes(UART_NUM_1, at_command, strlen(at_command));
      len = uart_read_bytes(UART_NUM_1, (uint8_t*)data, 200000, 3000/portTICK_RATE_MS);
      ESP_LOGI(TAG, "\n\n\n\[ * ] Read bytes %d******\n\n\n", len);
      //data;//+780;
      clip1(data, (uint8_t*)"GET /", (uint8_t*)"lengt", len);
      //looper=data;
      ESP_LOGI(TAG, "[*] AT command |%s||%d||%s|",songname, readbyte, uartRecData);

      clip2(data, (uint8_t*)"\r\n+CH", (uint8_t*)"PACT:", len);

      ESP_LOGI(TAG, "[*] Data cleaned");

      fwrite(data, sizeof(uint8_t), len, fa);

      readbyte+= len;
    fclose(fa);
    free(data);
    }

}

void app_main(void)
{
    //uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE);
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
    //uart_driver_delete(UART_NUM_0);
    //uart_driver_delete(UART_NUM_0);
    xTaskCreate(echo_task, "uart_echo_task", 5096, NULL, configMAX_PRIORITIES, NULL);


            /* FILE *ff= fopen("/sdcard/new.mp3", "wb");
             fclose(ff);*/
     atcommand("\r\nAT+IPREX=921600\r\n",  "OK", "ERROR", 5);
     vTaskDelay(100);

     //getsong("/sdcard/first.mp3", "/dataset-files/tp6kxasopb-91899807-d3f5-46ba-a88f-376a1e81f951/StreamFile08bb8562-50f6-4643-a817-2064442cbdec.mp3");
     //getsong("/sdcard/second.mp3", "/dataset-files/tp6dazs3yh-50def7f3-41a5-4296-9d0a-9b6c728763e2/StreamFilea67469b1-74df-43f2-97eb-4c88cb3bebfe.mp3");
     //getsong("/sdcard/third.mp3", "/dataset-files/tp65ovxvbm-55c37383-9ee0-45af-933e-e2168380d5c1/StreamFilec1523d21-5598-4c1f-86c1-456587653d87.mp3");




    while (1) {
        atcommand("\r\nAT\r\n","OK", "ERROR", 2);
        vTaskDelay(1000);
        atcommand("\r\nAT+CHTTPSSTOP\r\n","OK", "ERROR", 2);
        vTaskDelay(1000);
        atcommand("\r\nAT+CHTTPCLSE\r\n","OK", "ERROR", 2);
      }
    }
