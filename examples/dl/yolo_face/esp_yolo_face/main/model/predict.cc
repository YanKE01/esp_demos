#include "predict.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "yoloface_int8.h"
#include "esp_log.h"

static const char *TAG = "YOLO";
constexpr int kTensorArenaSize = 262144;
uint8_t tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

void model_yoloface_init()
{
    const tflite::Model *model = tflite::GetModel(yoloface_int8_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        MicroPrintf("Model provided is schema version %d not equal to supported "
                    "version %d.",
                    model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::MicroMutableOpResolver<11> resolver;

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
        ESP_LOGI(TAG, "Add maxpool2d failed");
        return;
    }

    if (resolver.AddFullyConnected() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add connect failed");
        return;
    }

    if (resolver.AddSoftmax() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add softmax failed");
        return;
    }

    if (resolver.AddPad() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add pad failed");
        return;
    }

    if (resolver.AddLeakyRelu() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add leaky relu failed");
        return;
    }

    if (resolver.AddDepthwiseConv2D() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add depthwise 2d failed");
        return;
    }

    if (resolver.AddAdd() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add depthwise 2d failed");
        return;
    }

    if (resolver.AddQuantize() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add quantize failed");
        return;
    }

    if (resolver.AddConcatenation() != kTfLiteOk)
    {
        ESP_LOGI(TAG, "Add concatenation failed");
        return;
    }

    // Create interpreter
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
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

void model_yoloface_predict(uint8_t *pic, size_t size)
{
    for (int i = 0; i < size; i++)
    {
        input->data.int8[i] = (int8_t)(pic[i] - 128);
    }

    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk)
    {
        MicroPrintf("Invoke failed on");
        return;
    }

    printf("\n");
}