////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestRunnerTest.hpp"

#include "UnitTestRunner.hpp"

namespace sequoia::unit_testing
{
  void unit_test_runner_test::run_tests()
  {
    test_parser();
  }

  void unit_test_runner_test::test_parser()
  {
    using args = std::vector<std::vector<std::string>>;
    
    {
      const auto parsed{parse(0, nullptr)};
      check_equality(LINE(""), parsed, args{});
    }

    {
      std::array<char*, 2> a{new char[4]{"foo"}, new char[5]{"test"}};
      
      check_equality(LINE(""), parse(2, &a[0]), args{{"test"}});

      for(auto e : a) delete e;
    }
  }
}
