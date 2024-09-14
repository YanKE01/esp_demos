#pragma once

#include "ssd1306.h"

#define MAX_LIST_NUM 100
#define MAX_LIST_ELEMENT_NUM 20
#define LIST_TOP_WIDTH 4       // 列表到顶端的距离
#define LIST_HEIGHT_SPACING 16 // 列标文字之间的高度间距
#define TEXT_WIDTH 6
#define TEST_HEIGHT 8

typedef enum {
    MENU = 0,
    LAYER_IN,
    LAYER_OUT,
} ui_state_t;

typedef enum {
    LIST = 0,
    WIN,
} list_state_t;

typedef struct {
    bool roll;  // 滚动
    bool enter; // 进入
} input_state_t;

typedef struct {
    char *title;
    uint8_t value;
    uint8_t max;
    uint8_t min;
    float box_y;
    float box_y_target;
    float box_w;
    float box_w_target;
    float box_h;
    float box_h_target;
    float bar_w_target;
    float bar_w;
} win_page_t;

typedef struct {
    ui_state_t ui_state;                      // 判断当前ui状态
    list_state_t list_state;                  // 判断当前list状态
    char *list_element[MAX_LIST_ELEMENT_NUM]; // 当前list元素
    uint8_t index;                            // 当前索引
    window_t win;                             // 创建
    uint8_t init_flag;                        // 是否初始化
    uint8_t oper_flag;                        // 操作标志位
    float win_start_x_target;                 // 当前list下的目标x
    float win_start_x;                        // 当前list下的x
    float win_text_y_target;                  // 当前list下的目标y
    float win_text_y;                         // 当前list下的y
    uint8_t select;                           // 当前选中的为第几个，默认为0
    float select_box_w;                       // 选中框长度
    float select_box_y;                       // 选中框的y坐标
    float select_box_y_target;                // 选中框的目标y坐标
    float select_box_w_target;                // 选中框的目标长度
    uint8_t list_element_count;               // 当前list元素的个数
    void (*cb)();                             // 回调函数
    win_page_t win_page;                      // 弹窗
} list_page_t;

extern int list_page_index;
extern list_page_t list_page[MAX_LIST_NUM];

void ui_add_page(list_page_t list);
void ui_proc();
void input_enter_cb();
void input_roll_cb();
void jump_page(int index);
