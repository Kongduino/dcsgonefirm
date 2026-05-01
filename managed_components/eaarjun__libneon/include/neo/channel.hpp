//
// Created by spak on 4/30/23.
//

#ifndef LIBNEON_CHANNEL_HPP
#define LIBNEON_CHANNEL_HPP

#include <bit>
#include <string_view>
#include <vector>
#include <cstdint>
namespace neo {

    /**
     * @note When adding values, make sure @ref channel_sequence is large enough to accommodate enough many of them.
     *  Zero is used as signalling value
     */
    enum struct channel : char {
        r = 'r',
        g = 'g',
        b = 'b'
    };

    template <class Color>
    struct default_channel_extractor {
        [[nodiscard]] constexpr std::uint8_t operator()(Color col, channel chn) const;
    };

    struct channel_sequence {
        std::string_view sequence{};

        constexpr channel_sequence() = default;

        explicit constexpr channel_sequence(std::string_view seq);

        constexpr channel_sequence(const char *seq);

        [[nodiscard]] constexpr std::size_t size() const;

        constexpr bool operator==(channel_sequence const &other) const;

        constexpr bool operator!=(channel_sequence const &other) const;

        [[nodiscard]] constexpr auto begin() const;

        [[nodiscard]] constexpr auto end() const;

        template <class Color, class OutputIterator, class Extractor = default_channel_extractor<Color>>
        OutputIterator extract(Color const &col, OutputIterator out, Extractor const &extractor = {}) const;

        template <class ColorIterator, class OutputIterator, class Extractor = default_channel_extractor<std::iter_value_t<ColorIterator>>>
        OutputIterator extract(ColorIterator begin, ColorIterator end, OutputIterator out, Extractor const &extractor = {}) const;

        template <class Color>
        [[nodiscard]] std::vector<std::uint8_t> extract(Color const &col) const;
    };

}// namespace neo

namespace neo {

    template <class Color>
    constexpr std::uint8_t default_channel_extractor<Color>::operator()(Color col, channel chn) const {
        return col[chn];
    }

    constexpr channel_sequence::channel_sequence(std::string_view seq) : sequence{seq} {}

    constexpr channel_sequence::channel_sequence(const char *seq) : sequence{seq} {}

    constexpr std::size_t channel_sequence::size() const {
        return sequence.size();
    }

    constexpr bool channel_sequence::operator==(channel_sequence const &other) const {
        return sequence == other.sequence;
    }

    constexpr bool channel_sequence::operator!=(channel_sequence const &other) const {
        return sequence == other.sequence;
    }

    constexpr auto channel_sequence::begin() const {
        return std::begin(sequence);
    }

    constexpr auto channel_sequence::end() const {
        return std::end(sequence);
    }

    template <class Color, class OutputIterator, class Extractor>
    OutputIterator channel_sequence::extract(Color const &col, OutputIterator out, Extractor const &extractor) const {
        for (auto chn : *this) {
            *(out++) = extractor(col, std::bit_cast<channel>(chn));
        }
        return out;
    }

    template <class ColorIterator, class OutputIterator, class Extractor>
    OutputIterator channel_sequence::extract(ColorIterator begin, ColorIterator end, OutputIterator out, Extractor const &extractor) const {
        for (auto it = begin; it != end; ++it) {
            for (auto chn : *this) {
                *(out++) = extractor(*it, std::bit_cast<channel>(chn));
            }
        }
        return out;
    }

    template <class Color>
    std::vector<std::uint8_t> channel_sequence::extract(Color const &col) const {
        std::vector<std::uint8_t> retval;
        retval.reserve(size());
        extract(col, std::back_inserter(retval));
        return retval;
    }
}// namespace neo

#endif//LIBNEON_CHANNEL_HPP
