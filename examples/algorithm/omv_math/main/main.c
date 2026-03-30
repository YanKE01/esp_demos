#include <stdint.h>
#include <stdio.h>

#include "IQmathLib.h"
#include "esp_timer.h"
#include "omv.h"

#define SAMPLE_COUNT 1000
#define BENCH_ROUNDS 1000

static volatile float s_fast_sink;
static volatile int64_t s_iq_sink_raw;
static volatile int64_t s_int_sink;

static void fill_signed_samples(float samples[SAMPLE_COUNT])
{
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        float normalized = ((float) i / (float) (SAMPLE_COUNT - 1)) * 2.0f - 1.0f;
        samples[i] = normalized * 4.0f;
    }
}

static void fill_positive_samples(float samples[SAMPLE_COUNT])
{
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        float normalized = (float) i / (float) (SAMPLE_COUNT - 1);
        samples[i] = 0.125f + normalized * (4.0f - 0.125f);
    }
}

static void fill_pow_samples(float bases[SAMPLE_COUNT], float exponents[SAMPLE_COUNT])
{
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        float normalized = (float) i / (float) (SAMPLE_COUNT - 1);

        bases[i] = 0.125f + normalized * (4.0f - 0.125f);
        exponents[i] = -1.0f + normalized * 2.0f;
    }
}

static float iq_abs_with_convert(float x)
{
    return _IQtoF(_IQabs(_IQ(x)));
}

static int iq_floor_with_convert(float x)
{
    return _IQint(_IQ(x));
}

static int iq_ceil_with_convert(float x)
{
    _iq q = _IQ(x);
    int floor_i = _IQint(q);

    if (_IQtoF(q) > (float) floor_i) {
        return floor_i + 1;
    }

    return floor_i;
}

static int iq_round_with_convert(float x)
{
    _iq q = _IQ(x);
    _iq half = _IQ(0.5f);

    if (q >= 0) {
        return _IQint(q + half);
    }

    return _IQint(q - half);
}

static float iq_sqrt_with_convert(float x)
{
    return _IQtoF(_IQsqrt(_IQ(x)));
}

static float iq_pow_with_convert(float base, float exponent)
{
    return _IQtoF(_IQexp(_IQmpy(_IQlog(_IQ(base)), _IQ(exponent))));
}

