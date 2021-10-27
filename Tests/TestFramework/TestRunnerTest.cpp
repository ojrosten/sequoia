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

    class failing_test final : public free_test
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
        check_equality("Standard Failure", 43, 42);
      }
    };

    class failing_fp_test final : public free_false_positive_test
    {
    public:
      using free_false_positive_test::free_false_positive_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }

    private:
      void run_tests() final
      {
        check_equality("False positive failure", 42, 42);
      }
    };

    class failing_fn_test final : public free_false_negative_test
    {
    public:
      using free_false_negative_test::free_false_negative_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }

    private:
      void run_tests() final
      {
        check_equality("False negative failure", 43, 42);
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
        check_equality("Flipper", flipper{}.x, true);
      }
    };

    template<std::size_t N>
    struct periodic
    {
      periodic() { x = (++x) % N;}
      
      inline static int x{};
    };

    class periodic_free_test final : public free_test
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
        check_equality("Period 4", periodic<4>{}.x, 1);
      }
    };


    class multi_periodic_free_test final : public free_test
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
        check_equality("Period 2: Pass/Fail/Pass", periodic<2>{}.x, 1);
        check("Period 3: Pass/Pass/Fail", periodic<3>{}.x > 0);
      }
    };

    class failing_plus_instabilities_free_test final : public free_test
    {
      using free_test::free_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        check("Always fails", false);

        check_equality("Flipper", flipper{}.x, true);
      }
    };

    class consistently_failing_free_test : public free_test
    {
      using free_test::free_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        check("Always fails", false);
      }
    };

    template<std::size_t N>
    class consistently_passing_free_test : public free_test
    {
      using free_test::free_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        check("Always passes", true);
      }
    };

    class critical_free_test : public free_test
    {
      using free_test::free_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final
      {
        return __FILE__;
      }
    private:
      void run_tests() final
      {
        if(flipper{}.x)
          throw std::runtime_error{"Error"};
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
    test_exceptions();
    test_critical_errors();
    test_basic_output();
    test_instability_analysis();
  }

  [[nodiscard]]
  std::filesystem::path test_runner_test::aux_project() const
  {
    return auxiliary_materials() /  "FakeProject";
  }

  void test_runner_test::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(
      LINE("Neither name nor source unique"),
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

    check_exception_thrown<std::runtime_error>(
      LINE("Invalid repetitions for instability analysis"),
      [this](){
        test_instability_analysis("", "", "foo", critical_free_test{"Free Test"});
      }
    );

    check_exception_thrown<std::runtime_error>(
      LINE("Insufficient repetitions for instability analysis"),
      [this](){
        test_instability_analysis("", "",  "1", critical_free_test{"Free Test"});
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

  void test_runner_test::test_basic_output()
  {
    std::stringstream outputStream{};
    commandline_arguments args{""};

    const auto testMain{aux_project().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{aux_project().append("TestShared").append("SharedIncludes.hpp")};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       project_paths{aux_project(), testMain, includeTarget},
                       "  ",
                       outputStream};

    runner.add_test_family(
      "Failing Family",
      failing_test{"Free Test"},
      failing_fp_test{"False positive Test"},
      failing_fn_test{"False negative Test"}
    );

    runner.execute();

    const auto outputDir{working_materials() / "BasicOutput"};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    check_equivalence(LINE("Basic Output"), working_materials() / "BasicOutput",  predictive_materials() / "BasicOutput");
  }

  void test_runner_test::test_instability_analysis()
  {
    test_instability_analysis("Instability comprising pass/failure",
                              "BinaryInstabilityAnalysis",
                              "2",
                              {"-a", "null"},
                              flipper_free_test{"Free Test"});

    test_instability_analysis("Instability comprising pass/multiple distinct failures",
                              "MultiInstabilityAnalysis",
                              "4",
                              {"-a", "null"},
                              periodic_free_test{"Free Test"});

    test_instability_analysis("Instability comprising failures from two checks",
                              "MultiCheckInstabilityAnalysis",
                              "6",
                              {"-a", "null"},
                              multi_periodic_free_test{"Free Test"});

    test_instability_analysis("Instability following consistent failure",
                              "BinaryInstabilityFollowingFailures",
                              "2",
                              {"-a", "null"},
                              failing_plus_instabilities_free_test{"Free Test"});

    test_instability_analysis("Failure but no instability",
                              "ConsistentFailureNoInstability",
                              "2",
                              {"-a", "null"},
                              consistently_failing_free_test{"Free Test"});

    test_instability_analysis("Always passes",
                              "ConsistentSuccessNoInstability",
                              "2",
                              {"-a", "null"},
                              consistently_passing_free_test<0>{"Free Test"});

    test_instability_analysis("Critical failure instability",
                              "CriticalFailureInstability",
                              "2",
                              {"-a", "null"},
                              critical_free_test{"Free Test"});

    test_instability_analysis("Two tests always passing",
                              "ConsistentSuccessTwoTests",
                              "2",
                              {"-a", "null"},
                              consistently_passing_free_test<0>{"Free Test 0"},
                              consistently_passing_free_test<1>{"Free Test 1"});

    test_instability_analysis("Consistent success/consistent failure/instability",
                              "MixedBag",
                              "6",
                              {"-a", "null"},
                              consistently_passing_free_test<0>{"Passing Free Test"},
                              consistently_failing_free_test{"Failing Free Test"},
                              flipper_free_test{"Flipper Free Test"},
                              multi_periodic_free_test("Free Test")
                             );

    test_instability_analysis("Family selection",
                              "InstabilityFamilySelection",
                              "2",
                              {"test", "Another Family"},
                              [](test_runner& r){ r.add_test_family("Another Family", flipper_free_test{"Flipper Free Test"}); },
                              flipper_free_test{"Flipper Free Test"}
                             );
  }

  template<std::invocable<test_runner&> Manipulator, concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   std::initializer_list<std::string_view> extraArgs,
                                                   Manipulator manipulator,
                                                   Ts&&... ts)
  {
    std::stringstream outputStream{};

    auto argGenerator{
      [&extraArgs, numRuns](){
         std::vector<std::string> argList{"", "locate-instabilities", std::string{numRuns}};
         argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
         return argList;
      }
    };

    commandline_arguments args{argGenerator()};

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
      std::forward<Ts>(ts)...
    );

    manipulator(runner);

    runner.execute();

    const auto outputDir{working_materials() / outputDirName};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    check_equivalence(LINE(append_lines(message, make_type_info<Ts...>())),
                      outputDir,
                      predictive_materials() / outputDirName);
  }

  template<concrete_test... Ts>
    void test_runner_test::test_instability_analysis(std::string_view message,
                                                     std::string_view outputDirName,
                                                     std::string_view numRuns,
                                                     std::initializer_list<std::string_view> extraArgs,
                                                     Ts&&... ts)
    {
      test_instability_analysis(message, outputDirName, numRuns, extraArgs, [](test_runner&){}, std::forward<Ts>(ts)...);
    }

  template<concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   Ts&&... ts)
  {
    test_instability_analysis(message, outputDirName, numRuns, {}, [](test_runner&){}, std::forward<Ts>(ts)...);
  }
}
