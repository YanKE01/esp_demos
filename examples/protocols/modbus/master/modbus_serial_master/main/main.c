#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "mbcontroller.h"
#include "esp_log.h"
#include "stddef.h"

static const char *TAG = "MODBUS";

typedef struct {
    uint16_t input_value1;
    uint16_t input_value2;
    uint16_t input_value3;
    uint16_t input_value4;
    uint16_t input_value5;
} input_reg_params_t;

typedef struct {
    uint8_t coil_value1;
} coil_reg_params_t;

typedef struct {
    uint16_t holding_value1;
    uint16_t holding_value2;
    uint16_t holding_value3;
    uint16_t holding_value4;
    uint16_t holding_value5;
} holding_reg_params_t;

enum {
    CID_INPUT = 0,
    CID_COIL,
    CID_HOLDING,
};

input_reg_params_t input_reg_params;     /*!< 输入寄存器 */
coil_reg_params_t coil_reg_params;       /*!< 线圈 */
holding_reg_params_t holding_reg_params; /*!< 保持寄存器 */

#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field) ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
#define HOLDING_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))

#define OPTS(min_val, max_val, step_val)                   \
    {                                                      \
        .opt1 = min_val, .opt2 = max_val, .opt3 = step_val \
    }

/**
 * @brief device paramters
 *
 */
const mb_parameter_descriptor_t device_parameters[] = {
    {
        CID_INPUT, (const char *)("INPUT"), (const char *)(""), 1, MB_PARAM_INPUT, 0x00, 5,
        INPUT_OFFSET(input_value1), PARAM_TYPE_U16, sizeof(uint16_t), OPTS(0, 0, 0), PAR_PERMS_READ_WRITE_TRIGGER
    },
    {
        CID_COIL, (const char *)("COIL"), (const char *)(""), 1, MB_PARAM_COIL, 0x00, 16,
        COIL_OFFSET(coil_value1), PARAM_TYPE_U8, sizeof(uint8_t), OPTS(0, 0, 0), PAR_PERMS_READ_WRITE_TRIGGER
    },
    {
        CID_HOLDING, (const char *)("HOLDING"), (const char *)(""), 1, MB_PARAM_HOLDING, 0x00, 5,
        HOLDING_OFFSET(holding_value1), PARAM_TYPE_U16, sizeof(uint16_t), OPTS(0, 0, 0), PAR_PERMS_READ_WRITE_TRIGGER
    },
};

char *device_parameters_name[] = {"INPUT", "COIL", "HOLDING"};
uint16_t device_parameters_count = sizeof(device_parameters) / sizeof(device_parameters[0]);

const mb_parameter_descriptor_t *param_descriptor = NULL;

/**
 * @brief 获取定义的parameters中的数据
 *
 * @param param_descriptor
 * @return void*
 */
static void *master_get_param_data(const mb_parameter_descriptor_t *param_descriptor)
{
    assert(param_descriptor != NULL);
    void *instance_ptr = NULL;
    if (param_descriptor->param_offset != 0) {
        switch (param_descriptor->mb_param_type) {
        case MB_PARAM_INPUT:
            instance_ptr = ((void *)&input_reg_params + param_descriptor->param_offset - 1);
            break;
        case MB_PARAM_COIL:
            instance_ptr = ((void *)&coil_reg_params + param_descriptor->param_offset - 1);
            break;
        case MB_PARAM_HOLDING:
            instance_ptr = ((void *)&holding_reg_params + param_descriptor->param_offset - 1);
            break;
        default:
            instance_ptr = NULL;
            break;
        }
    } else {
        ESP_LOGE(TAG, "Wrong parameter offset for CID #%d", param_descriptor->cid);
        assert(instance_ptr != NULL);
    }
    return instance_ptr;
}

/**
 * @brief mdobus read input reg
 *
 * @return esp_err_t
 */
esp_err_t modbus_master_read_input_reg()
{
    esp_err_t err = mbc_master_get_cid_info(CID_INPUT, &param_descriptor);
    if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
        void *temp_data_ptr = master_get_param_data(param_descriptor);
        assert(temp_data_ptr);
        uint8_t type = 0;
        uint16_t value[5] = {0};
        err = mbc_master_get_parameter(CID_INPUT, (char *)param_descriptor->param_key, (uint8_t *)&value, &type);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Input: %d %d %d %d %d", value[0], value[1], value[2], value[3], value[4]);
        }
    }

    return err;
}

/**
 * @brief modbus read holding reg
 *
 * @return esp_err_t
 */
esp_err_t modbus_master_read_holding_reg()
{
    esp_err_t err = mbc_master_get_cid_info(CID_HOLDING, &param_descriptor);
    if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
        void *temp_data_ptr = master_get_param_data(param_descriptor);
        assert(temp_data_ptr);
        uint8_t type = 0;
        uint16_t value[5] = {0};
        err = mbc_master_get_parameter(CID_HOLDING, (char *)param_descriptor->param_key, (uint8_t *)&value, &type);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Holding: %d %d %d %d %d", value[0], value[1], value[2], value[3], value[4]);
        }
    }

    return err;
}

/**
 * @brief modbus read coil reg
 *
 * @return esp_err_t
 */
esp_err_t modbus_master_read_coil_reg()
{
    esp_err_t err = mbc_master_get_cid_info(CID_COIL, &param_descriptor);
    if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
        void *temp_data_ptr = master_get_param_data(param_descriptor);
        assert(temp_data_ptr);
        uint8_t type = 0;
        uint8_t value[2] = {0};
        err = mbc_master_get_parameter(CID_COIL, (char *)param_descriptor->param_key, (uint8_t *)&value, &type);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Coil: %d %d", (*(uint8_t *)&value[0]), *(uint8_t *)&value[1]);
        }
    }
    return err;
}

void app_main(void)
{
    mb_communication_info_t comm = {
        .port = UART_NUM_2,
        .mode = MB_MODE_RTU,
        .baudrate = 115200,
        .parity = MB_PARITY_NONE,
    };
    void *master_handler = NULL;
    ESP_ERROR_CHECK(mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler));

    if (master_handler == NULL) {
        ESP_LOGI(TAG, "Modbus intilization failed");
    }
    ESP_ERROR_CHECK(mbc_master_setup((void *)&comm));

    // 设置UART
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 9, 8, -1, UART_PIN_NO_CHANGE));

    // 启动Modbus
    ESP_ERROR_CHECK(mbc_master_start());
    ESP_ERROR_CHECK(uart_set_mode(UART_NUM_2, UART_MODE_UART));

    vTaskDelay(100 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mbc_master_set_descriptor(&device_parameters[0], sizeof(device_parameters) / sizeof(device_parameters[0])));

    while (1) {
        modbus_master_read_coil_reg();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
