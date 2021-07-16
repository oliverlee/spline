#pragma once

#include <array>
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

}  // namespace detail

template <class Storage>
class spline : detail::storage_wrapper<Storage> {
    using base_type = detail::storage_wrapper<Storage>;

  public:
    using storage_type = Storage;
    using value_type = typename Storage::value_type;

    spline() = default;

    template <class InputIt>
    constexpr spline(InputIt first, InputIt last) : base_type{first, last}
    {}

    using base_type::empty;
    using base_type::size;

    using base_type::cbegin;
    using base_type::cend;
};

}  // namespace spline
