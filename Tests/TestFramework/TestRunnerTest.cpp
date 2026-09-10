////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTest.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

import std;

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct foo
    {
      int x{};
    };

    /** These doubles have no source file of their own, so they mint one. Keying it on the test's
        class rather than on its display name keeps the two from drifting apart, and spares the
        space-to-underscore substitution a class name never needs.
     */

    template<concrete_test T>
    [[nodiscard]]
    fs::path make_fake_file_path(std::string_view group = "") {
      return fs::path{std::source_location::current().file_name()}.parent_path().parent_path() / group / (std::string{test_name<T>()} + ".cpp");
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
  
  namespace
  {
    class foo_test final : public regular_test
    {
    public:
      using regular_test::regular_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<foo_test>();
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<passing_test>();
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<failing_test>("Failing");
      }

      void run_tests()
      {
        check(equality, {"Standard Failure"}, 43, 42);
      }
    };

    class throwing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<throwing_test>();
      }

      void run_tests()
      {
        check_exception_thrown<std::runtime_error>("Exception", [](){ throw std::runtime_error{"Oops"}; });
      }
    };

    class platform_specific_throwing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<platform_specific_throwing_test>();
      }

      [[nodiscard]]
      std::string output_discriminator() const { return "Platypus"; }

      [[nodiscard]]
      std::string summary_discriminator() const { return "Release"; }

      void run_tests()
      {
        check_exception_thrown<std::runtime_error>("Another Exception", [](){ throw std::runtime_error{"Oh Dear"}; });
      }
    };

    class failing_fp_test final : public free_false_negative_test
    {
    public:
      using free_false_negative_test::free_false_negative_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<failing_fp_test>("Failing");
      }

      void run_tests()
      {
        check(equality, {"False positive failure"}, 42, 42);
      }
    };

    class failing_fn_test final : public free_false_positive_test
    {
    public:
      using free_false_positive_test::free_false_positive_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<failing_fn_test>("Failing");
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<flipper_free_test>();
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<periodic_free_test>();
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<multi_periodic_free_test>();
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<failing_plus_instabilities_free_test>();
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
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<consistently_failing_free_test>();
      }

      void run_tests()
      {
        check({"Always fails"}, false);
      }
    };

    /** Two classes rather than one template: the pair exists to be registered together, and a
        test's name - and so its output path - now comes from its class.
     */

    class consistently_passing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<consistently_passing_free_test>();
      }

      void run_tests()
      {
        check({"Always passes"}, true);
      }
    };

    class another_consistently_passing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<another_consistently_passing_free_test>();
      }

      void run_tests()
      {
        check({"Always passes"}, true);
      }
    };

    class fake_performance_test final : public performance_test
    {
    public:
      using performance_test::performance_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<fake_performance_test>();
      }

      void run_tests()
      {
        check(equality, "Performance", 42, 42);
      }
    };

    class critical_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<critical_free_test>();
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
                         "  ",
                         {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                         outputStream};

      runner.register_test<failing_test>();
      runner.register_test<failing_fp_test>();
      runner.register_test<failing_fn_test>();

      return runner;
    }
  }
  
  [[nodiscard]]
  std::filesystem::path test_runner_test::source_file()
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
    test_throwing_tests();
    test_filtered_suites();
    test_prune_basic_output();
    test_nested_suite();
    test_nested_suite_verbose();
    test_excluded_performance_tests();
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
    return fake_project().append("build/CMade/FakeExe.txt");
  }

  [[nodiscard]]
  std::string test_runner_test::zeroth_arg() const
  {
    return (minimal_fake_path()).generic_string();
  }

  void test_runner_test::write(std::string_view dirName, std::stringstream& output) const
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);

    if(const auto filePath{outputDir / "io.txt"}; std::ofstream file{filePath})
    {
      file << output.str();
    }

    output.str("");
  }

  void test_runner_test::check_output(reporter description, std::string_view dirName, std::stringstream& output)
  {
    write(dirName, output);
    check(equivalence, description, working_materials() /= dirName, predictive_materials() /= dirName);
  }

  void test_runner_test::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(
      reporter{"Test Main has empty path"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{""}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Test Main does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"FooMain.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Include Target has empty path"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{""}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Include Target does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"FooPath.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root is empty"},
      []() {
        std::stringstream outputStream{};
        commandline_arguments args{{""}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{(fake_project() / "FooRepo").generic_string()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      },
      [](const project_paths& projPaths, std::string message){
        constexpr auto npos{std::string::npos};
        if(const auto pos{message.find(projPaths.project_root().generic_string())}; pos < npos)
        {
          const auto start{pos + projPaths.project_root().generic_string().size()};
          if(const auto end{message.find_first_of("\"]", start)}; end < npos)
            message = "canonical - file not found: " + message.substr(start, end - start);
        }

        return message;
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root not findable"},
      [this]() {
        const auto zerothArg{fake_project().append("TestShared").generic_string()};
        std::stringstream outputStream{};
        commandline_arguments args{{zerothArg}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::logic_error>(
      reporter{"The same test registered twice"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};
  
        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<foo_test>();
        runner.register_test<foo_test>();
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Invalid repetitions for instability analysis"},
      [this](){
        test_instability_analysis("", "", "foo", return_code::critical_failures, critical_free_test{});
      }
    );

    check_exception_thrown<std::runtime_error>(
      reporter{"Insufficient repetitions for instability analysis"},
      [this](){
        test_instability_analysis("", "",  "1", return_code::critical_failures, critical_free_test{});
      }
    );
  }

  void test_runner_test::test_critical_errors()
  {
    std::stringstream outputStream{};

    // This is scoped to ensure destruction of the runner - and therefore loggers -
    // before dumping output to a file. The destructors are not trivial in recovery mode.
    {
      commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v", "recover", "dump"}};
  
      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         "  ",
                         {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                         outputStream};

      runner.register_test<bar_free_test>();
      runner.register_test<foo_test>();

      check(equality, "Recovery and dump return code", runner.execute(), return_code::critical_failures);
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
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    check(equality, "No tests return code", runner.execute(), return_code::success);
    check_output("No Tests", "NoTests", outputStream);

    runner.register_test<failing_test>();
    runner.register_test<failing_fp_test>();
    runner.register_test<failing_fn_test>();

    check(equality, "Basic output return code", runner.execute(), return_code::soft_failures);
    check_output("Basic Output", "BasicOutput", outputStream);
  }

  void test_runner_test::test_verbose_output()
  {
    std::stringstream outputStream{};
    auto runner{make_failing_suite({{(minimal_fake_path()).generic_string(), "-v"}}, outputStream)};

    check(equality, "Verbose output return code", runner.execute(), return_code::soft_failures);
    check_output("Basic Verbose Output", "BasicVerboseOutput", outputStream);
  }

  void test_runner_test::test_serial_verbose_output()
  {
    std::stringstream outputStream{};
    auto runner{make_failing_suite({{(minimal_fake_path()).generic_string(), "-v", "--serial"}}, outputStream)};

    check(equality, "Serial verbose output return code", runner.execute(), return_code::soft_failures);
    check_output("Basic Serial Verbose Output", "BasicSerialVerboseOutput", outputStream);
  }

  void test_runner_test::test_throwing_tests()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    runner.register_test<throwing_test>();
    runner.register_test<platform_specific_throwing_test>();

    check(equality, "Throwing tests return code", runner.execute(), return_code::success);
    check_output("Throwing Output", "ThrowingOutput", outputStream);

    const fs::path diagnosticsDir{working_materials() /= "ThrowingDiagnostics"};
    fs::create_directory(diagnosticsDir);
    fs::copy(fake_project() / "output/DiagnosticsOutput/Tests", diagnosticsDir);

    check(equivalence, "Exception Output", predictive_materials() / "ThrowingDiagnostics", diagnosticsDir);
  }

  void test_runner_test::test_filtered_suites()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "test", "Failing"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    runner.register_test<passing_test>();

    runner.register_test<failing_test>();
    runner.register_test<failing_fp_test>();
    runner.register_test<failing_fn_test>();

    check(equality, "Filtered suites return code", runner.execute(), return_code::soft_failures);
    check_output("Filtered Suite Output", "FilteredSuiteOutput", outputStream);
  }

  void test_runner_test::test_prune_basic_output()
  {
    fs::remove_all(output_paths{fake_project()}.dir());

    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    check(equality, "Prune with no stamp return code", runner.execute(), return_code::success);
    check_output("Prune with no stamp", "PruneWithNoStamp", outputStream);

    check(equality, "Prune with no tests return code", runner.execute(), return_code::success);
    check_output("Prune with no tests", "PruneWithNoTests", outputStream);
  }

  void test_runner_test::test_nested_suite()
  {
      std::stringstream outputStream{};
      commandline_arguments args{{(minimal_fake_path()).generic_string()}};

      test_runner runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         "  ",
                         {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                         outputStream};

      using namespace object;

      runner.register_test<failing_test>();
      runner.register_test<failing_fp_test>();
      runner.register_test<failing_fn_test>();

      check(equality, "Nested suite return code", runner.execute(), return_code::soft_failures);
      check_output("Basic Nested Output", "BasicNestedOutput", outputStream);
  }

  void test_runner_test::test_nested_suite_verbose()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    using namespace object;

    runner.register_test<failing_test>();
    runner.register_test<failing_fp_test>();
    runner.register_test<failing_fn_test>();

    check(equality, "Nested suite verbose return code", runner.execute(), return_code::soft_failures);
    check_output("Verbose Nested Output", "VerboseNestedOutput", outputStream);
  }

  void test_runner_test::test_excluded_performance_tests()
  {
    auto run{
      [this](std::string_view description, std::string_view outputDirName, std::initializer_list<std::string_view> extraArgs){
        std::stringstream outputStream{};

        std::vector<std::string> argList{(minimal_fake_path()).generic_string()};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<passing_test>();
        runner.register_test<fake_performance_test>();

        check(equality, append_lines(description, "Return code"), runner.execute(), return_code::success);
        check_output(description, outputDirName, outputStream);
      }
    };

    // The same registrations both ways, so the option is the only thing which differs.
    run("Performance tests included", "IncludedPerformanceOutput", {});
    run("Performance tests excluded", "ExcludedPerformanceOutput", {"--exclude-performance"});
  }

  void test_runner_test::test_instability_analysis()
  {
    test_instability_analysis("Instability comprising pass/failure",
                              "BinaryInstabilityAnalysis",
                              "2",
                              return_code::soft_failures,
                              {"--serial"},
                              flipper_free_test{});

    test_instability_analysis("Instability comprising pass/multiple distinct failures",
                              "MultiInstabilityAnalysis",
                              "4",
                              return_code::soft_failures,
                              {"--serial"},
                              periodic_free_test{});

    test_instability_analysis("Instability comprising failures from two checks",
                              "MultiCheckInstabilityAnalysis",
                              "6",
                              return_code::soft_failures,
                              {"--serial"},
                              multi_periodic_free_test{});

    test_instability_analysis("Instability following consistent failure",
                              "BinaryInstabilityFollowingFailures",
                              "2",
                              return_code::soft_failures,
                              {"--serial"},
                              failing_plus_instabilities_free_test{});

    test_instability_analysis("Failure but no instability",
                              "ConsistentFailureNoInstability",
                              "2",
                              return_code::soft_failures,
                              {"--serial"},
                              consistently_failing_free_test{});

    test_instability_analysis("Always passes",
                              "ConsistentSuccessNoInstability",
                              "2",
                              return_code::success,
                              {"--serial"},
                              consistently_passing_free_test{});

    test_instability_analysis("Critical failure instability",
                              "CriticalFailureInstability",
                              "2",
                              return_code::critical_failures,
                              {"--serial"},
                              critical_free_test{});

    test_instability_analysis("Two tests always passing",
                              "ConsistentSuccessTwoTests",
                              "2",
                              return_code::success,
                              {"--serial"},
                              consistently_passing_free_test{},
                              another_consistently_passing_free_test{});

    test_instability_analysis("Consistent success/consistent failure/instability",
                              "MixedBag",
                              "6",
                              return_code::soft_failures,
                              {"--serial"},
                              consistently_passing_free_test{},
                              consistently_failing_free_test{},
                              flipper_free_test{},
                              multi_periodic_free_test{}
                             );
  }

  template<std::invocable<test_runner&> Manipulator, concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   return_code expected,
                                                   std::initializer_list<std::string_view> extraArgs,
                                                   Manipulator manipulator,
                                                   Ts&&...)
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
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    (runner.register_test<std::remove_cvref_t<Ts>>(), ...);

    manipulator(runner);

    check(equality, reporter{append_lines(message, "Return code")}, runner.execute(), expected);

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
                                                   return_code expected,
                                                   std::initializer_list<std::string_view> extraArgs,
                                                   Ts&&... ts)
  {
    test_instability_analysis(message, outputDirName, numRuns, expected, extraArgs, [](test_runner&){}, std::forward<Ts>(ts)...);
  }

  template<concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   return_code expected,
                                                   Ts&&... ts)
  {
    test_instability_analysis(message, outputDirName, numRuns, expected, {}, [](test_runner&){}, std::forward<Ts>(ts)...);
  }
}
