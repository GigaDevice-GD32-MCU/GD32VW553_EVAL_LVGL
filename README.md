# LVGL V8.3 ported to the GD32VW553H EVAL

This project ports `LVGL V8.3.11` to the `GD32VW553H EVAL` platform for GUI demonstrations.

## Hardware Information

The `GD32VW553H Development Kit` is based on:

- `GD32VW553HMQ7` microcontroller (`RISC-V` core with FPU and DSP instructions, Wi-Fi 6 and BLE 5.2 connectivity)
- `4 MB` on-chip Flash memory and `320 KB` on-chip SRAM 
- `ILI9341` TFT display (`320 x 240`) driven over `SPI1` with DMA-based pixel streaming
- LCD control pins: `PA12` (CS), `PB13` (RS/DC), `PB12` (RESET)
- `USART0` virtual COM port for `printf` debug output
- On-chip RTC, TRNG, CAU/HAU/PKCAU crypto accelerators and QSPI Flash interface
- User LEDs, function keys, and universal expansion pin headers

## Project Information

- GUI framework: `LVGL V8.3.11`
- Toolchain: `IAR Embedded Workbench for RISC-V / GD32EmbeddedBuilder (Eclipse)`
- Target board: `GD32VW553H EVAL V1.2`
- Display configuration: `320 x 240 / RGB565 16-bit color / landscape`
- Draw buffers: `two partial buffers of 320 x 120 pixels, flushed asynchronously by DMA`

## Third-Party Components

| Category   | In use | Component | Version    | License |
| ---------- | ------ | --------- | ---------- | ------- |
| GUI        | `Yes`  | `LVGL`    | `V 8.3.11` | `MIT`   |

> When adding a new third-party library, update this table accordingly and retain its license text and copyright notices.
