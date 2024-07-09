#include "predict.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "mfcc_model_quantized.h"
#include "esp_log.h"

static const char *TAG = "wakeup";
constexpr int kTensorArenaSize = 282144;
const unsigned char tensor_arena[kTensorArenaSize] = {0};
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

void wakeup_model_init()
{
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