////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TypeTraitsTest.hpp"

import std;
import sequoia.core.meta;

namespace sequoia::testing
{
  namespace
  {
    struct foo { int x{}; };

    struct throwing_move_construction
    {
      throwing_move_construction() = default;
      throwing_move_construction(const throwing_move_construction&) = default;
      throwing_move_construction(throwing_move_construction&&) noexcept(false) {}
      throwing_move_construction& operator=(const throwing_move_construction&) = default;
      throwing_move_construction& operator=(throwing_move_construction&&) noexcept { return *this; }
    };

    struct throwing_copy_assignment
    {
      throwing_copy_assignment() = default;
      throwing_copy_assignment(const throwing_copy_assignment&) = default;
      throwing_copy_assignment(throwing_copy_assignment&&) noexcept = default;
      throwing_copy_assignment& operator=(const throwing_copy_assignment&) noexcept(false) { return *this; }
      throwing_copy_assignment& operator=(throwing_copy_assignment&&) noexcept = default;
    };

    struct move_only
    {
      int value{};

      move_only() = default;

      move_only(move_only&&) noexcept = default;

      move_only& operator=(move_only&&) noexcept = default;
    };

    struct assign_only
    {
      int value{};

      assign_only() = default;

      assign_only(const assign_only&) = delete;

      assign_only& operator=(const assign_only&) = default;
    };

    struct non_assignable
    {
      const int value{};
    };

    /** Whether the trait agrees with the standard library's own `noexcept` on `std::exchange`. */
    template<class T, class U>
    constexpr bool agrees_with_library_v{
      is_nothrow_exchangeable_v<T, U> == noexcept(std::exchange(std::declval<T&>(), std::declval<U>()))
    };
  }
  
  [[nodiscard]]
  std::filesystem::path type_traits_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void type_traits_test::run_tests()
  {
    test_resolve_to_copy();
    test_is_const_pointer();
    test_is_const_reference();
    test_is_tuple();
    test_is_initializable();
    test_is_nothrow_exchangeable();
    test_has_allocator_type();
    test_is_compatible();
    test_are_same();
    test_value_type_of();
    test_is_deep_copy_constructible();
    test_is_deep_copy_assignable();
  }

  void type_traits_test::test_resolve_to_copy()
  {
    {
      using d = resolve_to_copy<int>;

      STATIC_CHECK( std::is_same_v<std::false_type, d::type>);
      STATIC_CHECK( std::is_same_v<std::false_type, resolve_to_copy_t<int>>);
      STATIC_CHECK(!resolve_to_copy_v<int>);
    }

    {
      using d = resolve_to_copy<int, int>;

      STATIC_CHECK(std::is_same_v<std::true_type, d::type>);
      STATIC_CHECK(std::is_same_v<std::true_type, resolve_to_copy_t<int, int>>);
      STATIC_CHECK(resolve_to_copy_v<int, int>);
    }

    {
      using d = resolve_to_copy<int&, int>;

      STATIC_CHECK(std::is_same_v<std::true_type, d::type>);
      STATIC_CHECK(std::is_same_v<std::true_type, resolve_to_copy_t<int&, int>>);
      STATIC_CHECK(resolve_to_copy_v<int&, int>);
    }

    {
      using d = resolve_to_copy<int, int&>;

      STATIC_CHECK(std::is_same_v<std::true_type, d::type>);
      STATIC_CHECK(std::is_same_v<std::true_type, resolve_to_copy_t<int, int&>>);
      STATIC_CHECK(resolve_to_copy_v<int, int&>);
    }

    {
      using d = resolve_to_copy<const int&, volatile int&>;

      STATIC_CHECK(std::is_same_v<std::true_type, d::type>);
      STATIC_CHECK(std::is_same_v<std::true_type, resolve_to_copy_t<const int&, volatile int&>>);
      STATIC_CHECK(resolve_to_copy_v<const int&, volatile int&>);
    }

    {
      using d = resolve_to_copy<int, double>;

      STATIC_CHECK( std::is_same_v<std::false_type, d::type>);
      STATIC_CHECK( std::is_same_v<std::false_type, resolve_to_copy_t<int, double>>);
      STATIC_CHECK(!resolve_to_copy_v<int, double>);
    }

    {
      using d = resolve_to_copy<int, int, int>;

      STATIC_CHECK( std::is_same_v<std::false_type, d::type>);
      STATIC_CHECK( std::is_same_v<std::false_type, resolve_to_copy_t<int, int, double>>);
      STATIC_CHECK(!resolve_to_copy_v<int, int, int>);
    }
  }

