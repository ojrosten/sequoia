////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TypeListFreeTest.hpp"
#include "sequoia/Core/Meta/TypeList.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path type_list_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void type_list_free_test::run_tests()
  {
      test_type_list();
      test_type_list_union();
  }

  void type_list_free_test::test_type_list()
  {
      check(report(""), [](){ static_assert(std::is_same_v<faithful_type_list<>, type_list<>>);                                         return true; }());
      check(report(""), [](){ static_assert(std::is_same_v<faithful_type_list<int>, type_list<int>>);                                   return true; }());
      check(report(""), [](){ static_assert(std::is_same_v<faithful_type_list<int, int>, type_list<int>>);                              return true; }());
      check(report(""), [](){ static_assert(std::is_same_v<faithful_type_list<int, int, int>, type_list<int>>);                         return true; }());
      check(report(""), [](){ static_assert(std::is_same_v<faithful_type_list<int, double, int, int, double>, type_list<int, double>>); return true; }());
  }

  void type_list_free_test::test_type_list_union()
  {
      check(report(""), [](){ static_assert(std::is_same_v<type_list<int>, type_list_union_t<type_list<int>>>);                            return true; }());
      check(report(""), [](){ static_assert(std::is_same_v<type_list<int, double>, type_list_union_t<type_list<int>, type_list<double>>>); return true; }());
      check(report(""), [](){ static_assert(std::is_same_v<type_list<int>, type_list_union_t<type_list<int>, type_list<int>>>);            return true; }());
  }
}
