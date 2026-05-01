//
// Created by spak on 4/28/23.
//

#ifndef LIBNEON_ENCODER_HPP
#define LIBNEON_ENCODER_HPP

#include <chrono>
#include <cstdint>
#include <driver/gpio.h>
#include <driver/rmt_tx.h>
#include <driver/rmt_types.h>
#include <neo/channel.hpp>
#include <ranges>
#include <vector>


namespace neo {
    using namespace std::chrono_literals;

    static constexpr std::uint32_t default_rmt_resolution_hz = 20'000'000;// 20MHz

    [[nodiscard]] constexpr rmt_tx_channel_config_t make_rmt_config(gpio_num_t gpio, bool dma = false, std::size_t mem_symbols = 64, std::size_t queue_depth = 4);

    using const_byte_range = std::ranges::subrange<std::vector<std::uint8_t>::const_iterator>;

    struct encoding_spec {
        std::chrono::nanoseconds t0h;
        std::chrono::nanoseconds t0l;
        std::chrono::nanoseconds t1h;
        std::chrono::nanoseconds t1l;
        std::chrono::nanoseconds res;
        channel_sequence chn_seq;
    };

    struct encoding {
        channel_sequence chn_seq;
        rmt_bytes_encoder_config_t rmt_encoder_cfg;
        rmt_symbol_word_t rmt_reset_sym;

        constexpr encoding(std::chrono::nanoseconds t0h, std::chrono::nanoseconds t0l,
                           std::chrono::nanoseconds t1h, std::chrono::nanoseconds t1l,
                           channel_sequence chn_seq_,
                           std::chrono::nanoseconds res = 500ns,
                           std::uint32_t resolution_hz = default_rmt_resolution_hz,
                           bool msb_first = true);

        constexpr encoding(encoding_spec spec);

        static constexpr encoding_spec ws2812b{400ns, 850ns, 800ns, 450ns, 50us, "grb"};
        static constexpr encoding_spec ws2812{350ns, 700ns, 800ns, 600ns, 50us, "grb"};
        static constexpr encoding_spec ws2811{500ns, 1200ns, 2000ns, 1300ns, 50us, "rgb"};
    };

    class led_encoder : private rmt_encoder_t {
        rmt_encoder_handle_t _bytes_encoder;
        rmt_encoder_handle_t _tail_encoder;
        rmt_symbol_word_t _reset_sym;
        channel_sequence _chn_seq;
        rmt_channel_handle_t _rmt_chn;
        std::vector<std::uint8_t> _buffer;

        static std::size_t _encode(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel, const void *primary_data, std::size_t data_size, rmt_encode_state_t *ret_state);
        static esp_err_t _reset(rmt_encoder_t *encoder);
        static esp_err_t _del(rmt_encoder_t *);

        std::size_t encode(rmt_channel_handle_t tx_channel, const void *primary_data, std::size_t data_size, rmt_encode_state_t *ret_state);
        esp_err_t reset();

    public:
        led_encoder();
        explicit led_encoder(encoding enc, rmt_tx_channel_config_t config);

        led_encoder(led_encoder const &) = delete;
        led_encoder &operator=(led_encoder const &) = delete;

        led_encoder(led_encoder &&) noexcept = default;
        led_encoder &operator=(led_encoder &&) noexcept = default;


        esp_err_t transmit_raw(const_byte_range data);

        template <class ColorIterator, class Extractor = default_channel_extractor<std::iter_value_t<ColorIterator>>>
        esp_err_t transmit(ColorIterator begin, ColorIterator end, Extractor const &extractor = {});

        ~led_encoder();
    };


}// namespace neo


namespace neo {

    constexpr rmt_tx_channel_config_t make_rmt_config(gpio_num_t gpio, bool dma, std::size_t mem_symbols, std::size_t queue_depth) {
        rmt_tx_channel_config_t cfg{
                .gpio_num = static_cast<gpio_num_t>(-1),
                .clk_src = RMT_CLK_SRC_DEFAULT,
                .resolution_hz = default_rmt_resolution_hz,
                .mem_block_symbols = 64,
                .trans_queue_depth = 4,
                .flags = {.invert_out = false,
                          .with_dma = false,
                          .io_loop_back = false,
                          .io_od_mode = false}};
        cfg.gpio_num = gpio;
        cfg.flags.with_dma = dma;
        cfg.mem_block_symbols = mem_symbols;
        cfg.trans_queue_depth = queue_depth;
        return cfg;
    }

    constexpr encoding::encoding(std::chrono::nanoseconds t0h, std::chrono::nanoseconds t0l,
                                 std::chrono::nanoseconds t1h, std::chrono::nanoseconds t1l,
                                 channel_sequence chn_seq_,
                                 std::chrono::nanoseconds res,
                                 std::uint32_t resolution_hz,
                                 bool msb_first)
        : chn_seq{chn_seq_},
          rmt_encoder_cfg{.bit0 = {.duration0 = std::uint16_t(double(t0h.count()) * double(resolution_hz) * 1.e-9),
                                   .level0 = 1,
                                   .duration1 = std::uint16_t(double(t0l.count()) * double(resolution_hz) * 1.e-9),
                                   .level1 = 0},
                          .bit1 = {.duration0 = std::uint16_t(double(t1h.count()) * double(resolution_hz) * 1.e-9),
                                   .level0 = 1,
                                   .duration1 = std::uint16_t(double(t1l.count()) * double(resolution_hz) * 1.e-9),
                                   .level1 = 0},
                          .flags = {.msb_first = msb_first}},
          rmt_reset_sym{.duration0 = std::uint16_t(double(res.count()) * double(resolution_hz) * 0.5e-9),
                        .level0 = 0,
                        .duration1 = std::uint16_t(double(res.count()) * double(resolution_hz) * 0.5e-9),
                        .level1 = 0} {}

    constexpr encoding::encoding(encoding_spec spec) : encoding{spec.t0h, spec.t0l, spec.t1h, spec.t1l, spec.chn_seq, spec.res} {}


    template <class ColorIterator, class Extractor>
    esp_err_t led_encoder::transmit(ColorIterator begin, ColorIterator end, Extractor const &extractor) {
        _buffer.clear();
        _buffer.reserve(std::distance(begin, end) * _chn_seq.size());
        _chn_seq.extract(begin, end, std::back_inserter(_buffer), extractor);
        return transmit_raw(_buffer);
    }

}// namespace neo

#endif//LIBNEON_ENCODER_HPP