  void type_traits_test::test_is_const_pointer()
  {
    STATIC_CHECK( std::is_same_v<std::true_type, is_const_pointer_t<const int*>>);
    STATIC_CHECK( is_const_pointer_v<const int*>);
    STATIC_CHECK( std::is_same_v<std::false_type, is_const_pointer_t<int*>>);
    STATIC_CHECK(!is_const_pointer_v<int*>);
    STATIC_CHECK( std::is_same_v<std::false_type, is_const_pointer_t<int* const>>);
    STATIC_CHECK(!is_const_pointer_v<int* const>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_const_pointer_t<const int* const>>);
    STATIC_CHECK( is_const_pointer_v<const int* const>);
  }

  void type_traits_test::test_is_const_reference()
  {
    STATIC_CHECK( std::is_same_v<std::true_type, is_const_reference_t<const int&>>);
    STATIC_CHECK( is_const_reference_v<const int&>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_const_reference_t<const volatile int&>>);
    STATIC_CHECK( is_const_reference_v<const volatile int&>);
    STATIC_CHECK( std::is_same_v<std::false_type, is_const_reference_t<int&>>);
    STATIC_CHECK(!is_const_reference_v<int&>);
  }

  void type_traits_test::test_is_initializable()
  {
    STATIC_CHECK( std::is_same_v<std::false_type, is_initializable_t<foo, std::vector<int>>>);
    STATIC_CHECK(!is_initializable_v<foo, std::vector<int>>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_initializable_t<foo, int>>);
    STATIC_CHECK( is_initializable_v<foo, int>);
  }

  void type_traits_test::test_is_nothrow_exchangeable()
  {
    STATIC_CHECK( is_nothrow_exchangeable_v<int, int>);
    STATIC_CHECK( std::is_same_v<is_nothrow_exchangeable_t<int, const int&>, std::true_type>);
    STATIC_CHECK(!is_nothrow_exchangeable_v<throwing_move_construction, throwing_move_construction>);
    STATIC_CHECK( is_nothrow_exchangeable_v<throwing_copy_assignment, throwing_copy_assignment>);
    STATIC_CHECK(!is_nothrow_exchangeable_v<throwing_copy_assignment, const throwing_copy_assignment&>);

    STATIC_CHECK(   agrees_with_library_v<int, int>
                 && agrees_with_library_v<int, const int&>
                 && agrees_with_library_v<throwing_move_construction, throwing_move_construction>
                 && agrees_with_library_v<throwing_copy_assignment, throwing_copy_assignment>
                 && agrees_with_library_v<throwing_copy_assignment, const throwing_copy_assignment&>);
  }

  void type_traits_test::test_is_tuple()
  {
    STATIC_CHECK(!is_tuple_v<int>);
    STATIC_CHECK( std::is_same_v<std::false_type, is_tuple_t<int>>);
    STATIC_CHECK( is_tuple_v<std::tuple<>>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_tuple_t<std::tuple<>>>);
    STATIC_CHECK( is_tuple_v<std::tuple<int>>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_tuple_t<std::tuple<int>>>);
  }

  void type_traits_test::test_has_allocator_type()
  {
    STATIC_CHECK( has_allocator_type_v<std::vector<double>>);
    STATIC_CHECK( std::is_same_v<std::true_type, has_allocator_type_t<std::vector<double>>>);
    STATIC_CHECK(!has_allocator_type_v<double>);
    STATIC_CHECK( std::is_same_v<std::false_type, has_allocator_type_t<double>>);
  }

  void type_traits_test::test_is_compatible()
  {
    STATIC_CHECK( is_compatible_v<double, float>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_compatible_t<double, float>>);
    STATIC_CHECK( is_compatible_v<double, double>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_compatible_t<double, double>>);
    STATIC_CHECK( is_compatible_v<double, int>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_compatible_t<double, int>>);
    STATIC_CHECK(!is_compatible_v<float, double>);
    STATIC_CHECK( std::is_same_v<std::false_type, is_compatible_t<float, double>>);
    STATIC_CHECK( is_compatible_v<float, float>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_compatible_t<float, float>>);
    STATIC_CHECK( is_compatible_v<float, int>);
    STATIC_CHECK( std::is_same_v<std::true_type, is_compatible_t<float, int>>);
    STATIC_CHECK( has_allocator_type_v<std::vector<double>>);
    STATIC_CHECK( std::is_same_v<std::true_type, has_allocator_type_t<std::vector<double>>>);
    STATIC_CHECK(!has_allocator_type_v<double>);
    STATIC_CHECK( std::is_same_v<std::false_type, has_allocator_type_t<double>>);
  }

