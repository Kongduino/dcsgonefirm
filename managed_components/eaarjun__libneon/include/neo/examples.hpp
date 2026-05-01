#pragma once

#include <cstddef>
#include <driver/gpio.h>

void example_simple_blink(gpio_num_t gpio_pin, std::size_t num_leds);
void example_static_gradient(gpio_num_t gpio_pin, std::size_t num_leds);
void example_gradient_fx(gpio_num_t gpio_pin, std::size_t num_leds);
void example_spinner_fx(gpio_num_t gpio_pin, std::size_t num_leds);
void example_pulse_fx(gpio_num_t gpio_pin, std::size_t num_leds);
void example_composite(gpio_num_t gpio_pin, std::size_t num_leds);

void examples_run_all(gpio_num_t gpio_pin, std::size_t num_leds);