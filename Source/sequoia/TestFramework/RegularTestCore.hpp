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
  [[nodiscard]]
  std::string regular_message(std::string_view description);

  /*! \brief Extender for testing classes exhibiting regular/std::totally_ordered semantics.

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

    regular_extender() = default;

    /// Prerequisite: x!=y
    template<pseudoregular T, class Self>
    void check_semantics(this Self& self, const reporter& description, const T& x, const T& y)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               optional_ref<const T>{},
                               optional_ref<const T>{});
    }

    /*! Prerequisites:
          x!=y
          x==xEquivalent
          y==yEquivalent
     */
    template<pseudoregular T, class U, class Self>
      requires equivalence_checkable_for_semantics<Mode, T, U>
    void check_semantics(this Self& self,
                         const reporter& description,
                         const T& x,
                         const T& y,
                         const U& xEquivalent,
                         const U& yEquivalent)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               xEquivalent,
                               yEquivalent,
                               optional_ref<const T>{},
                               optional_ref<const T>{});
    }

    /// Prerequisite: x!=y, with values consistent with order
    template<pseudoregular T, class Self>
      requires std::totally_ordered<T>
    void check_semantics(this Self& self, const reporter& description, const T& x, const T& y, std::weak_ordering order)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               optional_ref<const T>{},
                               optional_ref<const T>{},
                               order);
    }

    /*! Prerequisites:
          x!=y, with values consistent with order
          x==xEquivalent
          y==yEquivalent
     */
    template<pseudoregular T, class U, class Self>
      requires std::totally_ordered<T> && equivalence_checkable_for_semantics<Mode, T, U>
    void check_semantics(this Self& self,
                         const reporter& description,
                         const T& x,
                         const T& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         std::weak_ordering order)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               xEquivalent,
                               yEquivalent,
                               optional_ref<const T>{},
                               optional_ref<const T>{},
                               order);
    }

    /// Prerequisite: x!=y
    template<pseudoregular T, std::invocable<T&> Mutator, class Self>
    void check_semantics(this Self& self, const reporter& description, const T& x, const T& y, Mutator m)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               optional_ref<const T>{},
                               optional_ref<const T>{},
                               std::move(m));
    }

    /*! Prerequisites:
          x!=y
          x==xEquivalent
          y==yEquivalent
     */
    template<pseudoregular T, class U, std::invocable<T&> Mutator, class Self>
      requires equivalence_checkable_for_semantics<Mode, T, U>
    void check_semantics(this Self& self,
                         const reporter& description,
                         const T& x,
                         const T& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         Mutator m)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               xEquivalent,
                               yEquivalent,
                               optional_ref<const T>{},
                               optional_ref<const T>{},
                               std::move(m));
    }

    /// Prerequisites: x!=y, with values consistent with order
    template<pseudoregular T, std::invocable<T&> Mutator, class Self>
      requires std::totally_ordered<T>
    void check_semantics(this Self& self, const reporter& description, const T& x, const T& y, std::weak_ordering order, Mutator m)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               optional_ref<const T>{},
                               optional_ref<const T>{},
                               order,
                               std::move(m));
    }

    /*! Prerequisites:
          x!=y, with values consistent with order
          x==xEquivalent
          y==yEquivalent
     */
    template<pseudoregular T, class U, std::invocable<T&> Mutator, class Self>
      requires std::totally_ordered<T> && equivalence_checkable_for_semantics<Mode, T, U>
    void check_semantics(this Self& self,
                         const reporter& description,
                         const T& x,
                         const T& y,
                         const U& xEquivalent,
                         const U& yEquivalent,
                         std::weak_ordering order,
                         Mutator m)
    {
      testing::check_semantics(regular_message(self.report(description)),
                               self.m_Logger,
                               x,
                               y,
                               xEquivalent,
                               yEquivalent,
                               optional_ref<const T>{},
                               optional_ref<const T>{},
                               order,
                               std::move(m));
    }
  protected:
    ~regular_extender() = default;

    regular_extender(regular_extender&&)            noexcept = default;
    regular_extender& operator=(regular_extender&&) noexcept = default;
  };

  template<test_mode mode>
  using canonical_regular_test = basic_test<mode, regular_extender<mode>>;

  /*! \anchor regular_test_alias */
  using regular_test                = canonical_regular_test<test_mode::standard>;
  using regular_false_positive_test = canonical_regular_test<test_mode::false_positive>;
  using regular_false_negative_test = canonical_regular_test<test_mode::false_negative>;
}
