#pragma once

#include "stdint.h"
#include "stddef.h"
#include "ssd1306.h"
#include "iot_button.h"

#define MAX_LIST_ELEMENT 10 // 最大列表元素个数
#define MAX_PAGE_COUNT 10   // 最大页面个数
#define LIST_SPACING 16     // 列表元素高度间隔
#define LIST_TOP_MARGIN 4   // 列表元素顶部间隔
#define TEXT_WIDTH 6        // 文本宽度

typedef enum
{
    LIST = 0,
    POP_WINDOW,
} page_type_t;

typedef enum
{
    PAGE_NORMAL_STATUS = 0, // 正常显示
    PAGE_CHANGE,            // 切换页面
} page_status_t;

typedef enum
{
    NO_ACTION = 0,
    BACK_PREV_PAGE,
    ENTER_NEXT_PAGE,
    POP_WINDOW_SHOW,
} list_elemt_action_t;

/**
 * @brief list类型的元素
 *
 */
typedef struct
{
    char *element;
    list_elemt_action_t action;
} list_elemt_t;

/**
 * @brief list页面所需的参数
 *
 */
typedef struct
{
    list_elemt_t element[MAX_LIST_ELEMENT]; // 列表元素
    size_t element_count;                   // 有效列表的个数
    size_t index;                           // 选中的列表索引
    uint8_t init_flag;                      // 初始化标志位
    uint8_t oper_flag;                      // 操作标志位
    window_t oled_win;                      // oled窗口
    float start_x;                          // 窗口开始坐标x
    float start_x_target;                   // 窗口目标坐标x
    float start_y;                          // 窗口开始坐标y
    float start_y_target;                   // 窗口目标坐标y
    float select_box_w;                     // 选中框长度
    float select_box_y;                     // 选中框的y坐标
    float select_box_y_target;              // 选中框的目标y坐标
    float select_box_w_target;              // 选中框的目标长度
    float slide_bar_h;                      // 侧边滑动条高度
    float slide_bar_h_target;               // 侧边滑动条目标高度
    float text_y;                           // 显示文本y
    float text_y_target;                    // 显示文本目标y
} list_t;

typedef struct
{

} pop_window_t;

typedef struct
{
    uint8_t page_id;           // 当前页面ID
    page_type_t page_type;     // 页面类型
    list_t list;               // list类型
    pop_window_t pop_window;   // 弹窗类型
    page_status_t page_status; // 页面状态
} page_t;

typedef struct
{
    bool scroll_up;
    bool scroll_down;
    bool enter;
} input_state_t;

extern input_state_t input; // 输入状态

/**
 * @brief add list
 *
 * @param page_index
 * @param src
 * @param size
 */
void ui_page_add_list(uint8_t page_index, list_elemt_t *src, size_t size);

/**
 * @brief main ui proc
 *
 */
void ui_proc();

/**
 * @brief set scroll up callback
 *
 * @param button_handle
 * @param usr_data
 */
void input_set_scroll_up(void *button_handle, void *usr_data);

/**
 * @brief set scroll down callback
 *
 * @param button_handle
 * @param usr_data
 */
void input_set_scroll_down(void *button_handle, void *usr_data);

/**
 * @brief set enter callback
 *
 * @param button_handle
 * @param usr_data
 */
void input_set_enter(void *button_handle, void *usr_data);
