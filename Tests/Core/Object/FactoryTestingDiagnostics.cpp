////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FactoryTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace object;

  [[nodiscard]]
  std::filesystem::path factory_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void factory_false_negative_test::run_tests()
  {
    {
      using prediction_t = std::array<std::pair<std::string, std::variant<int>>, 1>;
 
      factory<int> f{"int"};
      using check_t = value_tester<factory<int>>::factory_check_type<int>;
      
      check(equivalence, "", f, prediction_t{{{"int", 5}}});
      check(check_t{4}, "", f, prediction_t{{{"int", 5}}});
    }

    {
      using prediction_t = std::array<std::pair<std::string, std::variant<int, double>>, 2>;
      factory<int, double> f{"int", "double"};
      check(equivalence, "", f, prediction_t{{{"int", 0}, {"double", 5.0}}});
    }

    {
      using factory_t = factory<int, double>;
      using vessels = std::vector<factory_t::vessel>;

      factory_t f{"int", "double"};

      // Both products wrong, so the single check this makes is bound to fail, as an entry in a
      // false-negative test must.
      check(equality, "make_all", f.make_all(), vessels{{5.0}, {5}});

      // The right number, the wrong product: `make_if` admits "int" alone.
      check(equality, "make_if", f.make_if([](std::string_view name){ return name == "int"; }), vessels{{0.0}});
    }
  }
}
