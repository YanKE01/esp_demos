#include "ui.h"
#include "math.h"
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WOUOUI";

list_page_t list_page[MAX_LIST_NUM] = {0};
input_state_t input_state = {0};
int list_page_index = -1;

void input_roll_cb()
{
    input_state.roll = 1;
}

void input_enter_cb()
{
    input_state.enter = 1;
}

void ui_add_page(list_page_t list)
{
    list_page[list.index] = list;
    list_page[list.index].init_flag = 0;
    list_page[list.index].list_element_count = 0;

    // 更新元素个数
    for (int i = 0; i < MAX_LIST_ELEMENT_NUM; i++) {
        if (list.list_element[i] != NULL) {
            list_page[list.index].list_element_count++;
        }
    }
    list_page_index = 0;
}

void animation(float *a, float *a_trg, int n)
{
    if (*a != *a_trg) {
        if (fabs(*a - *a_trg) < 0.15f) {
            *a = *a_trg;
        } else {
            *a += (*a_trg - *a) / (n / 10.0f);
        }
    }
}

void animation_pd(float *a, float *a_trg, float kp, float kd)
{
    static float prev_error = 0.0f;
    float error = *a_trg - *a;

    float derivative = error - prev_error;
    prev_error = error;
    float output = kp * error + kd * derivative;
    *a += output;
}

void list_proc(list_page_t *list)
{
    if (list->init_flag == 0) {
        list->init_flag = 1;
        list->oper_flag = 1;
        list->win_start_x = -1.0f * WIDTH;
        list->win_start_x_target = 0.0f;
        list->win_text_y = 0.0f;
        list->win_text_y_target = 0.0f;
    }

    if (list->oper_flag) {
        oled_set_display_mode(INVERSE);
        list->oper_flag = 0;
        list->select_box_w = 0.0f;
        list->select_box_w = 1.0f * strlen(list->list_element[list->select]) * TEXT_WIDTH + TEXT_WIDTH; // 字体是6*8,
    }

    // 动画
    animation_pd(&list->win_start_x, &list->win_start_x_target, 0.05f, 0.01f);
    animation_pd(&list->win_text_y, &list->win_text_y_target, 0.05f, 0.01f);
    animation_pd(&list->select_box_y, &list->select_box_y_target, 0.05f, 0.01f);
    list->win.start_x = (int16_t)list->win_start_x;

    // 绘制列表
    for (int i = 0; i < MAX_LIST_ELEMENT_NUM; i++) {
        if (list->list_element[i] != NULL) {
            oled_win_draw_str(&list->win, 0, LIST_HEIGHT_SPACING * i + LIST_TOP_WIDTH + list->win_text_y, list->list_element[i]);
        }
    }
    // 绘制选中框
    oled_win_draw_box(&list->win, 0, (int16_t)list->select_box_y, (int16_t)list->select_box_w, LIST_HEIGHT_SPACING, 1);

    // 处理按键状态
    if (input_state.roll) {
        input_state.roll = 0; // 清除状态
        list->oper_flag = 1;  // 在此进入oper状态，更新选中框长度

        if (list->select == list->list_element_count - 1) {
            // 表明选中框已经选中到了最后一个，直接返回顶部
            list->select_box_y_target = 0.0f; // 选中框回到顶部
            list->win_text_y_target = 0.0f;   // 文字回到顶部
            list->select = 0;                 // 回到第一个
            return;
        }

        if (list->select_box_y_target == HEIGHT - LIST_HEIGHT_SPACING) {
            // 此时选中框已经到达了当前页面的外面，应该让text的y上移动
            list->win_text_y_target -= LIST_HEIGHT_SPACING;
        } else {
            list->select_box_y_target += LIST_HEIGHT_SPACING;
        }
        list->select++; // 滚动键按下后，对应的选中索引++
    }

    // 处理回调函数

    if (input_state.enter) {
        input_state.enter = 0;
        if (list->cb != NULL) {
            list->cb();
        }
    }
}

