# libneon

[![Component Registry](https://components.espressif.com/components/eaarjun/libneon/badges)](https://components.espressif.com/components/eaarjun/libneon)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

**libneon** is a lightweight, dependency-free Neopixel (WS2812/WS2812B) driver library for the ESP-IDF framework. It utilizes the ESP32's built-in RMT (Remote Control) peripheral for highly accurate, non-blocking LED signal generation.

This library includes built-in visual effects and is designed to be highly efficient, modular, and easy to integrate into your embedded projects.

---

## ✨ Features

* **Zero Dependencies:** Pure ESP-IDF implementation
* **Hardware-Driven:** Uses the RMT peripheral to precisely time the Neopixel protocol without blocking the CPU
* **Built-in Effects:** Includes reusable lighting effects, gradients, and animations
* **Modular Design:** Clean separation between encoder, effects, and examples
* **Multi-Target Support:** Compatible with:

  * ESP32
  * ESP32-C3
  * ESP32-S3
  * ESP32-C6

---

## 📦 Installation

You can easily add this component using the ESP Component Registry:

```bash
idf.py add-dependency "eaarjun/libneon^1.0.2"
```

---

## 🚀 Quick Start

```cpp
#include <neo/encoder.hpp>
#include <examples.hpp>

extern "C" void app_main()
{
    neo::led_encoder encoder(
        neo::encoding::ws2812b,
        neo::make_rmt_config(GPIO_NUM_8)
    );

    examples::run_all(encoder, 16);
}
```

---

## 🎨 Built-in Examples

The library provides ready-to-use animations:

* 🌈 Gradient / Rainbow
* 💓 Pulse effects
* 🌀 Spinner effects
* 🔴 Simple blink
* 🎨 Static gradients
* 🔥 Composite animations (multiple effects with transitions)

---

## 🧠 Architecture Overview

```text
FX Engine → Callback → Encoder (RMT) → LED Strip
```

* **FX Engine:** Generates animation frames
* **Alarm:** Drives frame timing (FPS-based)
* **Encoder:** Converts frames into RMT signals
* **RMT Peripheral:** Handles precise signal transmission

---

## 📌 Notes

* Ensure correct GPIO selection based on your hardware
* WS2812 LEDs use **GRB color order**
* Timing is handled automatically via RMT

---

## 📄 License

This project is licensed under the MIT License.
