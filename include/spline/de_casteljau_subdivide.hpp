#pragma once

/// @file
/// @brief De Casteljau's algorithm with subdivision

#include <functional>
#include <type_traits>
#include "spline/concepts.hpp"
#include "spline/traits.hpp"

/// @namespace spline
/// @brief The Spline library
namespace spline {

namespace detail {

struct de_casteljau_subdivide_fn final {
    /// @brief De Casteljau's algorithm with subdivision
    /// @tparam InputIt Input iterator.
    /// @tparam OutputIt Output iterator.
    /// @tparam Scalar Scalar number type (typically float/double/long double).
    /// @tparam Mul binary operator accepting
    ///         InputIt::value_type and Scalar, returning an
    ///         InputIt::value_type.
    /// @tparam Add binary operator accepting two InputIt::value_type
    ///         and returning a InputIt::value_type.
    /// @param[in] first Iterator to first element of input range.
    /// @param[in] last Iterator to one past the last element of the range.
    /// @param[in] t Point at which to compute the polynomial value.
    /// @param[in] mul Vector-Scalar multiplication function object.
    /// @param[in] add Vector-Vector addition function object.
    /// @param[out] d_first Beginning of destination range.
    /// @return Output iterator to the element past the last element written.
    /// @pre @p d_first must point to a destination range where it is possible
    /// to
    ///      write  2 * std::distance(first, last) - 1 elements.
    template <class InputIt, class OutputIt, class Scalar,
              class Mul = multiplies<Scalar, xtd::iter_value_t<InputIt>>,
              class Add = std::plus<>>
    constexpr auto operator()(InputIt first, InputIt last, OutputIt d_first,
                              Scalar t, Mul mul = {}, Add add = {}) const
        -> std::enable_if_t<concepts::is_vector_space_v<
                                xtd::iter_value_t<InputIt>, Scalar, Add, Mul>,
                            OutputIt>
    {
        auto const num_coefficients = last - first;
        if (num_coefficients <= 0)  // Empty or non-sensical input range.
        {
            return d_first;
        }

        // Set up pointer to last element of output array (not one past last).
        OutputIt d_last = d_first + 2 * (num_coefficients - 1);
        // SPLINE_TRACE_ON_ENTRY

        auto lerp = [t, add, mul](auto a, auto b) {
            return add(mul(Scalar(1) - t, a), mul(t, b));
        };

        // Special case for 1 or 2 coefficients.
        if (num_coefficients == 1) {
            *d_first++ = *first;
            // SPLINE_TRACE_N_1
            return d_first;
        }
        if (num_coefficients == 2) {
            *d_first++ = *first;
            *d_first++ = lerp(*first, *--last);
            *d_first++ = *last;
            // SPLINE_TRACE_N_2
            return d_first;
        }

        // Copy c_0^0 from input to first element of output.
        *d_first++ = *first;

        // Copy c_n^0 from input to last element of output.
        *d_last-- = *--last;

        // Lerp c_i^0 with c_{i+1}^0 for i \in [0, n-2], place results in left
        // portion of output. The first element written is in it's final
        // location, all subsequent elements are intermediate values that will
        // be overwritten during subsequent row iterations.
        while (first++ != last - 1) {
            *d_first++ = lerp(*(first - 1), *first);
        }
        // After this loop exits, d_first points to one past the second to last
        // element of row one, i.e., one past c_{n - 1}^1. This is the middle of
        // the output array.

        // Lerp c_{n-1}^0 with c_n^0, place result in output. After this step,
        // we never look at input range again since we have placed all results
        // (both final and temporary) in the output range.
        *d_last-- = lerp(*(last - 1), *last);
        // After this line, d_last points to index 2*n - 2, i.e., where c_n^2
        // needs to be placed.

        // SPLINE_TRACE_ROW_0
        for (int r = 2; r < num_coefficients - 1; ++r) {
            *d_last = lerp(*--d_first, *(d_last + 1));
            --d_last;
            // On row index r >= 1, mean we are reading from row r to produce
            // row r
            // + 1
            // Step 1: lerp second to last element of row r with last element of
            // row r, place result in appropriate location on right segment of
            // output.  I think this is:
            // c_{n - r}^{r + 1} = lerp(c_{n - r}^r, c_{n - r + 1}^r), noting
            //      i = 4                  i = 2          i = 5
            // that the two inputs to lerp are not adjacent in the output array,
            // but the return value is placed one element to the left of the
            // right hand input element.
            for (int j = 0; j < num_coefficients - r - 1; ++j) {
                *d_first = lerp(*(d_first - 1), *d_first);
                --d_first;
            }
            // d_first points to c_{r-2}^{r-2}
            d_first += num_coefficients - r;
            // SPLINE_TRACE_ROW_N
        }

        *d_last = lerp(*(d_last - 1), *(d_last + 1));

        // SPLINE_TRACE_ON_EXIT
        return d_last + num_coefficients;
    }
};
}  // namespace detail

/// @brief Global function object performing De Casteljau's algorithm with
///     subdivision
inline constexpr detail::de_casteljau_subdivide_fn de_casteljau_subdivide{};
}  // namespace spline
