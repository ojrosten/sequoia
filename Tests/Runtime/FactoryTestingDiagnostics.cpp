////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FactoryTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace object;

  [[nodiscard]]
  std::filesystem::path factory_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void factory_false_positive_test::run_tests()
  {
    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int>>, 1>;
      factory<int> f{{"int"}};
      check(equivalence, "", f, prediction_type{{{"int", 5}}});
      check(equivalence, "", f, prediction_type{{{"int", 5}}}, 4);
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int, double>>, 2>;
      factory<int, double> f{{"int", "double"}};
      check(equivalence, "", f, prediction_type{{{"int", 0}, {"double", 5.0}}});
    }
  }
}
