////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "ConceptsTest.hpp"

#include "sequoia/Core/Meta/Concepts.hpp"

#include <complex>
#include <cstdint>
#include <set>
#include <map>
#include <vector>


namespace sequoia::testing
{
  namespace
  {
    struct bar{};

    struct serializable_thing
    {
      template<class Stream>
      friend Stream& operator<<(Stream& s, const serializable_thing&)
      {
        return s;
      }
    };

    struct non_serializable
    {};

    template<class> struct foo;

    template<>
    struct foo<int> {};

    struct aggregate
    {
      int i;
      double x;
    };

    struct move_only_init
    {
      move_only_init(std::vector<int>&& j) : i{std::move(j)}
      {}

      std::vector<int> i;
    };
  }

  [[nodiscard]]
  std::filesystem::path concepts_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void concepts_test::run_tests()
  {
    test_is_range();
    test_is_allocator();
    test_is_serializable();
    test_deep_equality_comparable();
    test_deep_totally_ordered();
    test_initializable_from();
    test_integer();
  }

  void concepts_test::test_is_range()
  {
    check_static<(faithful_range<std::vector<double>>)>();
    check_static<(!faithful_range<double>)>();
  }

  void concepts_test::test_is_allocator()
  {
    check_static<(!alloc<int>)>();
    check_static<(alloc<std::allocator<int>>)>();
  }

  void concepts_test::test_is_serializable()
  {
    check_static<(serializable_to<int, std::stringstream>)>();
    check_static<(serializable_to<serializable_thing, std::stringstream>)>();
    check_static<(!serializable_to<non_serializable, std::stringstream>)>();
  }

  void concepts_test::test_deep_equality_comparable()
  {
    check_static<(deep_equality_comparable<int>)>();
    check_static<(deep_equality_comparable<std::vector<int>>)>();
    check_static<(deep_equality_comparable<std::array<int, 3>>)>();
    check_static<(deep_equality_comparable<std::map<int, double>>)>();
    check_static<(deep_equality_comparable<std::tuple<int>>)>();
    check_static<(deep_equality_comparable<std::tuple<int, double>>)>();
    check_static<(deep_equality_comparable<std::pair<int, double>>)>();
    check_static<(deep_equality_comparable<std::optional<int>>)>();
    check_static<(deep_equality_comparable<std::variant<int, float>>)>();
    check_static<(deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<int, float>, 2>>>)>();

    check_static<(!deep_equality_comparable<bar>)>();
    check_static<(!deep_equality_comparable<std::vector<bar>>)>();
    check_static<(!deep_equality_comparable<std::array<bar, 3>>)>();
    check_static<(!deep_equality_comparable<std::map<int, bar>>)>();
    check_static<(!deep_equality_comparable<std::tuple<bar>>)>();
    check_static<(!deep_equality_comparable<std::tuple<bar, double>>)>();
    check_static<(!deep_equality_comparable<std::optional<bar>>)>();
    check_static<(!deep_equality_comparable<std::variant<int, bar>>)>();
    check_static<(!deep_equality_comparable<std::tuple<std::vector<bar>, std::array<std::pair<int, float>, 2>>>)>();
    check_static<(!deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<bar, float>, 2>>>)>();
    check_static<(!deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<int, bar>, 2>>>)>();
  }

  void concepts_test::test_deep_totally_ordered()
  {
    check_static<(deep_totally_ordered<int>)>();
    check_static<(deep_totally_ordered<std::vector<int>>)>();
    check_static<(deep_totally_ordered<std::array<int, 3>>)>();
    check_static<(deep_totally_ordered<std::map<int, double>>)>();
    check_static<(deep_totally_ordered<std::tuple<int>>)>();
    check_static<(deep_totally_ordered<std::tuple<int, double>>)>();
    check_static<(deep_totally_ordered<std::pair<int, double>>)>();
    check_static<(deep_totally_ordered<std::optional<int>>)>();
    check_static<(deep_totally_ordered<std::variant<int, float>>)>();
    check_static<(deep_totally_ordered<std::tuple<std::vector<int>, std::array<std::pair<int, float>, 2>>>)>();

  }

  void concepts_test::test_initializable_from()
  {
    check_static<(initializable_from<int, int>)>();
    check_static<(initializable_from<bar>)>();
    check_static<(!initializable_from<bar, int>)>();
    check_static<(initializable_from<aggregate, int, double>)>();
    check_static<(!initializable_from<aggregate, int, double, char>)>();
    check_static<(initializable_from<move_only_init, std::vector<int>>)>();
  }

  void concepts_test::test_integer()
  {
    // The standard's own distinction: these six are integral types but not integer
    // types, and it is the line std::in_range and the std::cmp_* family draw.
    check_static<(!integer<bool>)>();
    check_static<(!integer<char>)>();
    check_static<(!integer<wchar_t>)>();
    check_static<(!integer<char8_t>)>();
    check_static<(!integer<char16_t>)>();
    check_static<(!integer<char32_t>)>();

    // signed char and unsigned char fall the other side, which is what keeps
    // std::int8_t - being signed char - admitted.
    check_static<(integer<signed char>)>();
    check_static<(integer<unsigned char>)>();
    check_static<(integer<std::int8_t>)>();
    check_static<(integer<std::uint8_t>)>();

    check_static<(integer<short>)>();
    check_static<(integer<int>)>();
    check_static<(integer<unsigned>)>();
    check_static<(integer<long long>)>();

    // std::integral sees through cv-qualification where std::same_as does not, so
    // the exclusions are written against the unqualified type.
    check_static<(!integer<const bool>)>();
    check_static<(!integer<volatile char>)>();
    check_static<(integer<const int>)>();

    check_static<(!integer<float>)>();
    check_static<(!integer<double>)>();
    check_static<(!integer<bar>)>();
    check_static<(!integer<int*>)>();
    check_static<(!integer<int&>)>();
  }
}
