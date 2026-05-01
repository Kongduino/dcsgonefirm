//
// Created by spak on 6/5/21.
//

#ifndef NEO_GRADIENT_HPP
#define NEO_GRADIENT_HPP

#include <neo/color.hpp>
#include <neo/math.hpp>

namespace neo {
    using blend_fn_t = srgb (&)(srgb l, srgb r, float t);

    [[maybe_unused]] [[nodiscard]] inline srgb blend_lerp(srgb l, srgb r, float t);
    [[maybe_unused]] [[nodiscard]] inline srgb blend_linear(srgb l, srgb r, float t);
    [[maybe_unused]] [[nodiscard]] inline srgb blend_round_down(srgb l, srgb, float);
    [[maybe_unused]] [[nodiscard]] inline srgb blend_round_up(srgb, srgb r, float);
    [[maybe_unused]] [[nodiscard]] inline srgb blend_nearest_neighbor(srgb l, srgb r, float t);

    template <class FwdIt1, class FwdIt2, class OutIt>
    OutIt broadcast_blend(FwdIt1 l_begin, FwdIt1 l_end, FwdIt2 r_begin, FwdIt2 r_end, OutIt out, float t, blend_fn_t blend_fn = blend_linear);

    struct gradient_entry {
        float pos = 0.f;
        srgb col = {};

        constexpr gradient_entry() = default;
        constexpr gradient_entry(float pos_, srgb col_) : pos{pos_}, col{col_} {}
    };

    struct safe_less {
        [[nodiscard]] constexpr bool operator()(float l, float r) const;

        [[nodiscard]] constexpr bool operator()(gradient_entry const &l, float r) const;

        [[nodiscard]] constexpr bool operator()(float l, gradient_entry const &r) const;

        [[nodiscard]] constexpr bool operator()(gradient_entry const &l, gradient_entry const &r) const;
    };

    template <class It, class OutIt>
    OutIt gradient_sample(It begin, It end, std::size_t n, OutIt out, float rotate = 0.f, float scale = 1.f, blend_fn_t blend_fn = blend_linear);

    template <class It>
    void gradient_normalize(It begin, It end);

    template <class It, class OutIt>
    OutIt gradient_make_uniform_from_colors(It begin, It end, OutIt out);


    template <class Container = std::vector<gradient_entry>>
    [[nodiscard]] Container gradient_make_uniform_from_colors(std::vector<srgb> colors);

}// namespace neo

namespace neo {
    constexpr bool safe_less::operator()(float l, float r) const {
        return std::abs(l - r) > std::numeric_limits<float>::epsilon() and l < r;
    }

    constexpr bool safe_less::operator()(gradient_entry const &l, float r) const {
        return (*this)(l.pos, r);
    }

    constexpr bool safe_less::operator()(float l, gradient_entry const &r) const {
        return (*this)(l, r.pos);
    }

    constexpr bool safe_less::operator()(gradient_entry const &l, gradient_entry const &r) const {
        return (*this)(l.pos, r.pos);
    }

    srgb blend_lerp(srgb l, srgb r, float t) {
        return l.lerp(r, t);
    }

    srgb blend_linear(srgb l, srgb r, float t) {
        return l.blend(r, t);
    }

    srgb blend_round_down(srgb l, srgb, float) {
        return l;
    }

    srgb blend_round_up(srgb, srgb r, float) {
        return r;
    }

    srgb blend_nearest_neighbor(srgb l, srgb r, float t) {
        return safe_less{}(t, 0.5f) ? l : r;
    }

    template <class It, class OutIt>
    OutIt gradient_sample(It begin, It end, std::size_t n, OutIt out, float rotate, float scale, blend_fn_t blend_fn) {
        if (begin == end) {
            return out;
        }
        constexpr auto less = safe_less{};
        auto ub = begin;
        auto last_t = std::numeric_limits<float>::infinity();
        for (std::size_t i = 0; i < n; ++i) {
            const float t = modclamp(scale * (rotate + float(i) / float(n)), 0.f, 1.f);
            ub = std::upper_bound(less(t, last_t) ? begin : ub, end, t, less);
            last_t = t;
            if (ub == begin) {
                *(out++) = begin->col;
                continue;
            }
            auto lb = std::prev(ub);
            if (ub == end) {
                *(out++) = lb->col;
                continue;
            }
            const float blend_f = std::clamp((t - lb->pos) / (ub->pos - lb->pos), 0.f, 1.f);
            *(out++) = blend_fn(lb->col, ub->col, blend_f);
        }
        return out;
    }


    template <class It>
    void gradient_normalize(It begin, It end) {
        std::sort(begin, end, safe_less{});
        float lo = std::numeric_limits<float>::infinity();
        float hi = -lo;
        for (auto it = begin; begin != end; ++it) {
            lo = std::min(lo, it->pos);
            hi = std::max(hi, it->pos);
        }
        for (auto it = begin; it != end; ++it) {
            it->pos = std::clamp((it->pos - lo) / (hi - lo), 0.f, 1.f);
        }
    }

    template <class It, class OutIt>
    OutIt gradient_make_uniform_from_colors(It begin, It end, OutIt out) {
        if (begin == end) {
            return out;
        }
        const float dt = 1.f / std::max(float(std::distance(begin, end) - 1), 1.f);
        for (auto t = 0.f; begin != end; ++begin, t += dt) {
            *(out++) = {t, *begin};
        }
        return out;
    }

    template <class Container>
    Container gradient_make_uniform_from_colors(std::vector<srgb> colors) {
        Container c{};
        gradient_make_uniform_from_colors(std::begin(colors), std::end(colors), std::back_inserter(c));
        return c;
    }

    template <class FwdIt1, class FwdIt2, class OutIt>
    OutIt broadcast_blend(FwdIt1 l_begin, FwdIt1 l_end, FwdIt2 r_begin, FwdIt2 r_end, OutIt out, float t, blend_fn_t blend_fn) {
        for (; l_begin != l_end and r_begin != r_end; ++l_begin, ++r_begin) {
            *(out++) = blend_fn(*l_begin, *r_begin, t);
        }
        return out;
    }
}// namespace neo
#endif//NEO_GRADIENT_HPP
