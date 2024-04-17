#include <stdio.h>
#include "string.h"
#include <json_parser.h>

char json_str[] = "{\"output\":{\"choices\":[{\"finish_reason\":\"stop\",\"message\":{\"role\":\"assistant\",\"content\":\"对不起，作为一个AI模型，我无法获取实时的天气信息。你可以查看各大天气预报网站，或者使用专业的天气应用来查询。\"}}]},\"usage\":{\"total_tokens\":51,\"output_tokens\":29,\"input_tokens\":22},\"request_id\":\"7476133e-77da-9bce-89a0-0db7f4e7607a\"}";
char result[256];

void app_main(void)
{
    jparse_ctx_t jctx;
    int num_elem = 0;
    int ret = json_parse_start(&jctx, json_str, strlen(json_str));
    if (ret != OS_SUCCESS)
    {
        printf("Parser failed\n");
        return;
    }

    if (json_obj_get_object(&jctx, "output") == OS_SUCCESS)
    {
        if (json_obj_get_array(&jctx, "choices", &num_elem) == OS_SUCCESS)
        {
            for (int i = 0; i < num_elem; i++)
            {
                if (json_arr_get_object(&jctx, i) == OS_SUCCESS)
                {
                    if (json_obj_get_object(&jctx, "message") == OS_SUCCESS)
                    {
                        if (json_obj_get_string(&jctx, "content", result, sizeof(result)) == OS_SUCCESS)
                        {
                            printf("%s\n", result);
                        }
                    }
                }
            }
        }
    }
}
