#include <stdio.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "c_speech_features.h"
#include "signal.h"

void app_main(void)
{
    int win_length = (int)(0.02 * 16000);
    float window[win_length];

    for (int i = 0; i < win_length; i++)
    {
        window[i] = 1.0f;
    }

    float *result = NULL;
    int num_cep = 13;
    int64_t start_time = esp_timer_get_time();
    int n_frames = csf_mfcc((const short *)signal, signal_length, 16 * 1000, 0.02, 0.02, 13, 32, 512, 0, 16000 / 2, 0.98,
                            32, 1, window, &result);
    int64_t end_time = esp_timer_get_time();

    printf("MFCC calculation successful. Number of frames: %d\n", n_frames);
    printf("MFCC results:\n");
    for (int i = 0; i < n_frames; ++i)
    {
        printf("Frame %d: ", i);
        for (int j = 0; j < num_cep; ++j)
        {
            printf("%f ", result[i * num_cep + j]);
        }
        printf("\n");
    }

    int64_t elapsed_time = end_time - start_time;

    printf("program runtime:%lld us\n", elapsed_time);
}
