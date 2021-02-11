////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTest.hpp"
#include "CommandLineArgumentsTestingUtilities.hpp"

#include "TestRunner.hpp"


namespace sequoia::testing
{
  namespace
  {
    class foo_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        check_equality(LINE(""), 1, 1);
        throw std::runtime_error{"Throw after check"};
      }
    };
  }
  
  [[nodiscard]]
  std::string_view test_runner_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_test::run_tests()
  {
    test_critical_errors();
  }

  void test_runner_test::test_critical_errors()
  {
    auto working{
      [&mat{working_materials()}]() { return mat / "FakeProject"; }
    };

    commandline_arguments args{""};
    const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
    const repositories repos{working()};
    std::stringstream outputStream{};

    test_runner runner{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    runner.add_test_family(
      "Foo",
      foo_free_test("Free Test")
    );

    runner.execute();

    if(std::ofstream file{working() / "output" / "io.txt"})
    {
      file << outputStream.str();
    }

    check_equivalence(LINE(""), working(), predictive_materials() / "FakeProject");
  }
}
