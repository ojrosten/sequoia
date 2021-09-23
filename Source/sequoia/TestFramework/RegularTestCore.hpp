////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for checking regular semantics.
 */

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/RegularCheckers.hpp"

namespace sequoia::testing
{
  /*! \brief Extender for testing classes exhibiting regular/orderable semantics.

       This class is designed to be plugged into the
       \ref checker_primary "checker" class template, in order to extend
       its functionality.

       \anchor regular_extender_primary
   */
  template<test_mode Mode>
  class regular_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    explicit regular_extender(test_logger<Mode>& logger) : m_pLogger{&logger} {}

    regular_extender(const regular_extender&) = delete;
    regular_extender(regular_extender&&)      = delete;

    regular_extender& operator=(const regular_extender&) = delete;
    regular_extender& operator=(regular_extender&&)      = delete;

    /// Precondition: x!=y
    template<pseudoregular T>
      requires (!orderable<T>)
    void check_semantics(std::string_view description, const T& x, const T& y)
    {
      testing::check_semantics(append_lines(description, emphasise("Regular Semantics")), logger(), x, y);
    }

    /// Precondition: x!=y, with values consistent with order
    template<pseudoregular T>
      requires orderable<T>
    void check_semantics(std::string_view description, const T& x, const T& y, std::weak_ordering order)
    {
      testing::check_semantics(append_lines(description, emphasise("Regular Semantics")), logger(), x, y, order);
    }

    /// Precondition: x!=y
    template<pseudoregular T, invocable<T&> Mutator>
    void check_semantics(std::string_view description, const T& x, const T& y, Mutator m)
    {
      testing::check_semantics(append_lines(description, emphasise("Regular Semantics")), logger(), x, y, std::move(m));
    }

    /// Precondition: x!=y, with values consistent with order
    template<pseudoregular T, invocable<T&> Mutator>
      requires orderable<T>
    void check_semantics(std::string_view description, const T& x, const T& y, std::weak_ordering order, Mutator m)
    {
      testing::check_semantics(append_lines(description, emphasise("Regular Semantics")), logger(), x, y, order, std::move(m));
    }
  protected:
    ~regular_extender() = default;
  private:
    [[nodiscard]]
    test_logger<Mode>& logger() noexcept { return *m_pLogger; }

    test_logger<Mode>* m_pLogger;
  };

  template<test_mode mode>
  using regular_checker = checker<mode, regular_extender<mode>>;

  template<test_mode mode>
  using canonical_regular_test = basic_test<regular_checker<mode>>;

  /*! \anchor regular_test_alias */
  using regular_test                = canonical_regular_test<test_mode::standard>;
  using regular_false_negative_test = canonical_regular_test<test_mode::false_negative>;
  using regular_false_positive_test = canonical_regular_test<test_mode::false_positive>;
}
