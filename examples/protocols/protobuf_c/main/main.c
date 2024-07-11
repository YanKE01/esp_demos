#include <stdio.h>
#include "simple.pb-c.h"

void app_main(void)
{
    // 创建一个 SimpleMessage 实例
    SimpleMessage msg = SIMPLE_MESSAGE__INIT;

    // 设置 lucky_number 字段
    msg.lucky_number = 42;

    // 序列化消息
    uint8_t *buffer;
    unsigned len;
    len = simple_message__get_packed_size(&msg);
    buffer = malloc(len);
    simple_message__pack(&msg, buffer);

    // 这里你可以将序列化的 buffer 发送到其他系统或存储起来
    // 例如，打印 buffer 的内容
    for (unsigned i = 0; i < len; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");

    // 反序列化消息
    SimpleMessage *msg_out;
    msg_out = simple_message__unpack(NULL, len, buffer);
    if (msg_out == NULL)
    {
        fprintf(stderr, "Error unpacking incoming message\n");
        free(buffer);
    }

    // 使用反序列化后的消息
    printf("Lucky number: %ld\n", msg_out->lucky_number);

    // 清理
    simple_message__free_unpacked(msg_out, NULL);
    free(buffer);
}
