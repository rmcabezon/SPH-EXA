/*
 * MIT License
 *
 * Copyright (c) 2022 CSCS, ETH Zurich, University of Zurich, University of Basel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*! @file
 * @brief Unit tests for ParticlesData
 *
 * @author Noah Kubli <noah.kubli@uzh.ch>
 * @author Sebastian Keller <sebastian.f.keller@gmail.com>
 */

#pragma once

#include <array>

#include "cstone/fields/particles_get.hpp"
#include "cstone/fields/field_states.hpp"

namespace constexpr_to_string
{

inline constexpr char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*! @struct to_string_t
 * @brief Provides the ability to convert any integral to a string at compile-time.
 * @tparam N Number to convert
 * @tparam base Desired base, can be from 2 to 36
 *
 * adapted from git@github.com:tcsullivan/constexpr-to-string.git
 */
template<auto N, int base, typename char_type, std::enable_if_t<std::is_integral_v<decltype(N)>, int> = 0,
         std::enable_if_t<(base > 1 && base < sizeof(digits)), int> = 0>
class to_string_t
{
public:
    // The lambda calculates what the string length of N will be, so that `buf`
    // fits to the number perfectly.
    char_type buf[([]() constexpr noexcept {
        unsigned int len = N > 0 ? 1 : 2;
        for (auto n = N; n; len++, n /= base)
            ;
        return len;
    }())] = {};

    //! Constructs the object, filling `buf` with the string representation of N.
    constexpr to_string_t() noexcept
    {
        auto ptr = end();
        *--ptr   = '\0';
        if (N != 0)
        {
            for (auto n = N; n; n /= base)
                *--ptr = digits[(N < 0 ? -1 : 1) * (n % base)];
            if (N < 0) *--ptr = '-';
        }
        else { buf[0] = '0'; }
    }

    // Support implicit casting to `char *` or `const char *`.
    constexpr operator char_type*() noexcept { return buf; }
    constexpr operator const char_type*() const noexcept { return buf; }

    constexpr auto size() const noexcept { return sizeof(buf) / sizeof(buf[0]); }
    // Element access
    constexpr auto        data() noexcept { return buf; }
    constexpr auto        data() const noexcept { return buf; }
    constexpr auto&       operator[](unsigned int i) noexcept { return buf[i]; }
    constexpr const auto& operator[](unsigned int i) const noexcept { return buf[i]; }
    constexpr auto&       front() noexcept { return buf[0]; }
    constexpr const auto& front() const noexcept { return buf[0]; }
    constexpr auto&       back() noexcept { return buf[size() - 1]; }
    constexpr const auto& back() const noexcept { return buf[size() - 1]; }
    // Iterators
    constexpr auto begin() noexcept { return buf; }
    constexpr auto begin() const noexcept { return buf; }
    constexpr auto end() noexcept { return buf + size(); }
    constexpr auto end() const noexcept { return buf + size(); }
};

} // namespace constexpr_to_string

//! Simplifies use of `to_string_t` from `to_string_t<N>()` to `to_string<N>`.
template<auto N, int base = 10, typename char_type = char>
constexpr constexpr_to_string::to_string_t<N, base, char_type> to_string;

template<StructuralString S>
struct CT
{
    constexpr CT() noexcept {}
    constexpr operator char*() noexcept { return S.value; }
    constexpr operator const char*() const noexcept { return S.value; }
};

template<StructuralString S>
constexpr CT<S> CT_use;

template<StructuralString Prefix, size_t... Is>
constexpr std::array<const char*, sizeof...(Is)> enumerateFieldNames_helper(std::index_sequence<Is...>)
{
    constexpr size_t                            N = sizeof...(Is);
    std::array<const char*, N>                  names;
    [[maybe_unused]] std::initializer_list<int> list{
        (names[Is] = CT_use<Prefix + StructuralString(to_string<Is>.buf)>, 0)...};
    return names;
}

template<StructuralString Prefix, size_t N>
constexpr std::array<const char*, N> enumerateFieldNames()
{
    return enumerateFieldNames_helper<Prefix>(std::make_index_sequence<N>{});
}

namespace cooling
{

template<class T>
class ChemistryData : public cstone::FieldStates<ChemistryData<T>>
{
public:
    std::vector<T> x;

    inline static constexpr std::array fieldNames = enumerateFieldNames<"Y", 2>();
};

} // namespace cooling
