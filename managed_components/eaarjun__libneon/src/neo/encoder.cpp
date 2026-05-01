//
// Created by spak on 4/28/23.
//

#include <driver/rmt_tx.h>
#include <esp_log.h>
#include <neo/encoder.hpp>

namespace neo {

    namespace {
        constexpr rmt_copy_encoder_config_t rmt_copy_encoder_config{};
        constexpr rmt_transmit_config_t rmt_transmit_config{.loop_count = 0, .flags = {.eot_level = 0}};
    }// namespace

    esp_err_t led_encoder::transmit_raw(const_byte_range data) {
        if (_rmt_chn == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        if (data.empty()) {
            ESP_LOGW("NEO", "You are transmitting empty color data.");
            return ESP_OK;
        }
        return rmt_transmit(_rmt_chn, this, data.data(), data.size(), &rmt_transmit_config);
    }

    std::size_t led_encoder::encode(rmt_channel_handle_t tx_channel, const void *primary_data, std::size_t data_size, rmt_encode_state_t *ret_state) {
        assert(_bytes_encoder and _bytes_encoder->encode);
        assert(_tail_encoder and _tail_encoder->encode);
        std::size_t encoded_symbols = 0;
        encoded_symbols += _bytes_encoder->encode(_bytes_encoder, tx_channel, primary_data, data_size, ret_state);
        if (ret_state != nullptr and *ret_state != RMT_ENCODING_COMPLETE) {
            return encoded_symbols;
        }
        encoded_symbols += _tail_encoder->encode(_tail_encoder, tx_channel, &_reset_sym, sizeof(_reset_sym), ret_state);
        return encoded_symbols;
    }

    esp_err_t led_encoder::reset() {
        if (auto const r = rmt_encoder_reset(_bytes_encoder); r != ESP_OK) {
            return r;
        }
        if (auto const r = rmt_encoder_reset(_tail_encoder); r != ESP_OK) {
            return r;
        }
        return ESP_OK;
    }

    led_encoder::led_encoder()
        : rmt_encoder_t{.encode = nullptr, .reset = nullptr, .del = nullptr},
          _bytes_encoder{nullptr},
          _tail_encoder{nullptr},
          _reset_sym{},
          _chn_seq{},
          _rmt_chn{nullptr} {
        static_assert(static_cast<rmt_encoder_t *>(static_cast<led_encoder *>(nullptr)) == static_cast<led_encoder *>(nullptr));
    }

    led_encoder::led_encoder(encoding enc, rmt_tx_channel_config_t config)
        : rmt_encoder_t{.encode = &_encode, .reset = &_reset, .del = &_del},
          _bytes_encoder{nullptr},
          _tail_encoder{nullptr},
          _reset_sym{enc.rmt_reset_sym},
          _chn_seq{enc.chn_seq},
          _rmt_chn{nullptr} {
        ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc.rmt_encoder_cfg, &_bytes_encoder));
        ESP_ERROR_CHECK(rmt_new_copy_encoder(&rmt_copy_encoder_config, &_tail_encoder));
        ESP_ERROR_CHECK(rmt_new_tx_channel(&config, &_rmt_chn));
        ESP_ERROR_CHECK(rmt_enable(_rmt_chn));
    }

    led_encoder::~led_encoder() {
        if (_bytes_encoder != nullptr) {
            ESP_ERROR_CHECK(rmt_del_encoder(_bytes_encoder));
            _bytes_encoder = nullptr;
        }
        if (_tail_encoder != nullptr) {
            ESP_ERROR_CHECK(rmt_del_encoder(_tail_encoder));
            _tail_encoder = nullptr;
        }
        if (_rmt_chn != nullptr) {
            ESP_ERROR_CHECK(rmt_disable(_rmt_chn));
            ESP_ERROR_CHECK(rmt_del_channel(_rmt_chn));
            _rmt_chn = nullptr;
        }
    }

    esp_err_t led_encoder::_reset(rmt_encoder_t *encoder) {
        return reinterpret_cast<led_encoder *>(encoder)->reset();
    }

    std::size_t led_encoder::_encode(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel, const void *primary_data, std::size_t data_size, rmt_encode_state_t *ret_state) {
        return reinterpret_cast<led_encoder *>(encoder)->encode(tx_channel, primary_data, data_size, ret_state);
    }

    esp_err_t led_encoder::_del(rmt_encoder_t *) {
        ESP_LOGE("NEO", "Attempt to delete led_encoder via C API.");
        std::abort();
    }

}// namespace neo
