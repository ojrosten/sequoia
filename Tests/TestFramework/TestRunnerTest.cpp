////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTest.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct foo
    {
      int x{};
    };

    class foo_test final : public regular_test
    {
    public:
      using regular_test::regular_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        check_equality("Throw during check", foo{}, foo{});
      }
    };
  }

  template<>
  struct detailed_equality_checker<foo>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>&, const foo&, const foo&)
    {
      throw std::runtime_error{"This is bad"};
    }
  };

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
    auto auxiliary{
      [&mat{auxiliary_materials()}]() { return mat / "FakeProject"; }
    };

    std::stringstream outputStream{};

    // This is scoped to ensure destruction of the runner - and therefore loggers -
    // before dumping output to a file. The destructors are not trivial in recovery mode.
    {
      commandline_arguments args{"", "-v", "--recovery", "dump",
                                 "test", "Bar",
                                 "test", "Foo"};

      const auto testMain{auxiliary().append("TestSandbox").append("TestSandbox.cpp")};
      const auto includeTarget{auxiliary().append("TestShared").append("SharedIncludes.hpp")};
  
      test_runner runner{args.size(), args.get(), "Oliver J. Rosten", project_paths{auxiliary(), testMain, includeTarget}, "  ", outputStream};

      runner.add_test_family(
        "Bar",
        bar_free_test{"Free Test"}
      );

      runner.add_test_family(
        "Foo",
        foo_test{"Unit Test"}
      );

      runner.add_test_family(
        "Baz",
        foo_test{"Unit Test"}
      );

      runner.execute();
    }

    fs::create_directory(working_materials() / "RecoveryAndDumpOutput");

    if(std::ofstream file{working_materials() / "RecoveryAndDumpOutput" / "io.txt"})
    {
      file << outputStream.str();
    }

    fs::copy(auxiliary() / "output" / "Recovery" / "Recovery.txt", working_materials() / "RecoveryAndDumpOutput");
    fs::copy(auxiliary() / "output" / "Recovery" / "Dump.txt", working_materials() / "RecoveryAndDumpOutput");
    fs::copy(auxiliary() / "output" / "TestSummaries",
             working_materials() / "RecoveryAndDumpOutput" / "TestSummaries",
             fs::copy_options::recursive);
    

    check_equivalence(LINE("Recovery and Dump"),
                      working_materials() / "RecoveryAndDumpOutput",
                      predictive_materials() / "RecoveryAndDumpOutput");
  }
}
