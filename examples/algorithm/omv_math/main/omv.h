#pragma once

#define IMLIB_PI                 3.14159265358979323846f
#define IMLIB_PI_2               1.57079632679489661923f
#define IMLIB_PI_4               0.78539816339744830962f
#define FLT_MAX     __FLT_MAX__

float fast_atanf(float xx);
float fast_log(float x);
float fast_powf(float a, float b);
float fast_fabsf(float x);
int fast_roundf(float x);
int fast_ceilf(float x);
int fast_floorf(float x);
float fast_sqrtf(float x);
