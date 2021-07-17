#include <array>
#include <complex>
#include <iterator>
#include <spline/spline.hpp>
#include <tuple>
#include <type_traits>
#include <ut.hpp>
#include <vector>

auto main() -> int
{
    using namespace boost::ut::literals;
    using namespace boost::ut;
    using spline::spline;

    using Point = std::complex<double>;
    using StorageContainers =
        std::tuple<std::array<Point, 3>, std::vector<Point>>;

    test("Default Constructor") = []<class Storage>() {
        expect(nothrow([] { spline<Storage>{}; }));
    } | StorageContainers{};

    test("Has member types") = []<class Storage>() {
        static_assert(
            std::is_same<Storage,
                         typename spline<Storage>::storage_type>::value);
        static_assert(
            std::is_same<Point, typename spline<Storage>::value_type>::value);
    } | StorageContainers{};

    test("Empty on default construction") = []<class Storage>() {
        const auto s = spline<Storage>{};

        expect(s.empty());
        expect(0 == s.size());
        expect(s.cbegin() == s.cend());
    } | StorageContainers{};

    test("Copies points with iterator constructor") =
        []<class Storage>() {
            constexpr auto data = std::array{Point{0.0, 0.0}, Point{1.0, 1.0},
                                             Point{2.0, 0.0}, Point{3.0, -1.0}};

            const auto s = spline<Storage>{data.cbegin(), data.cend()};

            expect(not s.empty());
            expect(4 == s.size()) << s.size();
            expect(std::equal(s.cbegin(), s.cend(), data.cbegin()));
        } |
        std::tuple<std::array<Point, 4>, std::array<Point, 5>,
                   std::vector<Point>>{};

    test("Array storage is constexpr") = [] {
        constexpr auto s1 = spline<std::array<double, 4>>{};
        static_assert(s1.empty());

        constexpr auto data = std::array{0.0, 2.0, 2.0, 3.0};

        constexpr auto s2 =
            spline<std::array<double, 4>>{data.cbegin(), data.cend()};
        static_assert(not s2.empty());
    };

    test("stride_iterator equality, stride 1") = [] {
        constexpr auto data = std::array{0.0, 2.0, 2.0, 3.0};

        using S = spline::detail::stride_iterator<decltype(data.cbegin()), 1>;

        expect(++S{data.cbegin()} == S{data.cbegin(), 1});
        expect(++S{data.cbegin()} == (S{data.cbegin()} + 1));

        expect(S{data.cend()} == S{data.cbegin(), 4});
        expect(S{data.cbegin(), 4} == S{data.cend()});

        expect(S{data.cend(), -1} == S{data.cbegin(), 3});
        expect(S{data.cbegin(), 3} == S{data.cend(), -1});
    };

    test("stride_iterator equality, stride 3") = [] {
        constexpr auto data = std::array{0.0, 2.0, 2.0, 3.0};

        using S = spline::detail::stride_iterator<decltype(data.cbegin()), 3>;

        expect(++S{data.cbegin()} == S{data.cbegin(), 1});
        expect(++S{data.cbegin()} == (S{data.cbegin()} + 1));

        expect(S{data.cbegin(), 2} == S{data.cend()});
        expect(S{data.cend()} == S{data.cbegin(), 2});

        expect(S{data.cend(), -1} == S{data.cbegin()} + 1);
        expect(S{data.cbegin()} + 1 == S{data.cend(), -1});
    };

    test("spline stride with linear interp") = [] {
        constexpr auto data = std::array{0.0, 2.0, 2.0, 3.0};

        auto s = spline<std::array<double, 4>, spline::interp::linear>{
            data.cbegin(), data.cend()};

        expect(data.size() == s.size());
        expect(std::equal(s.cbegin(), s.cend(), data.cbegin()));

        expect(data.size() == static_cast<std::size_t>(std::distance(
                                  s.cbegin().base(), s.cend().base())));
        expect(std::equal(s.cbegin().base(), s.cend().base(), data.cbegin()));
    };

    test("spline stride with cubic interp") = []<class Storage>() {
        constexpr auto data = std::array{1.0, 3.0};
        constexpr auto interp_data =
            std::array{1.0, 0.0, 0.0, 3.0};  // not yet set

        expect(4 == spline::detail::interp_size(2, 3));

        auto s =
            spline<Storage, spline::interp::cubic>{data.cbegin(), data.cend()};

        expect(data.size() == s.size()) << std::distance(s.cbegin(), s.cend());
        expect(std::equal(s.cbegin(), s.cend(), data.cbegin()));

        expect(interp_data.size() == static_cast<std::size_t>(std::distance(
                                         s.cbegin().base(), s.cend().base())))
            << std::distance(s.cbegin().base(), s.cend().base());

        expect(*s.cbegin().base() == interp_data[0]);
        expect(*std::prev(s.cend().base()) == interp_data[3]);
    } | StorageContainers{};
}
