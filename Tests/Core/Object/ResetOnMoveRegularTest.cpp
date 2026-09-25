////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ResetOnMoveRegularTest.hpp"

#include <array>
#include <compare>

namespace sequoia::testing
{
  using namespace object;

  namespace
  {
    /** Resets to a value no default construction of `int` would produce. */
    using handle = reset_on_move<int, -1>;

    [[nodiscard]]
    int first() { return 1; }

    using function_handle = reset_on_move<int(*)()>;

    struct throwing_move
    {
      int value{};

      constexpr throwing_move() = default;

      constexpr throwing_move(const throwing_move&) = default;

      constexpr throwing_move(throwing_move&& other) noexcept(false)
        : value{other.value}
      {}

      constexpr throwing_move& operator=(const throwing_move&) = default;

      constexpr throwing_move& operator=(throwing_move&& other) noexcept(false)
      {
        value = other.value;
        return *this;
      }

      [[nodiscard]]
      friend constexpr bool operator==(const throwing_move&, const throwing_move&) noexcept = default;
    };

    using throwing_handle = reset_on_move<throwing_move, throwing_move{}>;

    struct throwing_equality
    {
      int value{};

      [[nodiscard]]
      friend constexpr bool operator==(const throwing_equality& lhs, const throwing_equality& rhs) noexcept(false)
      {
        return lhs.value == rhs.value;
      }

      [[nodiscard]]
      friend constexpr auto operator<=>(const throwing_equality& lhs, const throwing_equality& rhs) noexcept
      {
        return lhs.value <=> rhs.value;
      }
    };

    using throwing_equality_handle = reset_on_move<throwing_equality, throwing_equality{}>;

    struct throwing_ordering
    {
      int value{};

      [[nodiscard]]
      friend constexpr bool operator==(const throwing_ordering& lhs, const throwing_ordering& rhs) noexcept
      {
        return lhs.value == rhs.value;
      }

      [[nodiscard]]
      friend constexpr auto operator<=>(const throwing_ordering& lhs, const throwing_ordering& rhs) noexcept(false)
      {
        return lhs.value <=> rhs.value;
      }
    };

    using throwing_ordering_handle = reset_on_move<throwing_ordering, throwing_ordering{}>;

    struct move_only
    {
      int value{};

      constexpr move_only() = default;

      constexpr move_only(move_only&&) noexcept = default;

      constexpr move_only& operator=(move_only&&) noexcept = default;

      [[nodiscard]]
      friend constexpr bool operator==(const move_only&, const move_only&) noexcept = default;
    };

    template<class T>
    concept holdable_on_reset = requires { typename reset_on_move<T>; };

    template<class T>
    concept nothrow_equality_comparable = requires(const T& t) { { t == t } noexcept; };

    template<class T>
    concept nothrow_three_way_comparable = requires(const T& t) { { t <=> t } noexcept; };
  }

  [[nodiscard]]
  std::filesystem::path reset_on_move_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void reset_on_move_regular_test::run_tests()
  {
    test_exception_specifications();
    test_constant_evaluation();
    test_semantics();
    test_self_move_assignment();
  }

  void reset_on_move_regular_test::test_exception_specifications()
  {
    STATIC_CHECK(holdable_on_reset<int>);
    STATIC_CHECK(!holdable_on_reset<move_only>);

    STATIC_CHECK(std::is_nothrow_move_constructible_v<handle>);
    STATIC_CHECK(std::is_nothrow_move_assignable_v<handle>);
    STATIC_CHECK(!std::is_nothrow_move_constructible_v<throwing_handle>);
    STATIC_CHECK(!std::is_nothrow_move_assignable_v<throwing_handle>);

    STATIC_CHECK(nothrow_equality_comparable<handle>);
    STATIC_CHECK(nothrow_three_way_comparable<handle>);
    STATIC_CHECK(!nothrow_equality_comparable<throwing_equality_handle>);
    STATIC_CHECK(nothrow_three_way_comparable<throwing_equality_handle>);
    STATIC_CHECK(nothrow_equality_comparable<throwing_ordering_handle>);
    STATIC_CHECK(!nothrow_three_way_comparable<throwing_ordering_handle>);
  }

  void reset_on_move_regular_test::test_constant_evaluation()
  {
    constexpr auto values{
      []() {
        handle source{7};
        handle moved{std::move(source)};
        handle assigned{};
        assigned = std::move(moved);
        return std::array{assigned.value(), moved.value(), source.value()};
      }()
    };

    STATIC_CHECK(values == std::array{7, handle::reset_value, handle::reset_value});
  }

  void reset_on_move_regular_test::test_semantics()
  {
    check_semantics("An ordered value",
                    handle{},
                    handle{8},
                    handle::reset_value,
                    8,
                    handle::reset_value,
                    handle::reset_value,
                    std::strong_ordering::less);

    check_semantics("A value with equality but no ordering",
                    function_handle{},
                    function_handle{&first},
                    function_handle::reset_value,
                    &first,
                    function_handle::reset_value,
                    function_handle::reset_value);
  }

  void reset_on_move_regular_test::test_self_move_assignment()
  {
    handle h{8};
    handle& alias{h};
    h = std::move(alias);
    check(equivalence, "Self-move-assignment keeps the value", h, 8);
  }

}
