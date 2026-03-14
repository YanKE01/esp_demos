#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_check.h"
#include "esp_dsp.h"
#include "esp_log.h"

#define FFT_SIZE 256
#define SAMPLE_RATE_HZ 1024

static const char *TAG = "FFT_DEMO";

static float fft_input[FFT_SIZE * 2];
static float fft_magnitude[FFT_SIZE / 2];
static float fft_window[FFT_SIZE];
static int top_bins[8];

static void generate_signal(void)
{
    // Generate a test waveform containing three tones:
    // 50 Hz, 120 Hz, and 260 Hz.
    for (int i = 0; i < FFT_SIZE; i++) {
        float t = (float)i / SAMPLE_RATE_HZ;
        float sample = 0.9f * sinf(2.0f * (float)M_PI * 50.0f * t)
                       + 0.5f * sinf(2.0f * (float)M_PI * 120.0f * t)
                       + 0.3f * sinf(2.0f * (float)M_PI * 260.0f * t);

        // esp-dsp complex FFT input format:
        // Re[0], Im[0], Re[1], Im[1], ...
        fft_input[i * 2] = sample * fft_window[i];  // Real part
        fft_input[i * 2 + 1] = 0.0f;  // For time-domain data, the imaginary part is generally 0.
    }
}

static void calculate_magnitude(void)
{
    for (int bin = 0; bin < FFT_SIZE / 2; bin++) {
        float real = fft_input[bin * 2];
        float imag = fft_input[bin * 2 + 1];
        fft_magnitude[bin] = sqrtf(real * real + imag * imag);
    }
}

static void print_top_bins(void)
{
    for (int rank = 0; rank < 8; rank++) {
        int best_bin = -1;
        float best_mag = -1.0f;

        for (int bin = 1; bin < FFT_SIZE / 2; bin++) {
            if (fft_magnitude[bin] > best_mag) {
                bool used = false;
                for (int prev = 0; prev < rank; prev++) {
                    if (top_bins[prev] == bin) {
                        used = true;
                        break;
                    }
                }
                if (!used) {
                    best_mag = fft_magnitude[bin];
                    best_bin = bin;
                }
            }
        }

        top_bins[rank] = best_bin;
        if (best_bin >= 0) {
            float hz = ((float)best_bin * SAMPLE_RATE_HZ) / FFT_SIZE;
            ESP_LOGI(TAG, "Top %d: bin=%d, freq=%.1f Hz, mag=%.3f",
                     rank + 1, best_bin, hz, best_mag);
        }
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE));
    dsps_wind_hann_f32(fft_window, FFT_SIZE);

    generate_signal();

    // Step 1: run FFT
    ESP_ERROR_CHECK(dsps_fft2r_fc32(fft_input, FFT_SIZE));

    // Step 2: reorder FFT output into normal frequency-bin order
    ESP_ERROR_CHECK(dsps_bit_rev_fc32(fft_input, FFT_SIZE));

    // Step 3: convert complex FFT result to amplitude per frequency bin
    calculate_magnitude();

    ESP_LOGI(TAG, "FFT size = %d, sample_rate = %d Hz, bin_resolution = %.2f Hz",
             FFT_SIZE, SAMPLE_RATE_HZ, (float)SAMPLE_RATE_HZ / FFT_SIZE);
    print_top_bins();

    ESP_LOGI(TAG, "Expected main peaks near 50 Hz, 120 Hz, 260 Hz");

    dsps_fft2r_deinit_fc32();
}
