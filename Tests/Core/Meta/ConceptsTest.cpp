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
#include <sstream>
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
      // An ordinary streaming operator, not a template: the concept is checked
      // against one stream type, so genericity was never part of the claim.
      friend std::ostream& operator<<(std::ostream& s, const serializable_thing&)
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
    STATIC_CHECK(faithful_range<std::vector<double>>);
    STATIC_CHECK(!faithful_range<double>);
  }

  void concepts_test::test_is_allocator()
  {
    STATIC_CHECK(!alloc<int>);
    STATIC_CHECK(alloc<std::allocator<int>>);
  }

  void concepts_test::test_is_serializable()
  {
    STATIC_CHECK(serializable_to<int, std::stringstream>);
    STATIC_CHECK(serializable_to<serializable_thing, std::stringstream>);
    STATIC_CHECK(!serializable_to<non_serializable, std::stringstream>);
  }

  void concepts_test::test_deep_equality_comparable()
  {
    STATIC_CHECK(deep_equality_comparable<int>);
    STATIC_CHECK(deep_equality_comparable<std::vector<int>>);
    STATIC_CHECK(deep_equality_comparable<std::array<int, 3>>);
    STATIC_CHECK(deep_equality_comparable<std::map<int, double>>);
    STATIC_CHECK(deep_equality_comparable<std::tuple<int>>);
    STATIC_CHECK(deep_equality_comparable<std::tuple<int, double>>);
    STATIC_CHECK(deep_equality_comparable<std::pair<int, double>>);
    STATIC_CHECK(deep_equality_comparable<std::optional<int>>);
    STATIC_CHECK(deep_equality_comparable<std::variant<int, float>>);
    STATIC_CHECK(deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<int, float>, 2>>>);

    STATIC_CHECK(!deep_equality_comparable<bar>);
    STATIC_CHECK(!deep_equality_comparable<std::vector<bar>>);
    STATIC_CHECK(!deep_equality_comparable<std::array<bar, 3>>);
    STATIC_CHECK(!deep_equality_comparable<std::map<int, bar>>);
    STATIC_CHECK(!deep_equality_comparable<std::tuple<bar>>);
    STATIC_CHECK(!deep_equality_comparable<std::tuple<bar, double>>);
    STATIC_CHECK(!deep_equality_comparable<std::optional<bar>>);
    STATIC_CHECK(!deep_equality_comparable<std::variant<int, bar>>);
    STATIC_CHECK(!deep_equality_comparable<std::tuple<std::vector<bar>, std::array<std::pair<int, float>, 2>>>);
    STATIC_CHECK(!deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<bar, float>, 2>>>);
    STATIC_CHECK(!deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<int, bar>, 2>>>);
  }

  void concepts_test::test_deep_totally_ordered()
  {
    STATIC_CHECK(deep_totally_ordered<int>);
    STATIC_CHECK(deep_totally_ordered<std::vector<int>>);
    STATIC_CHECK(deep_totally_ordered<std::array<int, 3>>);
    STATIC_CHECK(deep_totally_ordered<std::map<int, double>>);
    STATIC_CHECK(deep_totally_ordered<std::tuple<int>>);
    STATIC_CHECK(deep_totally_ordered<std::tuple<int, double>>);
    STATIC_CHECK(deep_totally_ordered<std::pair<int, double>>);
    STATIC_CHECK(deep_totally_ordered<std::optional<int>>);
    STATIC_CHECK(deep_totally_ordered<std::variant<int, float>>);
    STATIC_CHECK(deep_totally_ordered<std::tuple<std::vector<int>, std::array<std::pair<int, float>, 2>>>);

  }

  void concepts_test::test_initializable_from()
  {
    STATIC_CHECK(initializable_from<int, int>);
    STATIC_CHECK(initializable_from<bar>);
    STATIC_CHECK(!initializable_from<bar, int>);
    STATIC_CHECK(initializable_from<aggregate, int, double>);
    STATIC_CHECK(!initializable_from<aggregate, int, double, char>);
    STATIC_CHECK(initializable_from<move_only_init, std::vector<int>>);
  }

  void concepts_test::test_integer()
  {
    // The standard's own distinction: these six are integral types but not integer
    // types, and it is the line std::in_range and the std::cmp_* family draw.
    STATIC_CHECK(!integer<bool>);
    STATIC_CHECK(!integer<char>);
    STATIC_CHECK(!integer<wchar_t>);
    STATIC_CHECK(!integer<char8_t>);
    STATIC_CHECK(!integer<char16_t>);
    STATIC_CHECK(!integer<char32_t>);

    // signed char and unsigned char fall the other side, which is what keeps
    // std::int8_t - being signed char - admitted.
    STATIC_CHECK(integer<signed char>);
    STATIC_CHECK(integer<unsigned char>);
    STATIC_CHECK(integer<std::int8_t>);
    STATIC_CHECK(integer<std::uint8_t>);

    STATIC_CHECK(integer<short>);
    STATIC_CHECK(integer<int>);
    STATIC_CHECK(integer<unsigned>);
    STATIC_CHECK(integer<long long>);

    // std::integral sees through cv-qualification where std::same_as does not, so
    // the exclusions are written against the unqualified type.
    STATIC_CHECK(!integer<const bool>);
    STATIC_CHECK(!integer<volatile char>);
    STATIC_CHECK(integer<const int>);

    STATIC_CHECK(!integer<float>);
    STATIC_CHECK(!integer<double>);
    STATIC_CHECK(!integer<bar>);
    STATIC_CHECK(!integer<int*>);
    STATIC_CHECK(!integer<int&>);
  }
}
