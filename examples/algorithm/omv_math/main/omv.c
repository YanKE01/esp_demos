#include "stdint.h"
#include "stddef.h"
#include "omv.h"
#include "math.h"

inline float fast_atanf(float xx)
{
    float x, y, z;
    int sign;

    x = xx;

    /* make argument positive and save the sign */
    if (xx < 0.0f) {
        sign = -1;
        x = -xx;
    } else {
        sign = 1;
        x = xx;
    }
    /* range reduction */
    if (x > 2.414213562373095f) {
        /* tan 3pi/8 */
        y = IMLIB_PI_2;
        x = -(1.0f / x);
    } else if (x > 0.4142135623730950f) {
        /* tan pi/8 */
        y = IMLIB_PI_4;
        x = (x - 1.0f) / (x + 1.0f);
    } else {
        y = 0.0f;
    }

    z = x * x;
    y +=
        (((8.05374449538e-2f * z
           - 1.38776856032E-1f) * z
          + 1.99777106478E-1f) * z
         - 3.33329491539E-1f) * z * x + x;

    if (sign < 0) {
        y = -y;
    }

    return (y);
}

float fast_log2(float x)
{
    union {
        float f; uint32_t i;
    } vx = { x };
    union {
        uint32_t i; float f;
    } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
    float y = vx.i;
    y *= 1.1920928955078125e-7f;

    return y - 124.22551499f - 1.498030302f * mx.f
           - 1.72587999f / (0.3520887068f + mx.f);
}

float fast_log(float x)
{
    return 0.69314718f * fast_log2(x);
}

float fast_powf(float a, float b)
{
    union {
        float d; int x;
    } u = { a };
    u.x = (int)((b * (u.x - 1064866805)) + 1064866805);
    return u.d;
}

void fast_get_min_max(float *data, size_t data_len, float *p_min, float *p_max)
{
    float min = FLT_MAX, max = -FLT_MAX;

    for (size_t i = 0; i < data_len; i++) {
        float temp = data[i];

        if (temp < min) {
            min = temp;
        }

        if (temp > max) {
            max = temp;
        }
    }

    *p_min = min;
    *p_max = max;
}

inline float fast_fabsf(float x)
{
    return fabsf(x);
}

inline int fast_roundf(float x)
{
    return roundf(x);
}

inline int fast_ceilf(float x)
{
    return ceilf(x);
}
inline int fast_floorf(float x)
{
    return floorf(x);
}

inline float fast_sqrtf(float x)
{
    return sqrtf(x);
}
