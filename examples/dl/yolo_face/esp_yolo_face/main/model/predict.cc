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
uint8_t anchors[3][2] = {{9, 14}, {12, 17}, {22, 21}};
int grid_x, grid_y;
float x, y, w, h;
int x_1, y_1, x_2, y_2;

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

float sigmoid(float x)
{
    float y = 1 / (1 + expf(-x));
    return y;
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

    for (int i = 0; i < 49; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            // 网络输出维度是1*7*7*18
            // 其中18维度包含了每个像素预测的三个锚框，每个锚框对应6个维度，依次为x y w h conf class
            // 当然因为这个网络是单类检测，所以class这一维度没有用
            // 如果对YOLO不熟悉的话，建议去学习一下yolov3
            int8_t conf = output->data.int8[i * 18 + j * 6 + 4];
            // 这里的-9是根据网络量化的缩放偏移量计算的，对应的是70%的置信度
            // sigmoid((conf+15)*0.14218327403068542) < 0.7 ==> conf > -9
            if (conf > -9)
            {
                grid_x = i % 7;
                grid_y = (i - grid_x) / 7;
                // 这里的15和0.14218327403068542就是网络量化后给出的缩放偏移量
                x = ((float)output->data.int8[i * 18 + j * 6] + 15) * 0.14218327403068542f;
                y = ((float)output->data.int8[i * 18 + j * 6 + 1] + 15) * 0.14218327403068542f;
                w = ((float)output->data.int8[i * 18 + j * 6 + 2] + 15) * 0.14218327403068542f;
                h = ((float)output->data.int8[i * 18 + j * 6 + 3] + 15) * 0.14218327403068542f;
                // 网络下采样三次，缩小了8倍，这里给还原回56*56的尺度
                x = (sigmoid(x) + grid_x) * 8;
                y = (sigmoid(y) + grid_y) * 8;
                w = expf(w) * anchors[j][0];
                h = expf(h) * anchors[j][1];
                x_1 = int(x - w / 2);
                x_2 = int(x + w / 2);
                y_1 = int(y - h / 2);
                y_2 = int(y + h / 2);
                if (x_1 < 0)
                    x_1 = 0;
                if (y_1 < 0)
                    y_1 = 0;
                if (x_2 > 55)
                    x_2 = 55;
                if (y_2 > 55)
                    y_2 = 55;
            }
        }
    }

    printf("%d %d %d %d", x_1, y_1, x_2, y_2);

    printf("\n");
}