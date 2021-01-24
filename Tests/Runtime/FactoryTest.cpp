////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FactoryTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view factory_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void factory_test::run_tests()
  {
    using namespace runtime;

    factory<int, double> f{{"int", "double"}}, g{{"foo", "bar"}};
    check_semantics(LINE(""), f, g);
  }
}
