# LVGL Ported to GD32VW553H EVAL

## Overview

This project ports `LVGL` to the `GD32VW553H EVAL` evaluation board for GUI demonstrations.

The board features a `GD32VW553HM` microcontroller based on the `RISC-V` core ,
along with a `320 x 240` TFT display.
The project uses landscape orientation with the `RGB565` pixel format and 16-bit color depth.

Project toolchains include `IAR` and `GD32EmbeddedBuilder`.

## Benchmark

The following benchmark results were obtained using LVGL 9.6.0 with double buffering.

| Name                      | Avg. CPU | Avg. FPS | Avg. time | render time | flush time |
|:------------------------- | --------:| --------:| ---------:| -----------:| ----------:|
| Empty screen              | 88%      | 28       | 30        | 12          | 18         |
| Moving wallpaper          | 97%      | 26       | 34        | 30          | 4          |
| Single rectangle          | 19%      | 60       | 1         | <1          | 1          |
| Multiple rectangles       | 95%      | 48       | 18        | 11          | 7          |
| Multiple RGB images       | 30%      | 60       | 4         | 4           | <1         |
| Multiple ARGB images      | 84%      | 54       | 16        | 16          | <1         |
| Rotated ARGB images       | 99%      | 28       | 31        | 31          | <1         |
| Multiple labels           | 99%      | 56       | 15        | 15          | <1         |
| Screen sized text         | 100%     | 7        | 123       | 121         | 2          |
| Multiple arcs             | 27%      | 60       | 1         | 1           | <1         |
| Containers                | 44%      | 54       | 18        | 18          | <1         |
| Containers with overlay   | 93%      | 21       | 42        | 40          | 2          |
| Containers with opa       | 55%      | 52       | 20        | 20          | <1         |
| Containers with opa_layer | 74%      | 34       | 35        | 34          | 1          |
| Containers with scrolling | 98%      | 18       | 49        | 47          | 2          |
| Widgets demo              | 99%      | 21       | 37        | 33          | 4          |
| All scenes avg.           | 75%      | 39       | 29        | 27          | 2          |

## Specification

- **MCU:** GD32VW553HM (RISC-V, 160MHz)
- **RAM:** 320KB internal SRAM
- **Flash:** 4096KB internal
- **GPU:** None
- **Resolution:** 320*240
- **Display Size:** 2.2 inch
- **Interface:** SPI
- **Color Depth:** 16-bit RGB565
- **Technology:** a-Si TFT
- **DPI:** 180 px/inch
- **Touch Pad:** None

## Getting Started

The following steps describe a general workflow. Confirm the project paths, software versions,
power supply requirements, and jumper settings against the actual project and the board manual.
This project currently supports two integrated development environments (IDEs): IAR and GD32EmbeddedBuilder. This document uses GD32EmbeddedBuilder to demonstrate how to build, download, and debug the project.

### Hardware Setup

1. Confirm that the board model is `GD32VW553H EVAL`.
2. Check the RGB display and resistive touch panel connections according to the board manual.
3. Connect the on-board GDLINK debug interface to your PC, and configure the power supply and jumpers as required.

### Software Setup

1. Install `GD32EmbeddedBuilder` for your operating system. Refer to the GD32EmbeddedBuilder software requirements for details.
2. Install the appropriate GDLink driver software and confirm that your PC recognizes the debugger.

### Build the Project

1. Clone this repository.
```shell
  git clone "Repository URL"
  ```
2. Open `GD32EmbeddedBuilder` and click **Browse...** to select a local directory as your workspace.
3. Import the project by selecting **File > Import...** and choosing the project directory (`./Template/GD32EBuilder_project`).
4. Build the project by right-clicking the project in the project explorer and selecting **Build Project**.
5. To download and debug the project, right-click the project in the project explorer and select
	**Debug As > Debug Configurations... > GDB General Debugging**. Configure the settings for your setup,
	then click **Debug** to download the firmware and start debugging.

### Debugging

Start a debug session using your IDE and GDLink to set breakpoints, step through code, and inspect variables.
If display or touch issues occur, first check peripheral initialization, display refresh, and touch input handling.
The available debug options depend on your IDE and project configuration.

## Third-Party Components

| Category | In Use | Component | Version | License |
| --- | --- | --- | --- | --- |
| GUI | Yes | `LVGL` | `V9.6.0` | `MIT` |

When adding a third-party library, update this table and retain its license text and copyright notices.

## Contribution and Support

If you find any issues with the development board feel free to open an Issue in this repository. For LVGL related issues (features, bugs, etc) please use the main [lvgl repository](https://github.com/lvgl/lvgl).

If you found a bug and found a solution too please send a Pull request. If you are new to Pull requests refer to [Our Guide](https://docs.lvgl.io/master/CONTRIBUTING.html#pull-request) to learn the basics.
