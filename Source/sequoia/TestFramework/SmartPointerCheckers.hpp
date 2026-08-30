////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Specializations of the class template value_tester for the smart pointers.

    See PointerCheckers.hpp for raw pointers, and ConcreteTypeCheckers.hpp for a single header
    pulling in every specialization at once.
 */

#include "sequoia/TestFramework/FreeCheckers.hpp"

#include <memory>

namespace sequoia::testing
{
  /** \brief Helper for testing smart pointers
  
      The general pattern for smart pointers is that `test(equality, ...)` checks for equality
      of the underlying pointers, whereas `test(equivalence, ...) checks the pointees, using
      the strongest available check.
   */

  template<class T>
  struct smart_pointer_tester
  {
    using type = T;

    template<test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Underlying pointers differ", logger, obtained.get(), prediction.get(), advisor);
    }
  protected:
    ~smart_pointer_tester() = default;

    template<test_mode Mode, class Advisor>
    static void test_pointees(test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      if(obtained && prediction)
      {
        check(with_best_available, "Pointees differ", logger, *obtained, *prediction, advisor);
      }
      else
      {
        const bool obtainedIsNull{obtained}, predictionIsNull{prediction};

        check(equality,
              nullable_type_message(obtainedIsNull, predictionIsNull),
              logger,
              static_cast<bool>(obtained),
              static_cast<bool>(prediction));
      }
    }
  };

  /** \brief Compares instance of `std::unique_ptr`

      The `test(equality_check_t,...)` overload checks that the underlying pointers point to the same thing.

      The `test(equivalence_check_t,...)` overload checks whether the pointers either both point to
      something or both point to nullptr, reporting a failure if this is not the case. If both
      pointers are not null, a check is dispatched to test the bound type. This is done using
      the strongest available check.
   */

  template<class T>
  struct value_tester<std::unique_ptr<T>> : smart_pointer_tester<std::unique_ptr<T>>
  {
    using type = std::unique_ptr<T>;
    using base_t = smart_pointer_tester<std::unique_ptr<T>>;
    using base_t::test;

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      base_t::test_pointees(logger, obtained, prediction, advisor);
    }
  };

  /** \brief Compares instance of `std::shared_ptr`

      The `test(equality_check_t,...)` overload checks that the underlying pointers point to the same thing.

      The `test(equivalence_check_t,...)` overload checks whether the pointers either both point to
      something or both point to nullptr, reporting a failure if this is not the case. If both
      pointers are not null, a check is dispatched to test the underlying type. This is done using
      the strongest available check.
   */

  template<class T>
  struct value_tester<std::shared_ptr<T>> : smart_pointer_tester<std::shared_ptr<T>>
  {
    using type = std::shared_ptr<T>;
    using base_t = smart_pointer_tester<std::shared_ptr<T>>;
    using base_t::test;

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      base_t::test_pointees(logger, obtained, prediction, advisor);
    }
  };

  /** \brief Compares instance of `std::weak_ptr`

      Comparison is performed by calling lock on the `obtained` and `predicted`
      `std::weak_ptr`s and then comparing the nascent `std::shared_ptr`s.
   */

  template<class T>
  struct value_tester<std::weak_ptr<T>>
  {
    using type = std::weak_ptr<T>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction)
    {
      check(equality, "Underlying pointers differ", logger, obtained.lock(), prediction.lock());
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction)
    {
      check(equivalence, "Underlying pointers differ", logger, obtained.lock(), prediction.lock());
    }
  };
}
