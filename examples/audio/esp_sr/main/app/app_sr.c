#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"
#include "app_sr.h"
#include "esp_mn_speech_commands.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"
#include "string.h"
#include "hal_i2s.h"
#include "string.h"
#include "app_led.h"

static const char *TAG = "APP_SR";
static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static srmodel_list_t *models = NULL;
static uint8_t detect_flag = 0;
extern app_led_config_t app_led_config;

esp_err_t app_sr_init(char *model_partition_label)
{
    models = esp_srmodel_init(model_partition_label);
    char *wn_name = NULL;
    if (models != NULL) {
        for (int i = 0; i < models->num; i++) {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
                if (wn_name == NULL) {
                    wn_name = models->model_name[i];
                    ESP_LOGI(TAG, "The wakenet model: %s", wn_name);
                }
            }
        }
    } else {
        ESP_LOGI(TAG, "Please enable wakenet model and select wake word by menuconfig!");
        return ESP_FAIL;
    }

    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    afe_config.wakenet_init = true;
    afe_config.wakenet_model_name = wn_name;
    afe_config.voice_communication_init = false;
    afe_config.aec_init = false;
    afe_config.pcm_config.total_ch_num = 2;
    afe_config.pcm_config.mic_num = 1;
    afe_config.pcm_config.ref_num = 1;
    afe_data = afe_handle->create_from_config(&afe_config);

    return ESP_OK;
}

void app_sr_feed_task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = 2;
    assert(nch < feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (1) {
        size_t buffer_len = audio_chunksize * sizeof(int16_t) * feed_channel;
        hal_i2s_get_data(i2s_buff, buffer_len);
        afe_handle->feed(afe_data, i2s_buff);
    }
}

void app_sr_detect_task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
    ESP_LOGI(TAG, "multinet:%s\n", mn_name);

    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 3000);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data); // Add speech commands from sdkconfig
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    assert(mu_chunksize == afe_chunksize);

    esp_mn_commands_add(1000, "guan deng");      //  add command
    esp_mn_commands_add(1001, "kai deng");       //  add command
    esp_mn_commands_add(1002, "cai hong deng");  //  add command
    esp_mn_commands_add(1003, "huan yan se");    //  add command
    esp_mn_commands_add(1004, "you dian liang"); //  add command
    esp_mn_commands_add(1005, "you dian an");    //  add command
    esp_mn_commands_add(1006, "yi jia yi");      //  add command
    esp_mn_commands_add(1007, "zao shang hao");  //  add command

    esp_mn_commands_update();

    while (1) {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGI(TAG, "fetch error!");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "WAKEWORD DETECTED");
            multinet->clean(model_data); // clean all status of multinet
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            detect_flag = 1;
            ESP_LOGI(TAG, "AFE_FETCH_CHANNEL_VERIFIED, channel index: %d", res->trigger_channel_id);
        }

        if (detect_flag == 1) {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING) {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++) {
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, string:%s prob: %f", i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                    if (mn_result->command_id[i] >= 1000 && mn_result->command_id[i] <= 1007) {
                        // 个人指令
                        app_led_config.command = mn_result->command_id[i] - 1000;
                        continue;
                    }
                }

                ESP_LOGI(TAG, "listening ........");
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                ESP_LOGI(TAG, "timeout, string:%s", mn_result->string);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = 0;
                ESP_LOGI(TAG, "awaits to be waken up ........");
                continue;
            }
        }
    }
}

void app_sr_task_start()
{
    xTaskCreatePinnedToCore(&app_sr_feed_task, "feed_task", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&app_sr_detect_task, "detect_task", 8 * 1024, (void *)afe_data, 4, NULL, 1);
}
