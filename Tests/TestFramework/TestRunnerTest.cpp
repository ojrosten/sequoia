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

    struct flipper
    {
      flipper() { x = !x; }

      inline static bool x{};
    };

    class flipper_free_test final : public free_test
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
        check_equivalence(LINE(""), flipper{}, true);
      }
    };

    struct counter
    {
      counter() { x = ++x; }

      inline static int x{};
    };

    class counter_free_test final : public free_test
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
        check_equivalence(LINE(""), counter{}, 1);
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

  template<>
  struct equivalence_checker<flipper>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const flipper& obtained, bool prediction)
    {
      check_equality("Wrapped value", logger, obtained.x, prediction);
    }
  };

  template<>
  struct equivalence_checker<counter>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const counter& obtained, int prediction)
    {
      check_equality("Wrapped value", logger, obtained.x, prediction);
    }
  };

  [[nodiscard]]
  std::string_view test_runner_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_test::run_tests()
  {
    test_name_and_source_duplication();
    test_critical_errors();
    test_instability_analysis("Instability comprising pass/failure",
                              flipper_free_test{"Free Test"},
                              "BinaryInstabilityAnalysis",
                              3);
  }

  [[nodiscard]]
  std::filesystem::path test_runner_test::aux_project() const
  {
    return auxiliary_materials() /  "FakeProject";
  }

  void test_runner_test::test_name_and_source_duplication()
  {
    check_exception_thrown<std::runtime_error>(
      LINE(""),
      [=](){
        commandline_arguments args{""};
        const auto testMain{aux_project().append("TestSandbox").append("TestSandbox.cpp")};
        const auto includeTarget{aux_project().append("TestShared").append("SharedIncludes.hpp")};
        std::stringstream outputStream{};
  
        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           project_paths{aux_project(), testMain, includeTarget},
                           "  ",
                           outputStream};

        runner.add_test_family(
          "Duplicates",
          foo_test{"Free Test"},
          flipper_free_test{"Free Test"}
        );
      },
      [](std::string mess) {
        constexpr std::string_view pattern{"Source file: \""};
        constexpr auto npos{std::string::npos};
        if(const auto pos{mess.find(pattern)}; pos != npos)
        {
          const auto startPos{pos + pattern.size()};
          if(auto endPos{mess.find("/Tests", startPos)}; endPos != npos)
          {
            mess.replace(startPos, endPos - startPos, "...");
          }
        }
        return mess;
      }
    );
  }

  void test_runner_test::test_critical_errors()
  {
    std::stringstream outputStream{};

    // This is scoped to ensure destruction of the runner - and therefore loggers -
    // before dumping output to a file. The destructors are not trivial in recovery mode.
    {
      commandline_arguments args{"", "-v", "--recovery", "dump",
                                 "test", "Bar",
                                 "test", "Foo"};

      const auto testMain{aux_project().append("TestSandbox").append("TestSandbox.cpp")};
      const auto includeTarget{aux_project().append("TestShared").append("SharedIncludes.hpp")};
  
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         project_paths{aux_project(), testMain, includeTarget},
                         "  ",
                         outputStream};

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

    const auto outputDir{working_materials() / "RecoveryAndDumpOutput"};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    fs::copy(aux_project() / "output" / "Recovery" / "Recovery.txt", working_materials() / "RecoveryAndDumpOutput");
    fs::copy(aux_project() / "output" / "Recovery" / "Dump.txt", working_materials() / "RecoveryAndDumpOutput");
    fs::copy(aux_project() / "output" / "TestSummaries",
             working_materials() / "RecoveryAndDumpOutput" / "TestSummaries",
             fs::copy_options::recursive);
    

    check_equivalence(LINE("Recovery and Dump"),
                      working_materials() / "RecoveryAndDumpOutput",
                      predictive_materials() / "RecoveryAndDumpOutput");
  }

  template<concrete_test T>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   T t,
                                                   std::string_view outputDirName,
                                                   std::size_t numRuns)
  {
    std::stringstream outputStream{};

    commandline_arguments args{"", "--num", std::to_string(numRuns)};

    const auto testMain{aux_project().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{aux_project().append("TestShared").append("SharedIncludes.hpp")};

    test_runner runner{args.size(),
      args.get(),
      "Oliver J. Rosten",
      project_paths{aux_project(), testMain, includeTarget},
      "  ",
      outputStream};

    runner.add_test_family(
      "Family",
      std::move(t)
    );

    runner.execute();

    const auto outputDir{working_materials() / outputDirName};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    check_equivalence(LINE(add_type_info<T>(message)),
                      outputDir,
                      predictive_materials() / outputDirName);
  }
}
