////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FactoryTestingDiagnostics.hpp"

namespace sequoia::testing
{  
  [[nodiscard]]
  std::string_view factory_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void factory_false_positive_test::run_tests()
  {
    using namespace runtime;

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int>>, 1>;
      factory<int> f{{"int"}};
      check_equivalence(LINE(""), f, prediction_type{{{"int", 5}}});

      factory<int> g{{"int"}, 1};
      check_equivalence(LINE(""), g, prediction_type{{{"int", 5}}});
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int, double>>, 2>;
      factory<int, double> f{{"int", "double"}};
      check_equivalence(LINE(""), f, prediction_type{{{"int", 0}, {"double", 5.0}}});
    }
  }
}