void win_proc(list_page_t *list)
{
    if (list->init_flag == 0) {
        list->init_flag = 1;
        list->oper_flag = 1;
        list->win_page.box_w = 0.0f;
        list->win_page.box_w_target = 90.0f;
        list->win_page.box_h_target = 35.0f;
        list->win_page.box_h = 0.0f;
        list->win_page.bar_w = 0.0f;
        list->win_page.bar_w_target = 50.0f;
    }

    animation_pd(&list->win_page.box_w, &list->win_page.box_w_target, 0.05f, 0.01f);
    animation_pd(&list->win_page.box_h, &list->win_page.box_h_target, 0.05f, 0.01f);
    animation_pd(&list->win_page.bar_w, &list->win_page.bar_w_target, 0.05f, 0.01f);

    // 这里需要知道前一个是哪一个，我这里直接用--，线绘制背景
    for (int i = 0; i < MAX_LIST_ELEMENT_NUM; i++) {
        if (list_page[list->index - 1].list_element[i] != NULL) {
            oled_win_draw_str(&list->win, 0, LIST_HEIGHT_SPACING * i + LIST_TOP_WIDTH, list_page[1].list_element[i]);
        }
    }

    // 绘制外框
    oled_set_display_mode(NORMAL);
    oled_win_draw_box(&list->win, 19, 0, (int16_t)list->win_page.box_w + 4, (int16_t)list->win_page.box_h + 2, 0);
    oled_set_display_mode(INVERSE);
    oled_win_draw_box(&list->win, 21, 1, (int16_t)list->win_page.box_w, (int16_t)list->win_page.box_h, 0);

    // 绘制标题
    int title_x = (128 - strlen(list->win_page.title) * 6) / 2;
    oled_win_draw_str(&list->win, 25, 2, list->win_page.title);
    oled_win_draw_str(&list->win, 90, 2, "50");
    oled_win_draw_box(&list->win, 22, 25, (int16_t)list->win_page.bar_w, 5, 1);

    // 处理回调
    if (input_state.enter) {
        input_state.enter = 0;
        jump_page(list->index - 1);
    }

    if (input_state.roll) {
        input_state.roll = 0;
        list->win_page.bar_w_target += 1.0f;
    }
}

void jump_page(int index)
{
    list_page_index = index;
    if (list_page[list_page_index].list_state != WIN) {
        list_page[list_page_index].ui_state = LAYER_IN; // 进入到LAYER IN状态
    }
}

void animation_fade()
{
    static uint8_t fade_count = 0;

    switch (fade_count) {
    case 0:
        for (int i = 0; i < HEIGHT / 8; i++) {
            for (int j = 0; j < WIDTH; j += 2) {
                oled_buffer[i][j] &= 0xAA;
            }
        }
        break;
    case 1:
        for (int i = 0; i < HEIGHT / 8; i++) {
            for (int j = 0; j < WIDTH; j += 2) {
                oled_buffer[i][j] &= 0x00;
            }
        }
        break;
    case 2:
        for (int i = 0; i < HEIGHT / 8; i++) {
            for (int j = 1; j < WIDTH; j += 2) {
                oled_buffer[i][j] &= 0x55;
            }
        }
        break;

    case 3:
        for (int i = 0; i < HEIGHT / 8; i++) {
            for (int j = 1; j < WIDTH; j += 2) {
                oled_buffer[i][j] &= 0x00;
            }
        }
        break;
    default:
        list_page[list_page_index].ui_state = MENU;
        fade_count = 0;
        break;
    }

    fade_count++;
    vTaskDelay(30 / portTICK_PERIOD_MS);
}

void ui_proc()
{
    if (list_page_index < 0) {
        // 当前无界面
        ESP_LOGI(TAG, "Please Add Page");
        return;
    }

    oled_refersh();
    list_page_t *currnt_list_page = &list_page[list_page_index]; // 当前页面
    switch (currnt_list_page->ui_state) {
    case MENU:
        oled_clear();

        switch (currnt_list_page->list_state) {
        case LIST:
            list_proc(currnt_list_page);
            break;
        case WIN:
            win_proc(currnt_list_page);
            break;
        default:
            break;
        }
        break;
    case LAYER_IN:
        animation_fade();
        break;
    case LAYER_OUT:
        break;
    default:
        break;
    }
}
