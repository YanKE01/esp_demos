#include "ui.h"
#include "string.h"
#include "stdio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

page_t page[MAX_PAGE_COUNT]; // 存放所有page
input_state_t input;         // 输入状态
uint8_t cur_page_index = 0;  // 当前页面索引

void ui_page_add_list(uint8_t page_index, list_elemt_t *src, size_t size)
{
    // 设置页面类型
    page[page_index].page_type = LIST;
    // 设置page_id
    page[page_index].page_id = page_index;
    // 设置list元素个数
    page[page_index].list.element_count = size;

    // 拷贝element
    memcpy(page[page_index].list.element, src, sizeof(list_elemt_t) * page[page_index].list.element_count);
    // 设置oled窗口
    page[page_index].list.oled_win.start_x = 0;
    page[page_index].list.oled_win.start_y = 0;
    page[page_index].list.oled_win.w = WIDTH;
    page[page_index].list.oled_win.h = HEIGHT;
    // 设置模式
    page[page_index].page_status = PAGE_NORMAL_STATUS;
}

void input_set_scroll_up(void *button_handle, void *usr_data)
{
    input.scroll_up = true;
}

void input_set_scroll_down(void *button_handle, void *usr_data)
{
    input.scroll_down = true;
}

void input_set_enter(void *button_handle, void *usr_data)
{
    input.enter = true;
}

static void animation_pd(float *a, float *a_trg, float kp, float kd)
{
    static float prev_error = 0.0f;
    float error = *a_trg - *a;

    float derivative = error - prev_error;
    prev_error = error;
    float output = kp * error + kd * derivative;
    *a += output;
}

void list_proc(list_t *list)
{
    if (list->init_flag == 0)
    {
        // 初始化窗口坐标
        list->init_flag = 1;
        list->oper_flag = 1;
        list->start_x = -1.0f * WIDTH;
        list->start_x_target = 0.0f;
        list->start_y = 0.0f;
        list->slide_bar_h = 0.0f;
        list->slide_bar_h_target = 1.0f * LIST_SPACING;
        list->select_box_y = 0.0f;
        list->select_box_y_target = 0.0f;
        list->text_y = 0.0f;
        list->text_y_target = 0.0f;
    }

    if (list->oper_flag)
    {
        oled_set_display_mode(INVERSE);
        list->oper_flag = 0;
        list->select_box_w = 1.0f * strlen(list->element[list->index].element) * TEXT_WIDTH + TEXT_WIDTH;
    }

    // 控制窗口动画
    animation_pd(&list->start_x, &list->start_x_target, 0.05f, 0.01f);
    // animation_pd(&list->select_box_w, &list->select_box_w_target, 0.05f, 0.01f);
    animation_pd(&list->select_box_y, &list->select_box_y_target, 0.05f, 0.01f);
    animation_pd(&list->slide_bar_h, &list->slide_bar_h_target, 0.05f, 0.01f);
    animation_pd(&list->text_y, &list->text_y_target, 0.05f, 0.01f);
    list->oled_win.start_x = (int16_t)list->start_x; // 只让窗口的x坐标变化

    // 绘制列表元素
    for (int i = 0; i < list->element_count; i++)
    {
        oled_win_draw_str(&list->oled_win, 0, LIST_TOP_MARGIN + LIST_SPACING * i + (int16_t)list->text_y, list->element[i].element);
    }

    // 绘制选中框
    oled_win_draw_box(&list->oled_win, 0, (int16_t)list->select_box_y, (int16_t)list->select_box_w, LIST_SPACING, 1);
    // 绘制侧边进度条
    oled_win_draw_box(&list->oled_win, WIDTH - 3, 0, 3, (int16_t)list->slide_bar_h, 0);

    // 处理外部输入
    if (input.scroll_down)
    {
        input.scroll_down = false;
        list->oper_flag = 1;

        if (list->index == list->element_count - 1)
        {
            // 表明已经选中到最底部
            list->select_box_y_target = 0.0f; // 选中框回到顶部
            list->text_y_target = 0.0f;
            list->index = 0;
            list->slide_bar_h_target = 1.0f * LIST_SPACING;
            return;
        }

        if (list->select_box_y_target == HEIGHT - LIST_SPACING)
        {
            // 此时应该让文本动，而不是选中框动，即页面整体上移
            list->text_y_target -= LIST_SPACING;
        }
        else
        {
            list->select_box_y_target += LIST_SPACING;
            list->slide_bar_h_target += LIST_SPACING;
        }

        list->index++; // 选中索引向下移动
    }

    // 处理外部输入
    if (input.enter)
    {
        input.enter = false;

        switch (list->element[list->index].action)
        {
        case NO_ACTION:
            break;
        case BACK_PREV_PAGE:
            page[cur_page_index--].page_status = PAGE_CHANGE;
            break;
        case ENTER_NEXT_PAGE:
            page[cur_page_index++].page_status = PAGE_CHANGE;
            break;
        case POP_WINDOW_SHOW:
            break;
        default:
            break;
        }
    }
}

void animation_fade()
{
    static uint8_t fade_count = 0;

    switch (fade_count)
    {
    case 0:
        for (int i = 0; i < HEIGHT / 8; i++)
        {
            for (int j = 0; j < WIDTH; j += 2)
            {
                oled_buffer[i][j] &= 0xAA;
            }
        }
        break;
    case 1:
        for (int i = 0; i < HEIGHT / 8; i++)
        {
            for (int j = 0; j < WIDTH; j += 2)
            {
                oled_buffer[i][j] &= 0x00;
            }
        }
        break;
    case 2:
        for (int i = 0; i < HEIGHT / 8; i++)
        {
            for (int j = 1; j < WIDTH; j += 2)
            {
                oled_buffer[i][j] &= 0x55;
            }
        }
        break;

    case 3:
        for (int i = 0; i < HEIGHT / 8; i++)
        {
            for (int j = 1; j < WIDTH; j += 2)
            {
                oled_buffer[i][j] &= 0x00;
            }
        }
        break;
    default:
        page[cur_page_index].page_status = PAGE_NORMAL_STATUS;
        fade_count = 0;
        break;
    }

    fade_count++;
    vTaskDelay(30 / portTICK_PERIOD_MS);
}

void ui_proc()
{
    oled_refersh();                           // 刷新显存
    page_t *cur_page = &page[cur_page_index]; // 当前页面
    switch (cur_page->page_status)
    {
    case PAGE_NORMAL_STATUS:
        oled_clear();
        switch (cur_page->page_type)
        {
        case LIST:
            /* code */
            list_proc(&cur_page->list);
            break;
        case POP_WINDOW:
            /* code */
            break;
        default:
            break;
        }
        break;
    case PAGE_CHANGE:
        animation_fade();
        break;
    default:
        break;
    }
}