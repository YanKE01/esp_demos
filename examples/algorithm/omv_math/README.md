# omv_math Benchmark Notes

## Environment

| Item | Value |
| --- | --- |
| Target | `esp32p4` |
| ESP-IDF | `v5.5.3` |
| Build mode | `O3` |
| Benchmark source | `main/main.c` |
| Fast math source | `main/omv.c` |
| IQMath mode | default `_IQ` |

说明:

- 当前工程没有重定义 `GLOBAL_IQ`，因此默认 `_IQ` 使用 IQMath 头文件里的默认全局格式。
- benchmark 代码见 `main/main.c`。

## Benchmark Method

| Item | Value |
| --- | --- |
| Samples | `1000` |
| Rounds | `1000` |
| Total calls per benchmark | `1,000,000` |
| Input style | float-domain |

IQMath 统计的是端到端路径，不是单独 kernel:

| Function | IQMath measured path |
| --- | --- |
| `atan` | `float -> _IQ -> _IQatan -> _IQtoF` |
| `log` | `float -> _IQ -> _IQlog -> _IQtoF` |
| `pow` | `float -> _IQ -> _IQlog -> _IQmpy -> _IQexp -> _IQtoF` |
| `fabsf` | `float -> _IQ -> _IQabs -> _IQtoF` |
| `roundf` | `float -> _IQ -> round helper` |
| `ceilf` | `float -> _IQ -> ceil helper` |
| `floorf` | `float -> _IQ -> floor helper` |
| `sqrtf` | `float -> _IQ -> _IQsqrt -> _IQtoF` |

## Speed Summary

| Function | Fast Path Avg/Call | IQMath Path Avg/Call | Speed Ratio | Winner |
| --- | ---: | ---: | ---: | --- |
| `atan` | `208.540 ns` | `426.774 ns` | `2.046x` | `fast_atanf` |
| `log` | `110.433 ns` | `479.294 ns` | `4.340x` | `fast_log` |
| `pow` | `67.771 ns` | `937.035 ns` | `13.826x` | `fast_powf` |
| `fabsf` | `22.598 ns` | `150.543 ns` | `6.662x` | `fast_fabsf` |
| `roundf` | `104.284 ns` | `56.482 ns` | `0.542x` | IQ-style helper |
| `ceilf` | `139.759 ns` | `183.451 ns` | `1.313x` | `fast_ceilf` |
| `floorf` | `138.599 ns` | `45.180 ns` | `0.326x` | IQ-style helper |
| `sqrtf` | `102.934 ns` | `452.776 ns` | `4.399x` | `fast_sqrtf` |

说明:

- `speed ratio` 定义为 `IQMath path / fast path`。
- 小于 `1.0x` 表示 IQ-style 写法更快。

## Detailed Timing

| Function | Fast Total | Fast Avg/Round | Fast Avg/Call | IQ Total | IQ Avg/Round | IQ Avg/Call |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `atan` | `208540 us` | `208.540 us` | `208.540 ns` | `426774 us` | `426.774 us` | `426.774 ns` |
| `log` | `110433 us` | `110.433 us` | `110.433 ns` | `479294 us` | `479.294 us` | `479.294 ns` |
| `pow` | `67771 us` | `67.771 us` | `67.771 ns` | `937035 us` | `937.035 us` | `937.035 ns` |
| `fabsf` | `22598 us` | `22.598 us` | `22.598 ns` | `150543 us` | `150.543 us` | `150.543 ns` |
| `roundf` | `104284 us` | `104.284 us` | `104.284 ns` | `56482 us` | `56.482 us` | `56.482 ns` |
| `ceilf` | `139759 us` | `139.759 us` | `139.759 ns` | `183451 us` | `183.451 us` | `183.451 ns` |
| `floorf` | `138599 us` | `138.599 us` | `138.599 ns` | `45180 us` | `45.180 us` | `45.180 ns` |
| `sqrtf` | `102934 us` | `102.934 us` | `102.934 ns` | `452776 us` | `452.776 us` | `452.776 ns` |

## Sample Result Check

| Function | Sample Input | Fast Result | IQMath Result | Observation |
| --- | --- | ---: | ---: | --- |
| `atan` | `x = 0.004004` | `0.004004` | `0.004004` | sample point is effectively identical |
| `log` | `x = 2.064440` | `0.724807` | `0.724859` | close on this sample |
| `pow` | `base = 2.064440, exp = 0.001001` | `0.971554` | `1.000726` | large gap on this sample |
| `fabsf` | `x = 0.004004` | `0.004004` | `0.004004` | identical on this sample |
| `roundf` | `x = 0.004004` | `0` | `0` | identical on this sample |
| `ceilf` | `x = 0.004004` | `1` | `1` | identical on this sample |
| `floorf` | `x = 0.004004` | `0` | `0` | identical on this sample |
| `sqrtf` | `x = 2.064440` | `1.436816` | `1.436816` | identical on this sample |

## Analysis

### Overall

| Category | Conclusion |
| --- | --- |
| Heavy math | `atan/log/pow/sqrt` 下，默认 `_IQ` 端到端路径没有体现性能优势 |
| Simple unary ops | `fabsf` 下，IQMath 明显更慢，因为转换开销占主导 |
| Integer-like ops | `roundf/floorf` 这类 helper 可能更快，但它们不是 IQMath 现成 API，而是当前工程里的简化组合写法 |

### Why IQMath Is Usually Slower Here

1. 当前应用输入本身是 float，所以 IQMath 每次都要先做 `float -> _IQ`。
2. `fast_atanf`、`fast_log`、`fast_powf` 是非常短的近似实现，目标就是低延迟。
3. `sqrtf`、`fabsf` 这种函数在当前平台上本来就不慢，转换成 `_IQ` 反而增加步骤。
4. `pow` 不是直接 `_IQpow`，而是 `log + multiply + exp` 管线，所以天然更重。

### Special Notes For round/ceil/floor

| Function | Result | How to read it |
| --- | --- | --- |
| `roundf` | IQ-style helper faster | 当前 helper 只覆盖 benchmark 用到的普通数值路径，没有标准库那样完整的边界语义 |
| `ceilf` | IQ-style helper slower | 这里转换和 `_IQtoF` 判断开销盖过了收益 |
| `floorf` | IQ-style helper faster | 主要因为 helper 本质接近截断操作，路径很短 |

注意:

- 这三组不是在比较标准 `libm` 和 IQMath 官方同名 API。
- 当前比较的是 `libm` 函数和“基于 `_IQ` 表示的简化 helper”。
- 如果要拿去替换正式业务逻辑，需要补充 `NaN`、`Inf`、超大数、负边界值等测试。

### Practical Takeaways

| Scenario | Recommendation |
| --- | --- |
| float-domain app, only care about speed | 保留当前 `fast_*` 或直接用标准 float 路径更合适 |
| want IQMath advantage | 需要把整条数据链都改成 `_IQ`，而不是每次调用前后都做转换 |
| want fairer comparison | 增加 `atanf/logf/powf/sqrtf/fabsf` 标准库基线和误差统计 |

## Build And Run

```bash
cd examples/algorithm/omv_math
source /home/yanke/esp/esp-idf55/esp-idf/export.sh
idf.py build flash monitor
```