static int64_t bench_fast_atanf(const float samples[SAMPLE_COUNT], float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_atanf(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = fast_atanf(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_atan_with_convert(const float samples[SAMPLE_COUNT], float *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += (int64_t) _IQatan(_IQ(samples[i]));
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_iq_sink_raw = sum;
    *last_result = _IQtoF(_IQatan(_IQ(samples[SAMPLE_COUNT / 2])));

    return elapsed_us;
}

static int64_t bench_fast_log(const float samples[SAMPLE_COUNT], float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_log(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = fast_log(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_log_with_convert(const float samples[SAMPLE_COUNT], float *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += (int64_t) _IQlog(_IQ(samples[i]));
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_iq_sink_raw = sum;
    *last_result = _IQtoF(_IQlog(_IQ(samples[SAMPLE_COUNT / 2])));

    return elapsed_us;
}

static int64_t bench_fast_powf(const float bases[SAMPLE_COUNT],
                               const float exponents[SAMPLE_COUNT],
                               float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_powf(bases[i], exponents[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = fast_powf(bases[SAMPLE_COUNT / 2], exponents[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_pow_with_convert(const float bases[SAMPLE_COUNT],
        const float exponents[SAMPLE_COUNT],
        float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += iq_pow_with_convert(bases[i], exponents[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = iq_pow_with_convert(bases[SAMPLE_COUNT / 2], exponents[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_fast_fabsf(const float samples[SAMPLE_COUNT], float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_fabsf(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = fast_fabsf(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_abs_with_convert(const float samples[SAMPLE_COUNT], float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += iq_abs_with_convert(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = iq_abs_with_convert(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_fast_roundf(const float samples[SAMPLE_COUNT], int *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_roundf(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_int_sink = sum;
    *last_result = fast_roundf(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_round_with_convert(const float samples[SAMPLE_COUNT], int *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += iq_round_with_convert(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_int_sink = sum;
    *last_result = iq_round_with_convert(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_fast_ceilf(const float samples[SAMPLE_COUNT], int *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_ceilf(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_int_sink = sum;
    *last_result = fast_ceilf(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_ceil_with_convert(const float samples[SAMPLE_COUNT], int *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += iq_ceil_with_convert(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_int_sink = sum;
    *last_result = iq_ceil_with_convert(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_fast_floorf(const float samples[SAMPLE_COUNT], int *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_floorf(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_int_sink = sum;
    *last_result = fast_floorf(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_floor_with_convert(const float samples[SAMPLE_COUNT], int *last_result)
{
    int64_t sum = 0;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += iq_floor_with_convert(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_int_sink = sum;
    *last_result = iq_floor_with_convert(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_fast_sqrtf(const float samples[SAMPLE_COUNT], float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += fast_sqrtf(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = fast_sqrtf(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static int64_t bench_iq_sqrt_with_convert(const float samples[SAMPLE_COUNT], float *last_result)
{
    float sum = 0.0f;
    int64_t start = esp_timer_get_time();

    for (int round = 0; round < BENCH_ROUNDS; ++round) {
        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            sum += iq_sqrt_with_convert(samples[i]);
        }
    }

    int64_t elapsed_us = esp_timer_get_time() - start;

    s_fast_sink = sum;
    *last_result = iq_sqrt_with_convert(samples[SAMPLE_COUNT / 2]);

    return elapsed_us;
}

static void print_stats(const char *name, int64_t total_us)
{
    double avg_round_us = (double) total_us / (double) BENCH_ROUNDS;
    double avg_call_ns = ((double) total_us * 1000.0) /
                         ((double) BENCH_ROUNDS * (double) SAMPLE_COUNT);

    printf("%s\n", name);
    printf("  total: %lld us\n", (long long) total_us);
    printf("  avg per round: %.3f us\n", avg_round_us);
    printf("  avg per call: %.3f ns\n", avg_call_ns);
}

static void print_compare_header(const char *title)
{
    printf("\n%s\n", title);
    printf("samples: %d, rounds: %d, total calls per benchmark: %d\n",
           SAMPLE_COUNT, BENCH_ROUNDS, SAMPLE_COUNT * BENCH_ROUNDS);
}

void app_main(void)
{
    float signed_samples[SAMPLE_COUNT];
    float positive_samples[SAMPLE_COUNT];
    float pow_bases[SAMPLE_COUNT];
    float pow_exponents[SAMPLE_COUNT];
    float fast_float_result = 0.0f;
    float iq_float_result = 0.0f;
    int fast_int_result = 0;
    int iq_int_result = 0;

    fill_signed_samples(signed_samples);
    fill_positive_samples(positive_samples);
    fill_pow_samples(pow_bases, pow_exponents);

    int64_t fast_atan_us = bench_fast_atanf(signed_samples, &fast_float_result);
    int64_t iq_atan_us = bench_iq_atan_with_convert(signed_samples, &iq_float_result);

    print_compare_header("OMV fast_atanf vs IQmath convert + _IQatan");
    printf("check sample x = %.6f\n", signed_samples[SAMPLE_COUNT / 2]);
    printf("  fast_atanf result: %.6f rad\n", fast_float_result);
    printf("  _IQatan with convert result: %.6f rad\n", iq_float_result);
    print_stats("fast_atanf", fast_atan_us);
    print_stats("float->_IQ + _IQatan", iq_atan_us);
    printf("speed ratio (convert + _IQatan / fast_atanf): %.3fx\n",
           (double) iq_atan_us / (double) fast_atan_us);

    int64_t fast_log_us = bench_fast_log(positive_samples, &fast_float_result);
    int64_t iq_log_us = bench_iq_log_with_convert(positive_samples, &iq_float_result);

    print_compare_header("OMV fast_log vs IQmath convert + _IQlog");
    printf("check sample x = %.6f\n", positive_samples[SAMPLE_COUNT / 2]);
    printf("  fast_log result: %.6f\n", fast_float_result);
    printf("  _IQlog with convert result: %.6f\n", iq_float_result);
    print_stats("fast_log", fast_log_us);
    print_stats("float->_IQ + _IQlog", iq_log_us);
    printf("speed ratio (convert + _IQlog / fast_log): %.3fx\n",
           (double) iq_log_us / (double) fast_log_us);

    int64_t fast_pow_us = bench_fast_powf(pow_bases, pow_exponents, &fast_float_result);
    int64_t iq_pow_us = bench_iq_pow_with_convert(pow_bases, pow_exponents, &iq_float_result);

    print_compare_header("OMV fast_powf vs IQmath convert + _IQlog/_IQmpy/_IQexp");
    printf("check sample base = %.6f, exp = %.6f\n",
           pow_bases[SAMPLE_COUNT / 2], pow_exponents[SAMPLE_COUNT / 2]);
    printf("  fast_powf result: %.6f\n", fast_float_result);
    printf("  IQmath pow pipeline result: %.6f\n", iq_float_result);
    print_stats("fast_powf", fast_pow_us);
    print_stats("float->_IQ + _IQlog/_IQmpy/_IQexp", iq_pow_us);
    printf("speed ratio (convert + IQmath pow pipeline / fast_powf): %.3fx\n",
           (double) iq_pow_us / (double) fast_pow_us);

    int64_t fast_abs_us = bench_fast_fabsf(signed_samples, &fast_float_result);
    int64_t iq_abs_us = bench_iq_abs_with_convert(signed_samples, &iq_float_result);

    print_compare_header("OMV fast_fabsf vs IQmath convert + _IQabs");
    printf("check sample x = %.6f\n", signed_samples[SAMPLE_COUNT / 2]);
    printf("  fast_fabsf result: %.6f\n", fast_float_result);
    printf("  _IQabs with convert result: %.6f\n", iq_float_result);
    print_stats("fast_fabsf", fast_abs_us);
    print_stats("float->_IQ + _IQabs + _IQtoF", iq_abs_us);
    printf("speed ratio (convert + _IQabs / fast_fabsf): %.3fx\n",
           (double) iq_abs_us / (double) fast_abs_us);

    int64_t fast_round_us = bench_fast_roundf(signed_samples, &fast_int_result);
    int64_t iq_round_us = bench_iq_round_with_convert(signed_samples, &iq_int_result);

    print_compare_header("OMV fast_roundf vs IQmath style convert + round");
    printf("check sample x = %.6f\n", signed_samples[SAMPLE_COUNT / 2]);
    printf("  fast_roundf result: %d\n", fast_int_result);
    printf("  IQ-style round result: %d\n", iq_int_result);
    print_stats("fast_roundf", fast_round_us);
    print_stats("float->_IQ + round helper", iq_round_us);
    printf("speed ratio (convert + round helper / fast_roundf): %.3fx\n",
           (double) iq_round_us / (double) fast_round_us);

    int64_t fast_ceil_us = bench_fast_ceilf(signed_samples, &fast_int_result);
    int64_t iq_ceil_us = bench_iq_ceil_with_convert(signed_samples, &iq_int_result);

    print_compare_header("OMV fast_ceilf vs IQmath style convert + ceil");
    printf("check sample x = %.6f\n", signed_samples[SAMPLE_COUNT / 2]);
    printf("  fast_ceilf result: %d\n", fast_int_result);
    printf("  IQ-style ceil result: %d\n", iq_int_result);
    print_stats("fast_ceilf", fast_ceil_us);
    print_stats("float->_IQ + ceil helper", iq_ceil_us);
    printf("speed ratio (convert + ceil helper / fast_ceilf): %.3fx\n",
           (double) iq_ceil_us / (double) fast_ceil_us);

    int64_t fast_floor_us = bench_fast_floorf(signed_samples, &fast_int_result);
    int64_t iq_floor_us = bench_iq_floor_with_convert(signed_samples, &iq_int_result);

    print_compare_header("OMV fast_floorf vs IQmath style convert + floor");
    printf("check sample x = %.6f\n", signed_samples[SAMPLE_COUNT / 2]);
    printf("  fast_floorf result: %d\n", fast_int_result);
    printf("  IQ-style floor result: %d\n", iq_int_result);
    print_stats("fast_floorf", fast_floor_us);
    print_stats("float->_IQ + floor helper", iq_floor_us);
    printf("speed ratio (convert + floor helper / fast_floorf): %.3fx\n",
           (double) iq_floor_us / (double) fast_floor_us);

    int64_t fast_sqrt_us = bench_fast_sqrtf(positive_samples, &fast_float_result);
    int64_t iq_sqrt_us = bench_iq_sqrt_with_convert(positive_samples, &iq_float_result);

    print_compare_header("OMV fast_sqrtf vs IQmath convert + _IQsqrt");
    printf("check sample x = %.6f\n", positive_samples[SAMPLE_COUNT / 2]);
    printf("  fast_sqrtf result: %.6f\n", fast_float_result);
    printf("  _IQsqrt with convert result: %.6f\n", iq_float_result);
    print_stats("fast_sqrtf", fast_sqrt_us);
    print_stats("float->_IQ + _IQsqrt + _IQtoF", iq_sqrt_us);
    printf("speed ratio (convert + _IQsqrt / fast_sqrtf): %.3fx\n",
           (double) iq_sqrt_us / (double) fast_sqrt_us);

}
