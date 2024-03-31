#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "mbcontroller.h"
#include "esp_log.h"

#define MB_READ_MASK (MB_EVENT_INPUT_REG_RD | MB_EVENT_HOLDING_REG_RD | MB_EVENT_DISCRETE_RD | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK (MB_READ_MASK | MB_WRITE_MASK)

mb_communication_info_t comm_info;
mb_param_info_t reg_info;
static const char *TAG = "MODBUS";

typedef struct
{
    uint8_t discrete_input_0 : 1;
    uint8_t discrete_input_1 : 1;
} discrete_reg_params_t;

typedef struct
{
    uint8_t coils_port_0 : 1;
    uint8_t coils_port_1 : 1;
} coil_reg_params_t;

typedef struct
{
    uint16_t holding_0;
    uint16_t holding_1;
} holding_reg_params_t;

discrete_reg_params_t discrete_reg_params = {0, 1};
coil_reg_params_t coil_reg_params = {1, 1};
holding_reg_params_t holding_reg_params = {1234, 7788};

void app_main(void)
{
    void *mbc_slave_handler = NULL;
    mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler);

    comm_info.mode = MB_MODE_RTU;
    comm_info.slave_addr = 1;
    comm_info.port = UART_NUM_2; // 串口2
    comm_info.baudrate = 115200;
    comm_info.parity = MB_PARITY_NONE;
    ESP_ERROR_CHECK(mbc_slave_setup((void *)&comm_info));

    // 离散输入寄存器
    mb_register_area_descriptor_t descrete_reg_area = {
        .type = MB_PARAM_DISCRETE,
        .start_offset = 0x0000,
        .address = (void *)&discrete_reg_params,
        .size = sizeof(discrete_reg_params_t),
    };
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(descrete_reg_area));

    // 线圈
    mb_register_area_descriptor_t coil_reg_area = {
        .type = MB_PARAM_COIL,
        .start_offset = 0x0000,
        .address = (void *)&coil_reg_params,
        .size = sizeof(coil_reg_params_t),
    };
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(coil_reg_area));

    // 保持寄存器
    mb_register_area_descriptor_t holding_reg_area = {
        .type = MB_PARAM_HOLDING,
        .start_offset = 0x0000,
        .address = (void *)&holding_reg_params,
        .size = sizeof(holding_reg_params_t),
    };
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 启动slave
    ESP_ERROR_CHECK(mbc_slave_start());

    // 设置UART
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 9, 8, -1, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(UART_NUM_2, UART_MODE_UART));

    while (1)
    {
        mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);

        if (event & MB_EVENT_DISCRETE_RD)
        {
            // 主机读取离散输入寄存器
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, 10));
            ESP_LOGI(TAG, "DISCRETE READ");
        }
        else if (event & MB_EVENT_COILS_RD)
        {
            // 主机读取线圈
            ESP_LOGI(TAG, "COIL READ");
        }
        else if (event & MB_EVENT_COILS_WR)
        {
            // 主机写线圈
            ESP_LOGI(TAG, "COIL WRITE");
            ESP_LOGI(TAG, "Coil: %d %d", coil_reg_params.coils_port_0, coil_reg_params.coils_port_1);
        }
        else if (event & MB_EVENT_HOLDING_REG_RD)
        {
            // 主机读取保持寄存器
            ESP_LOGI(TAG, "HOLDING READ");
        }
        else if (event & MB_EVENT_HOLDING_REG_WR)
        {
            // 主机写保持寄存器
            ESP_LOGI(TAG, "HOLDING WRITE");
            ESP_LOGI(TAG, "Holding: %d %d", holding_reg_params.holding_0, holding_reg_params.holding_1);
        }
    }

    ESP_ERROR_CHECK(mbc_slave_destroy());
}
