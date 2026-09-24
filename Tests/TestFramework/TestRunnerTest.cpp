////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTest.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"
#include "TestFramework/BuildArtefactsTestingUtilities.hpp"

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
      static std::string output_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      static std::string summary_discriminator(const cmake_cache&) { return "Release"; }

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

    /// The next two put a suite and a test which are siblings under one name: `namesake_test`
    /// names the test, and the directory holding `under_namesake_test` beside it. The test sorts
    /// first, so its node exists by the time the suite of that name is wanted.
    class namesake_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<namesake_test>("Namesakes");
      }

      void run_tests()
      {
        check(equality, reporter{"Namesake"}, 42, 42);
      }
    };

    class under_namesake_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<under_namesake_test>("Namesakes/namesake_test");
      }

      void run_tests()
      {
        check(equality, reporter{"Under the namesake"}, 42, 42);
      }
    };

    namespace another_namespace
    {
      /// Holds a `foo_test` of its own. `test_name` strips the qualification from both, and a
      /// test's name is the leaf of its materials path, so the runner must admit only one.
      class foo_test final : public free_test
      {
      public:
        using free_test::free_test;

        [[nodiscard]]
        static std::filesystem::path source_file()
        {
          return make_fake_file_path<foo_test>();
        }

        void run_tests() {}
      };
    }

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
    test_help_output();
    test_verbose_output();
    test_serial_verbose_output();
    test_throwing_tests();
    test_filtered_suites();
    test_prune_basic_output();
    test_prune_with_changed_toolchain();
    test_prune_selects_a_test_this_executable_lacks();
    test_post_run_failure();
    test_nested_suite();
    test_nested_suite_verbose();
    test_suite_named_as_a_sibling_test();
    test_excluded_performance_tests();
    test_excluded_tests();
    test_excluded_tests_are_rerun();
    test_dump_comparison();
    test_instability_analysis();
    test_exit_statuses();
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

    check_exception_thrown<std::logic_error>(
      reporter{"Two tests whose unqualified names coincide"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<foo_test>();
        runner.register_test<another_namespace::foo_test>();
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

  void test_runner_test::test_help_output()
  {
    const std::array<std::pair<std::vector<std::string>, std::string_view>, 4> requests{{
      {{"init"},                   "InitHelpOutput"         },
      {{"create"},                 "CreateHelpOutput"       },
      {{"create", "regular_test"}, "CreateRegularHelpOutput"},
      {{"test"},                   "TestHelpOutput"         }
    }};

    // Failing tests are registered so that a run which went ahead would show in the return code
    for(const auto& [commands, dirName] : requests)
    {
      std::vector<std::string> argList{zeroth_arg()};
      argList.append_range(commands);
      argList.push_back("--help");

      std::stringstream outputStream{};
      auto runner{make_failing_suite(argList, outputStream)};

      check(equality, std::format("{} return code", dirName), runner.execute(), return_code::success);
      check_output(std::format("{} output", dirName), dirName, outputStream);
    }
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

    // An exclusion does not turn prune off, and one matching nothing is reported under prune
    fs::remove_all(output_paths{fake_project()}.dir());

    std::stringstream excludingStream{};
    commandline_arguments excludingArgs{{minimal_fake_path().generic_string(), "prune", "exclude", "Failing/absent_test.cpp"}};

    test_runner excludingRunner{excludingArgs.size(),
                                excludingArgs.get(),
                                "Oliver J. Rosten",
                                "  ",
                                {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                                excludingStream};

    check(equality, "Prune with an exclusion return code", excludingRunner.execute(), return_code::success);
    check_output("Prune with an exclusion", "PruneWithExclusionOutput", excludingStream);
  }

  [[nodiscard]]
  test_runner_test::fake_build test_runner_test::write_fake_build()
  {
    fs::remove_all(output_paths{fake_project()}.dir());

    // A build of one test's source, Tests/ThingTest.cpp, which read one toolchain header
    const auto buildDir{minimal_fake_path().parent_path()};
    const fake_build build{.source{fake_project() / "Tests" / "ThingTest.cpp"}, .toolchainHeader{fake_project() / "Toolchain" / "vector"}};
    fs::create_directories(build.toolchainHeader.parent_path());
    fs::create_directories(buildDir / "CMakeFiles" / "4.1.2");
    write_to_file(build.source, "", std::ios_base::out);
    write_to_file(build.toolchainHeader, "", std::ios_base::out);
    write_to_file(buildDir / "CMakeFiles" / "4.1.2" / "CMakeCXXCompiler.cmake",
                  std::format("set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"{}\")\n", build.toolchainHeader.parent_path().generic_string()),
                  std::ios_base::out);
    write_to_file(buildDir / "build.ninja",
                  std::format("build CMakeFiles/x.dir/ThingTest.cpp.o: CXX_COMPILER {}\n", build.source.generic_string()),
                  std::ios_base::out);
    write_ninja_deps(buildDir / ".ninja_deps",
                     std::vector<compilation_record>{{"CMakeFiles/x.dir/ThingTest.cpp.o", {build.source, build.toolchainHeader}}});

    return build;
  }

  void test_runner_test::test_prune_with_changed_toolchain()
  {
    // The toolchain header modified after the previous run's stamp
    const auto build{write_fake_build()};

    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};
    const project_paths projPaths{args.size(), args.get(), {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}};
    const auto stamp{projPaths.prune().stamp()};
    fs::create_directories(stamp.parent_path());
    write_to_file(stamp, "", std::ios_base::out);

    // Both files the build read are stamped strictly before the executable: where last_write_time
    // resolves to whole seconds, a file written in the same second as the executable is out of date
    using namespace std::chrono_literals;
    const auto now{std::chrono::file_clock::now()};
    fs::last_write_time(stamp, now - 2s);
    fs::last_write_time(build.source, now - 1s);
    fs::last_write_time(build.toolchainHeader, now - 1s);
    fs::last_write_time(projPaths.executable(), now);

    std::stringstream outputStream{};
    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    check(equality, "Prune with changed toolchain return code", runner.execute(), return_code::success);
    check_output("Prune with changed toolchain", "PruneWithChangedToolchain", outputStream);
  }

  void test_runner_test::test_prune_selects_a_test_this_executable_lacks()
  {
    // The source modified after the previous run's stamp, the toolchain before it: prune selects
    // ThingTest.cpp, which no test registered here defines, and that is not a selection to report
    const auto build{write_fake_build()};

    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};
    const project_paths::customizer customization{
      .main_cpp{"TestSandbox/TestSandbox.cpp"},
      .common_includes{"TestShared/SharedIncludes.hpp"}
    };
    const project_paths projPaths{args.size(), args.get(), customization};
    const auto stamp{projPaths.prune().stamp()};
    fs::create_directories(stamp.parent_path());
    write_to_file(stamp, "", std::ios_base::out);

    using namespace std::chrono_literals;
    const auto now{std::chrono::file_clock::now()};
    fs::last_write_time(build.toolchainHeader, now - 3s);
    fs::last_write_time(stamp, now - 2s);
    fs::last_write_time(build.source, now - 1s);
    fs::last_write_time(projPaths.executable(), now);

    std::stringstream outputStream{};
    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    runner.register_test<passing_test>();
    check(equality, "Prune selecting an unregistered test return code", runner.execute(), return_code::success);
    check_output("Prune selecting an unregistered test", "PruneSelectsUnregisteredOutput", outputStream);
  }

  void test_runner_test::test_post_run_failure()
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

    // A filtered run merges its results into the previous failures, so a malformed record there fails the prune write
    const auto failuresFile{runner.proj_paths().prune().to_rerun(std::nullopt)};
    fs::create_directories(failuresFile.parent_path());
    const transient_file malformedFailures{failuresFile, "garbage\n"};

    check(equality, "Post-run failure return code", runner.execute(), return_code::soft_failures | return_code::post_run_failures);
    check_output("Post-Run Failure Output", "PostRunFailureOutput", outputStream);
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

  void test_runner_test::test_suite_named_as_a_sibling_test()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    runner.register_test<namesake_test>();
    runner.register_test<under_namesake_test>();

    check(equality, "Suite named as a sibling test return code", runner.execute(), return_code::success);
    check_output("Suite Named As A Sibling Test", "SuiteNamedAsASiblingTest", outputStream);
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

  void test_runner_test::test_excluded_tests()
  {
    auto run{
      [this](std::string_view description, std::string_view outputDirName, std::initializer_list<std::string> extraArgs, return_code expected){
        std::stringstream outputStream{};

        std::vector<std::string> argList{minimal_fake_path().generic_string()};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<passing_test>();
        runner.register_test<failing_test>();
        runner.register_test<fake_performance_test>();

        check(equality, append_lines(description, "Return code"), runner.execute(), expected);
        check_output(description, outputDirName, outputStream);
      }
    };

    // The same registrations each way, so the arguments are the only thing which differs; the
    // failing test's return code says whether it ran
    const auto failing{failing_test::source_file().generic_string()};
    const auto performance{fake_performance_test::source_file().generic_string()};

    run("Failing test excluded",                          "ExcludedTestOutput",
        {"exclude", failing},                              return_code::success);
    run("Exclusion matching no test",                     "ExclusionNotFoundOutput",
        {"exclude", "Failing/absent_test.cpp"},            return_code::soft_failures);
    run("Exclusion naming a suite",                       "ExclusionOfSuiteOutput",
        {"exclude", "Failing"},                            return_code::soft_failures);
    run("Selected test excluded, and so found both ways", "SelectedTestExcludedOutput",
        {"select", failing, "exclude", failing},           return_code::success);
    run("Performance test excluded both ways, and found", "PerformanceTestExcludedOutput",
        {"--exclude-performance", "exclude", performance}, return_code::soft_failures);
    run("Exclusion by alias",                             "ExcludedTestOutput",
        {"e", failing},                                    return_code::success);
  }

  void test_runner_test::test_excluded_tests_are_rerun()
  {
    // A run with no selection is full: it stamps, and whatever it left out is recorded for the
    // next run, since that test's status is unknown
    auto run{
      [this](std::string_view description, std::initializer_list<std::string> extraArgs, std::vector<std::filesystem::path> toRerun) {
        fs::remove_all(output_paths{fake_project()}.dir());

        std::vector<std::string> argList{minimal_fake_path().generic_string()};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        std::stringstream outputStream{};
        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<passing_test>();
        runner.register_test<fake_performance_test>();

        const auto prunePaths{runner.proj_paths().prune()};
        check(equality, append_lines(description, "Return code"), runner.execute(), return_code::success);
        check(equality, append_lines(description, "Stamped"),     fs::exists(prunePaths.stamp()), true);

        // The file holds paths relative to the project, so the names are compared
        auto name{[](const prune_record& r){ return r.test_path.filename(); }};
        const auto recorded{read_tests(prunePaths.to_rerun(std::nullopt)) | std::views::transform(name) | std::ranges::to<std::vector>()};
        check(equality, append_lines(description, "To rerun"), recorded, toRerun);
      }
    };

    const auto performance{fake_performance_test::source_file()};

    run("A full run",                        {},                                        {});
    run("A run excluding a test",            {"exclude", performance.generic_string()}, {performance.filename()});
    run("A run excluding performance tests", {"--exclude-performance"},                 {performance.filename()});
  }

  void test_runner_test::test_dump_comparison()
  {
    enum class registrations { passing_and_failing, passing, passing_failing_and_performance };

    auto run{
      [this](std::string_view description,
             std::string_view outputDirName,
             std::initializer_list<std::string> extraArgs,
             registrations registered,
             return_code expected) {
        std::vector<std::string> argList{minimal_fake_path().generic_string(), "dump"};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        std::stringstream outputStream{};
        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<passing_test>();
        if(registered != registrations::passing)
          runner.register_test<failing_test>();

        if(registered == registrations::passing_failing_and_performance)
          runner.register_test<fake_performance_test>();

        check(equality, append_lines(description, "Return code"), runner.execute(), expected);
        check_output(description, outputDirName, outputStream);
      }
    };

    auto failing{
      [this](std::string_view description, std::initializer_list<std::string> extraArgs) {
        check_exception_thrown<std::runtime_error>(description, [this, extraArgs](){
          std::vector<std::string> argList{minimal_fake_path().generic_string(), "dump"};
          argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
          commandline_arguments args{argList};

          std::stringstream outputStream{};
          test_runner runner{args.size(),
                             args.get(),
                             "Oliver J. Rosten",
                             "  ",
                             {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                             outputStream};

          runner.register_test<passing_test>();
          return runner.execute();
        });
      }
    };

    fs::remove_all(output_paths{fake_project()}.dir());
    const auto recovery{output_paths{fake_project()}.recovery()};

    run("A dump kept under a name", "DumpKeptOutput", {"--as", "before"}, registrations::passing_and_failing, return_code::soft_failures);
    check(equality, "The kept dump exists", fs::exists(recovery.kept_dump("before")), true);

    run("The same checks as the kept dump",    "DumpUnchangedOutput",
        {"--against", "before"}, registrations::passing_and_failing,             return_code::soft_failures);
    run("A check missing since the kept dump", "DumpMissingOutput",
        {"--against", "before"}, registrations::passing,                         return_code::success);
    run("A check added since the kept dump",   "DumpAddedOutput",
        {"--against", "before"}, registrations::passing_failing_and_performance, return_code::soft_failures);

    // Compared before kept, so one run may take the name it compared against
    run("A dump compared against a name and then kept under it", "DumpAddedOutput",
        {"--against", "before", "--as", "before"}, registrations::passing_failing_and_performance, return_code::soft_failures);
    run("The kept dump is the later one", "DumpUnchangedThreeOutput",
        {"--against", "before"}, registrations::passing_failing_and_performance, return_code::soft_failures);

    failing("Comparison against a dump never kept", {"--against", "never"});
    failing("A dump kept under no name",            {"--as", ""});
    failing("Comparison against no name",           {"--against", ""});

    // recover, like dump, writes under output/Recovery on a tree which has no output directory yet
    fs::remove_all(output_paths{fake_project()}.dir());
    std::stringstream recoveringStream{};
    commandline_arguments recoveringArgs{{minimal_fake_path().generic_string(), "recover"}};
    test_runner recoveringRunner{recoveringArgs.size(),
                                 recoveringArgs.get(),
                                 "Oliver J. Rosten",
                                 "  ",
                                 {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                                 recoveringStream};

    recoveringRunner.register_test<passing_test>();
    check(equality, "recover on a fresh tree", recoveringRunner.execute(), return_code::success);
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

  void test_runner_test::test_exit_statuses()
  {
    // Every combination of the five flags, in order of their underlying values.
    constexpr int numCodes{32};
    const auto codes{
      std::views::iota(0, numCodes) | std::views::transform([](int i){ return static_cast<return_code>(i); })
    };

    const auto statuses{codes | std::views::transform(to_exit_code) | std::ranges::to<std::vector>()};
    const auto expectedStatuses{
      std::views::iota(0, numCodes)
        | std::views::transform([](int i){ return i == 0 ? 0 : runner_exit_offset + i; })
        | std::ranges::to<std::vector>()
    };

    check(equality,
          "The runner exits with 0 for success, and with the offset plus the flags otherwise",
          statuses,
          expectedStatuses);

    auto decode{[](int status){ return static_cast<int>(std::to_underlying(child_return_code(status))); }};
    const auto decoded{statuses | std::views::transform(decode) | std::ranges::to<std::vector>()};

    check(equality,
          "Each exit status decodes to the code it encodes",
          decoded,
          std::views::iota(0, numCodes) | std::ranges::to<std::vector>());

    check(equality,
          "A child exiting 88 reports an incomplete run",
          child_return_code(runner_exit_offset + std::to_underlying(return_code::incomplete_run)),
          return_code::incomplete_run);

    // Statuses a process gives when it fails for reasons of its own, and the two just outside the runner's range.
    // Each is worded identically on every platform, being positive and no greater than 128.
    for(const int status : {1, 2, runner_exit_offset, runner_exit_offset + numCodes, 126, 127})
    {
      check_exception_thrown<std::runtime_error>(
        std::format("Exit status {} is not a runner's", status),
        [status](){ return child_return_code(status); });
    }
  }
}
