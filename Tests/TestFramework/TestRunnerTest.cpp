////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, reporter{"Throw during check"}, foo{}, foo{});
      }
    };

    class passing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, reporter{"Integer equality"}, 42, 42);
      }
    };

    class failing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, {"Standard Failure"}, 43, 42);
      }
    };

    class failing_fp_test final : public free_false_positive_test
    {
    public:
      using free_false_positive_test::free_false_positive_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, {"False positive failure"}, 42, 42);
      }
    };

    class failing_fn_test final : public free_false_negative_test
    {
    public:
      using free_false_negative_test::free_false_negative_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, {"False negative failure"}, 43, 42);
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
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, {"Flipper"}, flipper{}.x, true);
      }
    };

    template<std::size_t N>
    struct periodic
    {
      periodic() { x = (x+1) % N;}
      
      inline static int x{};
    };

    class periodic_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, {"Period 4"}, periodic<4>{}.x, 1);
      }
    };


    class multi_periodic_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, {"Period 2: Pass/Fail/Pass"}, periodic<2>{}.x, 1);
        check({"Period 3: Pass/Pass/Fail"}, periodic<3>{}.x > 0);
      }
    };

    class failing_plus_instabilities_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check({"Always fails"}, false);

        check(equality, {"Flipper"}, flipper{}.x, true);
      }
    };

    class consistently_failing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check({"Always fails"}, false);
      }
    };

    template<std::size_t N>
    class consistently_passing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check({"Always passes"}, true);
      }
    };

    class critical_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::filesystem::path source_file() const
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        if(flipper{}.x)
          throw std::runtime_error{"Error"};
      }
    };

    test_runner make_failing_suite(commandline_arguments args, std::stringstream& outputStream)
    {
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                         "  ",
                         outputStream};

      runner.add_test_suite(
        "Failing Suite",
        failing_test{"Free Test"},
        failing_fp_test{"False positive Test"},
        failing_fn_test{"False negative Test"}
      );

      return runner;
    }
  }

  template<>
  struct value_tester<foo>
  {
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>&, const foo&, const foo&)
    {
      throw std::runtime_error{"This is bad"};
    }
  };

  [[nodiscard]]
  std::filesystem::path test_runner_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_runner_test::run_tests()
  {
    test_exceptions();
    test_critical_errors();
    test_basic_output();
    test_verbose_output();
    test_serial_verbose_output();
    test_filtered_suites();
    test_prune_basic_output();
    test_nested_suite();
    test_nested_suite_verbose();
    test_instability_analysis();
  }

  [[nodiscard]]
  std::filesystem::path test_runner_test::fake_project() const
  {
    return auxiliary_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path test_runner_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade");
  }

  [[nodiscard]]
  std::string test_runner_test::zeroth_arg() const
  {
    return (minimal_fake_path()).generic_string();
  }

  fs::path test_runner_test::write(std::string_view dirName, std::stringstream& output) const
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);

    const auto filePath{outputDir / "io.txt"};
    if(std::ofstream file{filePath})
    {
      file << output.str();
    }

    output.str("");

    return filePath;
  }

  fs::path test_runner_test::check_output(std::string_view description, std::string_view dirName, std::stringstream& output)
  {
    fs::path filePath{write(dirName, output)};
    check(equivalence, description, working_materials() /= dirName, predictive_materials() /= dirName);

    return filePath;
  }

  void test_runner_test::test_exceptions()
  {
    auto pathTrimmer{
      [](std::string mess) {
        if(mess.find("canonical") != std::string::npos)
        {
          mess = "canonical: unable to find path";
        }
        else if(const auto pos{mess.find("output/")}; pos < mess.size())
        {
          const auto startPos{mess.rfind('\n', pos)};
          const auto eraseFrom{startPos < pos ? startPos + 1 : 0};

          mess.erase(eraseFrom, pos - eraseFrom);
        }

        return mess;
      }
    };

    check_exception_thrown<std::runtime_error>(
      reporter{"Test Main has empty path"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten",{"", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
      },
      pathTrimmer);

    check_exception_thrown<std::runtime_error>(
      reporter{"Test Main does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"FooMain.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
      },
      pathTrimmer);

    check_exception_thrown<std::runtime_error>(
      reporter{"Include Target has empty path"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, ""}, "  ", outputStream};
      },
      pathTrimmer);

    check_exception_thrown<std::runtime_error>(
      reporter{"Include Target does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "FooPath.hpp"}, "  ", outputStream};
      },
      pathTrimmer);

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root is empty"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{""}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{(fake_project() / "FooRepo").generic_string()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
      },
      pathTrimmer);

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root not findable"},
      [this]() {
        const auto zerothArg{fake_project().append("TestShared").generic_string()};
        std::stringstream outputStream{};
        commandline_arguments args{{zerothArg}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
      },
      pathTrimmer);

    check_exception_thrown<std::runtime_error>(
      reporter{"Neither name nor source unique"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};
  
        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                           "  ",
                           outputStream};

        runner.add_test_suite(
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
      reporter{"Invalid repetitions for instability analysis"},
      [this](){
        test_instability_analysis("", "", "foo", critical_free_test{"Free Test"});
      }
    );

    check_exception_thrown<std::runtime_error>(
      reporter{"Insufficient repetitions for instability analysis"},
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
      commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v", "recover", "dump",
                                 "test", "Bar",
                                 "test", "Foo"}};
  
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                         "  ",
                         outputStream};

      runner.add_test_suite(
        "Bar",
        bar_free_test{"Free Test"}
      );

      runner.add_test_suite(
        "Foo",
        foo_test{"Unit Test"}
      );

      runner.add_test_suite(
        "Baz",
        foo_test{"Unit Test"}
      );

      runner.execute();
    }

    const auto outputDir{working_materials() /= "RecoveryAndDumpOutput"};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    fs::copy(fake_project() / "output" / "Recovery" / "Recovery.txt", working_materials() /= "RecoveryAndDumpOutput");
    fs::copy(fake_project() / "output" / "Recovery" / "Dump.txt", working_materials() /= "RecoveryAndDumpOutput");
    fs::copy(fake_project() / "output" / "TestSummaries",
             working_materials() /= "RecoveryAndDumpOutput/TestSummaries",
             fs::copy_options::recursive);

    check(equivalence, "Recovery and Dump",
                      working_materials() /= "RecoveryAndDumpOutput",
                      predictive_materials() /= "RecoveryAndDumpOutput");
  }

  void test_runner_test::test_basic_output()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                       "  ",
                       outputStream};

    runner.execute();
    check_output(report({"No Tests"}), "NoTests", outputStream);

    runner.add_test_suite(
      "Failing Suite",
      failing_test{"Free Test"},
      failing_fp_test{"False positive Test"},
      failing_fn_test{"False negative Test"}
    );

    runner.execute();
    check_output(report({"Basic Output"}), "BasicOutput", outputStream);
  }

  void test_runner_test::test_verbose_output()
  {
    std::stringstream outputStream{};
    auto runner{make_failing_suite({{(minimal_fake_path()).generic_string(), "-v"}}, outputStream)};

    runner.execute();
    check_output(report({"Basic Verbose Output"}), "BasicVerboseOutput", outputStream);
  }

  void test_runner_test::test_serial_verbose_output()
  {
    std::stringstream outputStream{};
    auto runner{make_failing_suite({{(minimal_fake_path()).generic_string(), "-v", "--serial"}}, outputStream)};

    runner.execute();
    check_output(report({"Basic Serial Verbose Output"}), "BasicSerialVerboseOutput", outputStream);
  }

  void test_runner_test::test_filtered_suites()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "test", "Failing Suite"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                       "  ",
                       outputStream};

    runner.add_test_suite(
      "Passing Suite",
      passing_test{"Free Test"}
    );

    runner.add_test_suite(
      "Failing Suite",
      failing_test{"Free Test"},
      failing_fp_test{"False positive Test"},
      failing_fn_test{"False negative Test"}
    );

    runner.execute();
    check_output(report({"Filtered Suite Output"}), "FilteredSuiteOutput", outputStream);
  }

  void test_runner_test::test_prune_basic_output()
  {
    fs::remove_all(output_paths{fake_project()}.dir());

    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                       "  ",
                       outputStream};

    runner.execute();
    check_output(report({"Prune with no stamp"}), "PruneWithNoStamp", outputStream);

    runner.execute();
    check_output(report({"Prune with no tests"}), "PruneWithNoTests", outputStream);
  }

  void test_runner_test::test_nested_suite()
  {
      std::stringstream outputStream{};
      commandline_arguments args{{(minimal_fake_path()).generic_string()}};

      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                         "  ",
                         outputStream};

      using namespace object;

      runner.add_test_suite(
        "Failing Suite",
        suite{"Free Suite",
              failing_test{"Free Test"}
        },
        suite{"Diagnostics Suite",
              failing_fp_test{"False positive Test"},
              failing_fn_test{"False negative Test"}
        }
      );

      runner.execute();
      check_output(report({"Basic Nested Output"}), "BasicNestedOutput", outputStream);
  }

  void test_runner_test::test_nested_suite_verbose()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                       "  ",
                       outputStream};

    using namespace object;

    runner.add_test_suite(
      "Failing Suite",
      suite{"Free Suite",
            failing_test{"Free Test"}
      },
      suite{"Diagnostics Suite",
            failing_fp_test{"False positive Test"},
            failing_fn_test{"False negative Test"}
      }
    );

    runner.execute();
    check_output(report({"Verbose Nested Output"}), "VerboseNestedOutput", outputStream);
  }

  void test_runner_test::test_instability_analysis()
  {
    test_instability_analysis("Instability comprising pass/failure",
                              "BinaryInstabilityAnalysis",
                              "2",
                              {"--serial"},
                              flipper_free_test{"Free Test"});

    test_instability_analysis("Instability comprising pass/multiple distinct failures",
                              "MultiInstabilityAnalysis",
                              "4",
                              {"--serial"},
                              periodic_free_test{"Free Test"});

    test_instability_analysis("Instability comprising failures from two checks",
                              "MultiCheckInstabilityAnalysis",
                              "6",
                              {"--serial"},
                              multi_periodic_free_test{"Free Test"});

    test_instability_analysis("Instability following consistent failure",
                              "BinaryInstabilityFollowingFailures",
                              "2",
                              {"--serial"},
                              failing_plus_instabilities_free_test{"Free Test"});

    test_instability_analysis("Failure but no instability",
                              "ConsistentFailureNoInstability",
                              "2",
                              {"--serial"},
                              consistently_failing_free_test{"Free Test"});

    test_instability_analysis("Always passes",
                              "ConsistentSuccessNoInstability",
                              "2",
                              {"--serial"},
                              consistently_passing_free_test<0>{"Free Test"});

    test_instability_analysis("Critical failure instability",
                              "CriticalFailureInstability",
                              "2",
                              {"--serial"},
                              critical_free_test{"Free Test"});

    test_instability_analysis("Two tests always passing",
                              "ConsistentSuccessTwoTests",
                              "2",
                              {"--serial"},
                              consistently_passing_free_test<0>{"Free Test 0"},
                              consistently_passing_free_test<1>{"Free Test 1"});

    test_instability_analysis("Consistent success/consistent failure/instability",
                              "MixedBag",
                              "6",
                              {"--serial"},
                              consistently_passing_free_test<0>{"Passing Free Test"},
                              consistently_failing_free_test{"Failing Free Test"},
                              flipper_free_test{"Flipper Free Test"},
                              multi_periodic_free_test("Free Test")
                             );

    test_instability_analysis("Suite selection",
                              "InstabilitySuiteSelection",
                              "2",
                              {"test", "Another Suite"},
                              [](test_runner& r){ r.add_test_suite("Another Suite", flipper_free_test{"Flipper Free Test"}); },
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
      [this,&extraArgs, numRuns](){
         std::vector<std::string> argList{(minimal_fake_path()).generic_string(), "locate-instabilities", std::string{numRuns}};
         argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
         return argList;
      }
    };

    commandline_arguments args{argGenerator()};


    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"},
                       "  ",
                       outputStream};

    runner.add_test_suite(
      "Suite",
      std::forward<Ts>(ts)...
    );

    manipulator(runner);

    runner.execute();

    const auto outputDir{working_materials() /= outputDirName};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    check(equivalence, reporter(append_lines(message, make_type_info<Ts...>())),
                      outputDir,
                      predictive_materials() /= outputDirName);
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
