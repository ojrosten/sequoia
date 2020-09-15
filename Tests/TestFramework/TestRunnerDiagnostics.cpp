////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerDiagnostics.hpp"
#include "CommandLineArgumentsTestingUtilities.hpp"

#include "TestRunner.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_false_negative_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_false_negative_test::run_tests()
  {
    test_creation();
  }

  void test_runner_false_negative_test::test_creation()
  {
    namespace fs = std::filesystem;

    auto fake{
      [&mat{materials()}]() {
        return mat / "Before" / "FakeProject";
      }
    };
    
    const auto testMain{fake().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{fake().append("TestShared").append("SharedIncludes.hpp")};
    const auto testRepo{fake().append("Tests")};
    const auto materialsRepo{fake().append("TestMaterials")};
    const auto sourceRepo{fake().append("Source")};
    const auto outputDir{fake().append("output")};

    using namespace parsing::commandline;

    std::stringstream outputStream{};
    commandline_arguments args{"", "create", "ordered_test", "other::functional::maybe<class T>", "std::optional<T>"
                                 , "create", "ordered_test", "utilities::iterator", "int*"
                                 , "create", "move_only_test", "bar::baz::foo<class T>", "T"
                                 , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                      "-h", (testRepo / "Partners").string(), "-f", "partners", "-ch", "Couple.hpp"};
    
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, sourceRepo, testRepo, materialsRepo, outputDir, outputStream};

    tr.execute();
  }
}
