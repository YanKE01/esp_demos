#include "predict.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "mfcc_model_quantized.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "led_strip.h"

extern led_strip_handle_t led_strip;

static const char *TAG = "wakeup";
constexpr int kTensorArenaSize = 20480;
uint8_t *tensor_arena = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

void wakeup_model_init()
{
    tensor_arena = (uint8_t *)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM);
    if (!tensor_arena)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for tensor arena in SPI RAM");
        return;
    }

    const tflite::Model *model = tflite::GetModel(mfcc_model_quantized_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        MicroPrintf("Model provided is schema version %d not equal to supported "
                    "version %d.",
                    model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::MicroMutableOpResolver<9> resolver;
    // 这里需要添加我们模型使用的层
    if (resolver.AddReshape() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add reshape failed");
        return;
    }

    if (resolver.AddConv2D() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add conv2d failed");
        return;
    }

    if (resolver.AddMaxPool2D() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add maxp2d failed");
        return;
    }

    if (resolver.AddPack() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add pack failed");
        return;
    }

    if (resolver.AddExpandDims() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add expanddims failed");
        return;
    }

    if (resolver.AddFullyConnected() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add fulltconnected failed");
        return;
    }

    if (resolver.AddSoftmax() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add softmax failed");
        return;
    }

    if (resolver.AddShape() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add shape failed");
        return;
    }

    if (resolver.AddStridedSlice() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add stride failed");
        return;
    }

    // Create interpreter
    static tflite::MicroInterpreter static_interpreter(model, resolver, (uint8_t *)tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Allocate memory for model tensor
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        MicroPrintf("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
}

void wakeup_model_predict(float *mfcc, int size)
{
    for (int i = 0; i < size; i++)
    {
        input->data.f[i] = mfcc[i];
    }
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk)
    {
        MicroPrintf("Invoke failed on");
        return;
    }

    for (int i = 0; i < 2; i++)
    {
        printf("%.2f ", output->data.f[i]);

        if (output->data.f[1] > 0.9f)
        {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, 5, 5, 5));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        }
        else
        {
            led_strip_clear(led_strip);
        }
    }
    printf("\n");
}