#pragma once

#include <array>
#include <cmath>
#include <iterator>
#include <stdexcept>

/// @file
/// @brief A spline object

namespace spline {

namespace detail {

template <class T>
struct storage_wrapper : T {
    using T::T;
};

template <class T, std::size_t N>
struct storage_wrapper<std::array<T, N>> : std::array<T, N> {
    using storage_type = std::array<T, N>;

    using size_type = typename storage_type::size_type;
    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

    constexpr storage_wrapper() : storage_type{} {}

    template <class InputIt>
    constexpr storage_wrapper(InputIt first, InputIt last)
    {
        while (first != last) {
            if ((size_ + 1) > N) {
                throw std::runtime_error{"std::distance(first, last) > N"};
            }

            *end() = *first++;
            ++size_;
        }
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return size() == 0;
    }

    constexpr auto size() const noexcept -> size_type { return size_; }

    constexpr auto cend() const noexcept -> const_iterator { return end(); }

  private:
    constexpr auto end() noexcept -> iterator { return this->begin() + size_; }
    constexpr auto end() const noexcept -> const_iterator
    {
        return this->begin() + size_;
    }

    size_type size_{};
};

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
        -> decltype(rhs.base() - lhs.base())
    {
        // TODO
        throw std::runtime_error{"operator-"};
        return rhs.base() - lhs.base();
    }
    friend constexpr bool operator==(const stride_iterator& lhs,
                                     const stride_iterator& rhs)
    {
        using D = difference_type;

        const auto dist = std::distance(lhs.base_, rhs.base_);
        const auto sign = D{dist > D{}} - D{dist < D{}};
        const auto rounded = (dist + sign * (N - 1)) / N;

        return (rounded - (lhs.offset_ - rhs.offset_)) == 0;
    }
    friend constexpr bool operator!=(const stride_iterator& lhs,
                                     const stride_iterator& rhs)
    {
        return !(lhs == rhs);
    }
};

}  // namespace detail

template <class Storage>
class spline : detail::storage_wrapper<Storage> {
    using base_type = detail::storage_wrapper<Storage>;

  public:
    using storage_type = Storage;
    using value_type = typename Storage::value_type;

    using iterator =
        detail::stride_iterator<typename storage_type::iterator, 1>;
    using const_iterator =
        detail::stride_iterator<typename storage_type::const_iterator, 1>;

    spline() = default;

    template <class InputIt>
    constexpr spline(InputIt first, InputIt last) : base_type{first, last}
    {}

    using base_type::empty;
    using base_type::size;

    using base_type::cbegin;
    using base_type::cend;

    constexpr auto cbegin() const noexcept -> const_iterator
    {
        return {base_type::cbegin(), 0};
    }
    constexpr auto cend() const noexcept -> const_iterator
    {
        return {base_type::cend(), 0};
    }
};

}  // namespace spline
