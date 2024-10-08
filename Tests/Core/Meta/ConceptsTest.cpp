////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ConceptsTest.hpp"

#include "sequoia/Core/Meta/Concepts.hpp"

#include "sequoia/TestFramework/AllocationTestUtilities.hpp"

#include <complex>
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
    test_initializable_from();
  }

  void concepts_test::test_is_range()
  {
    check("", []() {
        static_assert(faithful_range<std::vector<double>>);
        static_assert(!faithful_range<double>);
        return true;
      }()
    );
  }

  void concepts_test::test_is_allocator()
  {
    check("", []() {
        static_assert(!alloc<int>);
        static_assert(alloc<std::allocator<int>>);
        return true;
      }()
    );
  }

  void concepts_test::test_is_serializable()
  {

    check("", []() {
        static_assert(serializable_to<int, std::stringstream>);
        static_assert(serializable_to<serializable_thing, std::stringstream>);
        static_assert(!serializable_to<non_serializable, std::stringstream>);
        return true;
      }()
    );
  }

  void concepts_test::test_deep_equality_comparable()
  {
    check("", []() {
        static_assert(deep_equality_comparable<int>);
        static_assert(deep_equality_comparable<std::vector<int>>);
        static_assert(deep_equality_comparable<std::array<int, 3>>);
        static_assert(deep_equality_comparable<std::map<int, double>>);
        static_assert(deep_equality_comparable<std::tuple<int>>);
        static_assert(deep_equality_comparable<std::tuple<int, double>>);
        static_assert(deep_equality_comparable<std::pair<int, double>>);
        static_assert(deep_equality_comparable<std::optional<int>>);
        static_assert(deep_equality_comparable<std::variant<int, float>>);
        static_assert(deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<int, float>, 2>>>);

        static_assert(!deep_equality_comparable<bar>);
        static_assert(!deep_equality_comparable<std::vector<bar>>);
        static_assert(!deep_equality_comparable<std::array<bar, 3>>);
        static_assert(!deep_equality_comparable<std::map<int, bar>>);
        static_assert(!deep_equality_comparable<std::tuple<bar>>);
        static_assert(!deep_equality_comparable<std::tuple<bar, double>>);
        static_assert(!deep_equality_comparable<std::optional<bar>>);
        static_assert(!deep_equality_comparable<std::variant<int, bar>>);
        static_assert(!deep_equality_comparable<std::tuple<std::vector<bar>, std::array<std::pair<int, float>, 2>>>);
        static_assert(!deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<bar, float>, 2>>>);
        static_assert(!deep_equality_comparable<std::tuple<std::vector<int>, std::array<std::pair<int, bar>, 2>>>);

        return true;
      }()
    );
  }

  void concepts_test::test_initializable_from()
  {
    check("", []() {
        static_assert(initializable_from<int, int>);
        static_assert(initializable_from<bar>);
        static_assert(!initializable_from<bar, int>);
        static_assert(initializable_from<aggregate, int, double>);
        static_assert(!initializable_from<aggregate, int, double, char>);
        static_assert(initializable_from<move_only_init, std::vector<int>>);

        return true;
      }()
    );
  }
}
