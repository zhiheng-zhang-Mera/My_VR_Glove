# My VR Glove - High Performance Edition
# VR 手套 - 高性能固件版

> **Based on LucidGloves | Optimized for ESP32 & Low Latency**
> **基于 LucidGloves 项目 | 针对 ESP32 与低延迟优化**

![Glove Preview](path/to/image.png)

## Introduction / 简介

This is a heavily modified fork of the original [LucidGloves](https://github.com/LucidVR/lucidgloves) firmware. While the original project provides an excellent foundation, this version is re-engineered with a single goal: **Extreme Responsiveness**. We have stripped away heavy abstraction layers, optimized memory management, and unlocked the hardware capabilities of the ESP32 to minimize "Motion-to-Photon" latency.

这是一个基于原版 [LucidGloves](https://github.com/LucidVR/lucidgloves) 固件的深度修改版。虽然原项目提供了极佳的基础，但本版本经过重新设计，核心目标只有一个：**极致的响应速度**。我们剥离了沉重的抽象层，优化了内存管理，并解锁了 ESP32 的硬件潜能，以最小化“动作到光子”的延迟。

---

## Key Improvements / 核心改进

### 1. Single Pass Linear Decoding (极速线性解码)
* **English:** Implements a raw pointer traversal algorithm. Instead of using `String` objects or repetitive scanning, use `strtol` to parse numbers and immediately advance the pointer. The data stream is parsed in a single pass (O(n) complexity), saving microseconds per packet.
* **中文:** 实现了原始指针遍历算法。不再使用 `String` 对象或重复扫描，而是使用 `strtol` 解析数字并立即移动指针。数据流仅需单次扫描即可完成解析（O(n) 复杂度），每包数据节省数微秒的处理时间。

### 2. Non-Blocking Serial I/O (非阻塞串口通信)
* **English:** Uses a ring-buffer approach. The loop consumes available bytes instantly without waiting for a full packet (`readStringUntil` blocking is removed). This eliminates micro-stutters caused by USB transmission delays.
* **中文:** 采用环形缓冲区机制。主循环会立即处理可用字节，而无需等待完整数据包（移除了 `readStringUntil` 等阻塞函数）。这消除了由 USB 传输延迟引起的微卡顿。

### 3. 250Hz Servo Haptics (250Hz 极速触觉反馈)
* **English:** Overclocks the ESP32 Hardware Timers to **250Hz** (4ms latency), compared to the standard 50Hz (20ms). This makes haptic feedback feel solid and instantaneous.
    * *⚠️ Warning: Ensure the used servos support high refresh rates.*
* **中文:** 将 ESP32 硬件定时器超频至 **250Hz**（4ms 延迟），远快于标准的 50Hz（20ms）。这使得触觉反馈手感扎实且瞬时响应。
    * *⚠️ 警告：请确保使用的舵机支持高刷新率。*

### 4. "Soft Fuse" Power Limiter (软保险丝功率限制)
* **English:** A dynamic scaling algorithm monitors the requested force. If the total power budget is exceeded, it proportionally scales down all motors to prevent the ESP32 from browning out or restarting due to USB current limits.
* **中文:** 动态缩放算法会监控请求的力度。如果超过总功率预算，系统将按比例降低所有电机的输出，以防止 ESP32 因 USB 电流限制而掉电或重启。

---

## Data reading test / 数据读取测试


---

## Installation / 安装指南

1.  **Environment / 环境**:
    * Use **VS Code** with **PlatformIO**. (Arduino IDE is not recommended for this structure).
    * 使用 **VS Code** 配合 **PlatformIO** 插件。（不建议使用 Arduino IDE）。

2.  **Configuration / 配置**:
    * Open `src/Config.h` and select your hardware pins.
    * **Crucial:** Ensure `SERIAL_BAUD_RATE` matches your driver software (Default: **921600**).
    * 打开 `src/Config.h` 选择您的硬件引脚。
    * **关键:** 确保 `SERIAL_BAUD_RATE` 与驱动软件匹配（默认：**921600**）。

3.  **Upload / 上传**:
    * Connect your ESP32.
    * Check `platformio.ini` to set your `upload_port`.
    * Run "Upload".
    * 连接 ESP32，检查 `platformio.ini` 设置端口，点击“上传”。

```ini
; platformio.ini settings
[env:esp32dev]
monitor_speed = 921600
build_flags = -DCORE_DEBUG_LEVEL=0
```

## Disclaimer / 免责声明

  * Overclocking: The 250Hz servo frequency is aggressive. If servos overheat, revert to 50Hz in src/Controller/Haptics.cpp.
  * Power: The "Soft Fuse" is software-level protection; it cannot prevent hardware shorts.
  * 超频: 250Hz 舵机频率较为激进。如果舵机过热，请在 src/Controller/Haptics.cpp 中恢复为 50Hz。
  * 电源: “软保险丝”仅为软件层面的保护，无法防止硬件短路。
