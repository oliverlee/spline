#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <type_traits>

/// @file
/// @brief A spline object

namespace spline {

namespace detail {

template <class Iter, typename std::iterator_traits<Iter>::difference_type N>
class stride_iterator {
  public:
    using iterator_type = Iter;
    using iterator_category =
        typename std::iterator_traits<Iter>::iterator_category;
    using value_type = typename std::iterator_traits<Iter>::value_type;
    using difference_type =
        typename std::iterator_traits<Iter>::difference_type;
    using pointer = typename std::iterator_traits<Iter>::pointer;
    using reference = typename std::iterator_traits<Iter>::reference;

  private:
    constexpr iterator_type strided_base() const
    {
        return base_ + (N * offset_);
    }

    iterator_type base_{};
    difference_type offset_{};

  public:
    stride_iterator() = default;

    constexpr stride_iterator(iterator_type it, difference_type offset = {}) :
        base_{it}, offset_{offset}
    {}

    constexpr iterator_type base() const { return strided_base(); }
    constexpr reference operator*() const { return *strided_base(); }
    constexpr pointer operator->() const
    {
        // TODO
        throw std::runtime_error{"operator->"};
        return *this;
    };

    constexpr auto operator[](difference_type n) const
        -> decltype(strided_base()[n])
    {
        return strided_base()[n];
    }

    constexpr stride_iterator& operator++()
    {
        ++offset_;
        return *this;
    }
    constexpr stride_iterator& operator--()
    {
        --offset_;
        return *this;
    }
    constexpr stride_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }
    constexpr stride_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    constexpr stride_iterator operator+(difference_type n) const
    {
        return {base_, offset_ + n};
    }
    constexpr stride_iterator operator-(difference_type n) const
    {
        return {base_, offset_ - n};
    }
    constexpr stride_iterator& operator+=(difference_type n)
    {
        offset_ += n;
        return *this;
    }
    constexpr stride_iterator& operator-=(difference_type n)
    {
        offset_ += n;
        return *this;
    }

  private:
    friend constexpr stride_iterator operator+(difference_type n,
                                               const stride_iterator& it)
    {
        return it + n;
    }

    friend constexpr auto operator-(const stride_iterator& lhs,
                                    const stride_iterator& rhs)
        -> difference_type
    {
        using D = difference_type;

        const auto dist = std::distance(rhs.base_, lhs.base_);
        const auto sign = D{dist > D{}} - D{dist < D{}};
        const auto rounded = (dist + sign * (N - 1)) / N;

        return (rounded - (rhs.offset_ - lhs.offset_));
    }
    friend constexpr bool operator==(const stride_iterator& lhs,
                                     const stride_iterator& rhs)
    {
        return std::distance(lhs, rhs) == 0;
    }
    friend constexpr bool operator!=(const stride_iterator& lhs,
                                     const stride_iterator& rhs)
    {
        return !(lhs == rhs);
    }
};

constexpr auto interp_size(std::size_t point_size, std::size_t stride) noexcept
    -> std::size_t
{
    if (point_size == 0) {
        return 0;
    }

    return 1 + (point_size - 1) * stride;
}

template <class T, class Interp>
struct storage_wrapper : T {
    using interp_type = Interp;

    using T::T;

    template <class InputIt, bool Enable = (Interp::stride > 1),
              class = std::enable_if_t<Enable>>
    constexpr storage_wrapper(InputIt first, InputIt last)
    {
        this->resize(interp_size(std::distance(first, last), Interp::stride));
        std::copy(first, last,
                  stride_iterator<typename T::iterator, Interp::stride>{
                      this->begin()});
    }
};

template <class T, std::size_t N, class Interp>
struct storage_wrapper<std::array<T, N>, Interp>
    : std::array<T, interp_size(N, Interp::stride)> {
    using interp_type = Interp;
    using storage_type = std::array<T, interp_size(N, Interp::stride)>;

    using size_type = typename storage_type::size_type;
    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

    constexpr storage_wrapper() : storage_type{} {}

    template <class InputIt>
    constexpr storage_wrapper(InputIt first, InputIt last)
    {
        size_ = interp_size(std::distance(first, last), Interp::stride);
        if (size() > capacity()) {
            throw std::runtime_error{"std::distance(first, last) > capacity()"};
        }
        std::copy(first, last,
                  stride_iterator<iterator, Interp::stride>{this->begin()});
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return size() == 0;
    }

    constexpr auto size() const noexcept -> size_type { return size_; }
    constexpr auto capacity() const noexcept -> size_type
    {
        return interp_size(N, Interp::stride);
    }

    constexpr auto cend() const noexcept -> const_iterator { return end(); }

  private:
    constexpr auto end() noexcept -> iterator { return this->begin() + size_; }
    constexpr auto end() const noexcept -> const_iterator
    {
        return this->begin() + size_;
    }

    size_type size_{};
};

}  // namespace detail

namespace interp {
struct linear {
    static constexpr std::size_t stride = 1;
};

struct cubic {
    static constexpr std::size_t stride = 3;
};
}  // namespace interp

template <class Storage, class Interp = interp::linear>
class spline : detail::storage_wrapper<Storage, Interp> {
    using base_type = detail::storage_wrapper<Storage, Interp>;

  public:
    using storage_type = Storage;
    using value_type = typename Storage::value_type;
    using size_type = typename storage_type::size_type;

    using iterator = detail::stride_iterator<typename storage_type::iterator,
                                             Interp::stride>;
    using const_iterator =
        detail::stride_iterator<typename storage_type::const_iterator,
                                Interp::stride>;

    spline() = default;

    template <class InputIt>
    constexpr spline(InputIt first, InputIt last) : base_type{first, last}
    {}

    using base_type::empty;

    constexpr auto size() const noexcept -> size_type
    {
        return std::distance(cbegin(), cend());
    }

    constexpr auto cbegin() const noexcept -> const_iterator
    {
        return {base_type::cbegin()};
    }
    constexpr auto cend() const noexcept -> const_iterator
    {
        return {base_type::cend()};
    }
};

}  // namespace spline
