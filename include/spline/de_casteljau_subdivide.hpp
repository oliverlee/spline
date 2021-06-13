#pragma once

/// @file
/// @brief De Casteljau's algorithm with subdivision

#include <functional>
#include <iterator>
#include <numeric>
#include <type_traits>
#include "spline/concepts.hpp"
#include "spline/traits.hpp"

/// @namespace spline
/// @brief The Spline library
namespace spline {

namespace detail {

struct de_casteljau_subdivide_fn final {
    /// @brief De Casteljau's algorithm with subdivision
    /// @tparam InputIt Input iterator. Must meet the requirements of
    ///         LegacyInputIterator.
    /// @tparam OutputIt Output iterator. Must meet the requirements of
    ///         LegacyBidirectionalIterator.
    /// @tparam Scalar Scalar number type (typically float/double/long double).
    /// @tparam Mul binary operator accepting
    ///         Scalar and InputIt::value_type, returning an
    ///         InputIt::value_type.
    /// @tparam Add binary operator accepting two InputIt::value_type
    ///         and returning a InputIt::value_type.
    /// @param first Iterator to first element of input range.
    /// @param last Iterator to one past the last element of the range.
    /// @param t Point at which to compute the polynomial value.
    /// @param mul Scalar-Vector multiplication function object.
    /// @param add Vector-Vector addition function object.
    /// @param d_first Beginning of destination range.
    /// @return Output iterator to the element past the last element written.
    /// @pre @p d_first must point to a destination range where it is possible
    ///      to write  2 * std::distance(first, last) - 1 elements.
    template <class InputIt, class OutputIt, class Scalar,
              class Mul = multiplies<Scalar, xtd::iter_value_t<InputIt>>,
              class Add = std::plus<>>
    constexpr auto operator()(InputIt first, InputIt last, OutputIt d_first,
                              Scalar t, Mul mul = {}, Add add = {}) const
        -> std::enable_if_t<
            std::conjunction_v<
                std::is_base_of<std::input_iterator_tag,
                                xtd::iter_category_t<InputIt>>,
                std::is_base_of<std::bidirectional_iterator_tag,
                                xtd::iter_category_t<OutputIt>>,
                std::is_same<xtd::iter_value_t<InputIt>,
                             xtd::iter_value_t<OutputIt>>,
                concepts::is_vector_space<xtd::iter_value_t<InputIt>, Scalar,
                                          Add, Mul>>,
            OutputIt>
    {
        // initialize left half of destination range with b0^(0) .. bn^(0), then
        // [b0^(0), b1^(0), ..., bn^(0), X, ..., X, X]
        auto d_last = std::copy(first, last, d_first);
        if (d_last == d_first) {
            return d_last;
        }

        // point to mid and last element in destination range
        const auto n = std::distance(d_first, d_last) - 1;
        const auto d_mid = std::prev(d_last);
        std::advance(d_last, n - 1);

        // copy the bn^(0) to the last element in the dest
        // [b0^(0), b1^(0), ..., bn^(0), X, ..., X, bn^(0)]
        *d_last = *d_mid;

        // evaluate reduced order polynomials with recurrence relation
        for (; d_first != d_last; ++d_first) {
            // in first iteration:
            // [b0^(0), b0^(1), ..., bn-1^(1), X, ..., X, bn^(0)]
            std::adjacent_difference(d_first, std::next(d_mid), d_first,
                                     [t, mul, add](auto b, auto a) {
                                         return add(mul((Scalar{1} - t), a),
                                                    mul(t, b));
                                     });

            // in first iteration:
            // [b0^(0), b0^(1), ..., bn-1^(1), X, ..., bn-1^(1), bn^(0)]
            *--d_last = *d_mid;
        }

        return std::next(d_mid, n + 1);
    }
};
}  // namespace detail

/// @brief Global function object performing De Casteljau's algorithm with
///     subdivision
inline constexpr detail::de_casteljau_subdivide_fn de_casteljau_subdivide{};
}  // namespace spline
