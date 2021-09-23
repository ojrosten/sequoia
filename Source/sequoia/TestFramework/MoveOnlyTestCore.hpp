////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extension for testing classes exhibiting move-only semantics.

    This class is designed to be plugged into the checker class template, in order to extend
    its functionality. See MoveOnlyCheckers.hpp for further information.
*/

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/MoveOnlyCheckers.hpp"

#include "sequoia/Core/Meta/Utilities.hpp"

namespace sequoia::testing
{
  /*! \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide allocation checks for move-only types,
      see \ref move_only_definition "here".

      \anchor move_only_extender_primary
   */
  template<test_mode Mode>
  class move_only_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    explicit move_only_extender(test_logger<Mode>& logger) : m_pLogger{&logger} {}

    move_only_extender(const move_only_extender&) = delete;
    move_only_extender(move_only_extender&&)      = delete;

    move_only_extender& operator=(const move_only_extender&) = delete;
    move_only_extender& operator=(move_only_extender&&)      = delete;

    /// Preconditions: x!=y; x==xClone, y==yClone
    template<moveonly T>
    void check_semantics(std::string_view description, T&& x, T&& y, const T& xClone, const T& yClone)
    {
      testing::check_semantics(append_lines(description, emphasise("Move-only Semantics")), logger(), std::move(x), std::move(y), xClone, yClone);
    }

    template
    <
      invocable<> xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      invocable_r<T> yMaker
    >
    void check_semantics(std::string_view description, xMaker xFn, yMaker yFn)
    {
      check_semantics(description, xFn(), yFn(), xFn(), yFn());
    }

     /// Preconditions: x!=y, with values consistent with order; x==xClone, y==yClone
    template<moveonly T>
      requires orderable<T>
    void check_semantics(std::string_view description, T&& x, T&& y, const T& xClone, const T& yClone, std::weak_ordering order)
    {
      testing::check_semantics(append_lines(description, emphasise("Move-only Semantics")), logger(), std::move(x), std::move(y), xClone, yClone, order);
    }

    template
    <
      invocable<> xMaker,
      moveonly T=std::invoke_result_t<xMaker>,
      invocable_r<T> yMaker
    >
      requires orderable<T>
    void check_semantics(std::string_view description, xMaker xFn, yMaker yFn, std::weak_ordering order)
    {
      check_semantics(description, xFn(), yFn(), xFn(), yFn(), order);
    }
  protected:
    ~move_only_extender() = default;
  private:
    [[nodiscard]]
    test_logger<Mode>& logger() noexcept { return *m_pLogger; }

    test_logger<Mode>* m_pLogger;
  };

  template<test_mode mode>
  using move_only_checker = checker<mode, move_only_extender<mode>>;

  template<test_mode mode>
  using canonical_move_only_test = basic_test<move_only_checker<mode>>;

  /*! \anchor move_only_test_alias */
  using move_only_test                = canonical_move_only_test<test_mode::standard>;
  using move_only_false_negative_test = canonical_move_only_test<test_mode::false_negative>;
  using move_only_false_positive_test = canonical_move_only_test<test_mode::false_positive>;
}
