////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:PerformanceTestTraits;

import std;

/** \file
    \brief The trait identifying performance tests, which
           PerformanceTestCore.cppm specializes.
*/

export namespace sequoia::testing
{
  /** \anchor is_performance_test primary class template */
  template<class T>
  struct is_performance_test : std::false_type {};

  template<class T>
  using is_performance_test_t = is_performance_test<T>::type;

  template<class T>
  inline constexpr bool is_performance_test_v{is_performance_test<T>::value};
}