  void type_traits_test::test_are_same()
  {
    STATIC_CHECK(are_same_v<int>);
    STATIC_CHECK(std::same_as<are_same_t<int>, std::true_type>);

    STATIC_CHECK(are_same_v<float>);
    STATIC_CHECK(are_same_v<int, int>);
    STATIC_CHECK(std::same_as<are_same_t<int, int>, std::true_type>);

    STATIC_CHECK( are_same_v<float, float>);
    STATIC_CHECK(!are_same_v<int, float>);
    STATIC_CHECK( std::same_as<are_same_t<int, float>, std::false_type>);
  }

  void type_traits_test::test_value_type_of()
  {
    struct foo{ using value_type = int; };

    STATIC_CHECK(std::is_same_v<value_type_of_t<foo>, int>);
  }

  void type_traits_test::test_is_deep_copy_constructible()
  {
    STATIC_CHECK( is_deep_copy_constructible_v<int>);
    STATIC_CHECK( std::is_same_v<is_deep_copy_constructible_t<int>, std::true_type>);
    STATIC_CHECK(!is_deep_copy_constructible_v<move_only>);

    // Homogeneous containers
    STATIC_CHECK( std::is_copy_constructible_v<std::vector<move_only>>);
    STATIC_CHECK(!is_deep_copy_constructible_v<std::vector<move_only>>);
    STATIC_CHECK(!is_deep_copy_constructible_v<std::vector<std::vector<move_only>>>);
    STATIC_CHECK( is_deep_copy_constructible_v<std::vector<std::vector<int>>>);

    // Heterogeneous containers
    STATIC_CHECK( std::is_copy_constructible_v<std::tuple<int, std::vector<move_only>>>);
    STATIC_CHECK(!is_deep_copy_constructible_v<std::tuple<int, std::vector<move_only>>>);
    STATIC_CHECK( is_deep_copy_constructible_v<std::pair<int, std::vector<int>>>);
    STATIC_CHECK( std::is_copy_constructible_v<std::variant<int, std::vector<move_only>>>);
    STATIC_CHECK(!is_deep_copy_constructible_v<std::variant<int, std::vector<move_only>>>);

    // Maps
    STATIC_CHECK( is_deep_copy_constructible_v<std::map<int, std::vector<int>>>);
    STATIC_CHECK(!is_deep_copy_constructible_v<std::map<int, std::vector<move_only>>>);
  }

  void type_traits_test::test_is_deep_copy_assignable()
  {
    STATIC_CHECK( is_deep_copy_assignable_v<int>);
    STATIC_CHECK( std::is_same_v<is_deep_copy_assignable_t<int>, std::true_type>);
    STATIC_CHECK(!is_deep_copy_assignable_v<move_only>);
    STATIC_CHECK( is_deep_copy_assignable_v<assign_only>);

    // Homogeneous containers require elements that are both copy constructible and copy assignable
    STATIC_CHECK( std::is_copy_assignable_v<std::vector<non_assignable>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::vector<non_assignable>>);
    STATIC_CHECK( std::is_copy_assignable_v<std::vector<assign_only>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::vector<assign_only>>);
    STATIC_CHECK( std::is_copy_assignable_v<std::array<assign_only, 2>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::array<assign_only, 2>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::vector<std::vector<assign_only>>>);
    STATIC_CHECK( is_deep_copy_assignable_v<std::vector<std::vector<int>>>);

    // Heterogeneous containers require elements that are copy assignable
    STATIC_CHECK( is_deep_copy_assignable_v<std::tuple<int, assign_only>>);
    STATIC_CHECK( std::is_copy_assignable_v<std::tuple<int, std::vector<assign_only>>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::tuple<int, std::vector<assign_only>>>);
    STATIC_CHECK( std::is_copy_assignable_v<std::variant<int, std::vector<assign_only>>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::variant<int, std::vector<assign_only>>>);

    // Maps require keys and mapped values that are copy assignable, disregarding the key's const
    STATIC_CHECK( is_deep_copy_assignable_v<std::map<int, int>>);
    STATIC_CHECK( is_deep_copy_assignable_v<std::map<int, std::vector<int>>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::map<int, non_assignable>>);
    STATIC_CHECK( std::is_copy_assignable_v<std::map<int, std::vector<assign_only>>>);
    STATIC_CHECK(!is_deep_copy_assignable_v<std::map<int, std::vector<assign_only>>>);
    STATIC_CHECK( is_deep_copy_assignable_v<std::set<int>>);
  }
}
