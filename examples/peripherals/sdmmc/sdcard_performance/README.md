# SD Card Performance Test

This example measures the sequential read/write throughput of an SD card on the **ESP32-P4** using the SDMMC interface (slot 0, 4-bit, UHS-I SDR50 @ 100MHz).

The test mounts the card with FATFS, then writes a 16 MB file and reads it back using a 64 KB DMA-aligned buffer, printing the throughput in MB/s.

## Hardware

* **Board:** ESP32-P4-Function-EV-Board
* **Card slot:** on-board Micro SD slot (powered by the on-chip LDO VO4)

The pin mapping is fixed for the ESP32-P4-Function-EV-Board SD slot:

| Signal | GPIO |
|:------:|:----:|
| CLK    | 43   |
| CMD    | 44   |
| D0     | 39   |
| D1     | 40   |
| D2     | 41   |
| D3     | 42   |

> Note: To reach SDR50 reliably, the input sampling delay phase is fixed to `SDMMC_DELAY_PHASE_1`. With the default phase, reads at 100MHz may show intermittent CRC errors (0x109); if errors still appear, try `PHASE_2` / `PHASE_3`.

## How to Use

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

The test runs once automatically in `app_main()` and prints the results.

## Example Output

Measured on the ESP32-P4-Function-EV-Board:

```
I (1405) sdcard_perf: Starting SD card performance test (16 MB)
I (2255) sdcard_perf: WRITE 16384 KB in 842 ms -> 19.91 MB/s
I (2935) sdcard_perf: READ  16384 KB in 685 ms -> 24.48 MB/s
I (2935) sdcard_perf: SD card performance test done
```

| Operation | Throughput |
|:---------:|:----------:|
| Write     | ~19.9 MB/s |
| Read      | ~24.5 MB/s |

> Actual throughput depends on the SD card grade/brand and the negotiated bus speed.
