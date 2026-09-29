////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTestCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"

#include "sequoia/TestFramework/TestCreator.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <array>
#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace lifecycle
  {
    /** \brief Stand-ins for tests `create` writes into the fake project, which `remove-test` removes only once they
        are registered. Each names its source as the fake project's tests repository holds it.
     */
    class utilities_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Utilities/UtilitiesFreeTest.cpp"; }

      void run_tests() {}
    };

    class utilities_free_test_extras final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Utilities/UtilitiesFreeTestExtras.cpp"; }

      void run_tests() {}
    };

    class widget_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Stuff/WidgetTest.cpp"; }

      void run_tests() {}
    };

    class widget_false_negative_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Stuff/WidgetTestingDiagnostics.cpp"; }

      void run_tests() {}
    };

    /** \brief The directories of a project which creating or removing a test may change. */
    constexpr std::array<std::string_view, 7>
      editedDirectories{"OtherSandbox", "output", "Source", "TestMaterials", "Tests", "TestSandbox", "TestShared"};

    /** \brief A test class sharing its source file with `pair_second_test`. */
    class pair_first_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Stuff/PairTest.cpp"; }

      void run_tests() {}
    };

    class pair_second_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Stuff/PairTest.cpp"; }

      void run_tests() {}
    };

    /** \brief Two tests whose sources share a name, in different directories. */
    class twin_stuff_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Stuff/TwinTest.cpp"; }

      void run_tests() {}
    };

    class twin_utilities_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Utilities/TwinTest.cpp"; }

      void run_tests() {}
    };

    /** \brief A test registered with the runner which the fake project's main does not register. */
    class unlisted_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file() { return "Tests/Stuff/UnlistedTest.cpp"; }

      void run_tests() {}
    };

    /** \brief A test whose source lies outside the fake project, as in a checkout copied with its build tree: a file,
        which does not exist, beside this one.
     */
    class stray_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return fs::path{std::source_location::current().file_name()}.parent_path() / "StrayTest.cpp";
      }

      void run_tests() {}
    };

    constexpr std::string_view projectName{"LifecycleProject"};

    [[nodiscard]]
    std::vector<std::string> with_zeroth_arg(std::string zerothArg, std::vector<std::string> args)
    {
      args.insert(args.begin(), std::move(zerothArg));
      return args;
    }

    [[nodiscard]]
    std::vector<std::string> removal_args(const std::vector<std::string>& tests)
    {
      auto removalOf{[](const std::string& test) { return std::array<std::string, 2>{"remove-test", test}; }};

      return tests | std::views::transform(removalOf) | std::views::join | std::ranges::to<std::vector>();
    }

    /** \brief A runner for the fake project; creation runs as from an executable naming the other as ancillary,
        removal as from one which does not.
     */
    [[nodiscard]]
    test_runner runner_for(commandline_arguments& args,
                           std::vector<fs::path> ancillaryMains,
                           std::ostream& stream)
    {
      return test_runner{args.size(),
                         args.get(),
                         "Oliver Jacob Rosten",
                         "    ",
                         {.source_folder{"fakeProject"},
                          .main_cpp{"TestSandbox/TestSandbox.cpp"},
                          .ancillary_main_cpps{std::move(ancillaryMains)},
                          .common_includes{"TestShared/SharedIncludes.hpp"}},
                         stream};
    }

    /** \brief Gives `project` the state of `snapshot`, in every directory creating or removing a test may change. */
    void set_state(const fs::path& project, const fs::path& snapshot)
    {
      fs::create_directories(project);
      for(const auto dir : editedDirectories)
      {
        fs::remove_all(project / dir);
        if(fs::exists(snapshot / dir))
          fs::copy(snapshot / dir, project / dir, fs::copy_options::recursive);
      }
    }
  }

  [[nodiscard]]
  fs::path test_runner_test_creation::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::string test_runner_test_creation::zeroth_arg(std::string_view projectName) const
  {
    return (auxiliary_materials() / projectName / "build"/ back(get_project_paths().build().cmake_cache_dir()) / "FakeExe.txt").generic_string();
  }

  void test_runner_test_creation::run_tests()
  {
    test_type_handling();
    test_project_namespace();
    test_template_data_generation();
    test_creation_and_removal();
    test_removal_refusals();
    test_creation("FakeProject", std::nullopt, main_location::in_source_dir);
    test_creation("AnotherFakeProject", "curlew", main_location::below_source_dir);
    test_creation_failure();
  }

  void test_runner_test_creation::test_type_handling()
  {
    check_exception_thrown<std::logic_error>("Empty string", []() { return handle_as_ref(""); });
    check_exception_thrown<std::logic_error>("Just spaces", []() { return handle_as_ref(" "); });
    check("Letter",        handle_as_ref("a"));
    check("int",          !handle_as_ref("int"));
    check(" int",         !handle_as_ref(" int"));
    check("  int",        !handle_as_ref("  int"));
    check("int*",         !handle_as_ref("int*"));
    check("int&",         !handle_as_ref("int&"));
    check("int *",        !handle_as_ref("int *"));
    check(" int ",        !handle_as_ref(" int "));
    check("long",         !handle_as_ref("long"));
    check("longint",      handle_as_ref("longint"));
    check("long int",     !handle_as_ref("long int"));
    check("double",       !handle_as_ref("double"));
    check("std::size_t",  !handle_as_ref("std::size_t"));
    check("tuple<int>",    handle_as_ref("tuple<int>"));
    check("tuple<int >",   handle_as_ref("tuple<int >"));
    check("tuple< int >",  handle_as_ref("tuple< int >"));
  }

  void test_runner_test_creation::test_project_namespace()
  {
    using namespace std::string_literals;

    const auto root{auxiliary_materials() /= "Namespaces"};
    fs::create_directories(root / "myProject");
    fs::create_directories(root / "my-project");
    fs::create_directories(root / "0project");

    check(equality, "A directory whose name is an identifier", project_namespace_for(root / "myProject"), "myProject"s);

    check_exception_thrown<std::runtime_error>(
      "A directory which does not exist",
      [&root]() { return project_namespace_for(root / "absent"); }
    );

    check_exception_thrown<std::runtime_error>(
      "A directory whose name is not an identifier",
      [&root]() { return project_namespace_for(root / "my-project"); }
    );

    check_exception_thrown<std::runtime_error>(
      "A directory whose name begins with a digit",
      [&root]() { return project_namespace_for(root / "0project"); }
    );
  }

  void test_runner_test_creation::test_template_data_generation()
  {
    check("", generate_template_data("").empty());
    check_exception_thrown<std::runtime_error>("Unmatched <",
                                               [](){ return generate_template_data("<"); });
    check_exception_thrown<std::runtime_error>("Backwards delimiters",
                                               [](){ return generate_template_data("><"); });
    check_exception_thrown<std::runtime_error>("Missing symbol",
                                               [](){ return generate_template_data("<class>"); });
    check_exception_thrown<std::runtime_error>("Missing symbol",
                                               [](){ return generate_template_data("< class>"); });

    check(equality, "Specialization", generate_template_data("<>"), template_data{{}});
    check(equality, "Class template parameter",
                   generate_template_data("<class T>"), template_data{{"class", "T"}});
    check(equality, "Class template parameter",
                   generate_template_data("<class T >"), template_data{{"class", "T"}});
    check(equality, "Class template parameter",
                   generate_template_data("< class T>"), template_data{{"class", "T"}});

    check(equality, "Two template parameters",
                   generate_template_data("<class T, typename S>"),
                   template_data{{"class", "T"}, {"typename", "S"}});
    check(equality, "Two template parameters",
                   generate_template_data("< class  T,  typename S >"),
                   template_data{{"class", "T"}, {"typename", "S"}});

    check(equality, "Variadic template",
                   generate_template_data("<class... T>"), template_data{{"class...", "T"}});

    check(equality, "Variadic template",
                   generate_template_data("<class ... T>"), template_data{{"class ...", "T"}});
  }

  /** Each node is a state of a fake project, and each edge a run of `create` or `remove-test` taking it from one state
      to another. So each removal is checked to give back the state before the creation it undoes, and each creation
      to give back the state before the removal. A removed test's companions stay, and so does the source `create`
      generated for it, so creating it again restores it. One edge stands for running the tests, writing their
      versioned output and materials; one test's name begins the other's, and removing it leaves the other's output.
   */
  void test_runner_test_creation::test_creation_and_removal()
  {
    using namespace lifecycle;
    using transition_checker_t = transition_checker<fs::path, check_ordering::no>;
    using project_graph        = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    fs::copy(auxiliary_materials() / "FakeProject", auxiliary_materials() / projectName, fs::copy_options::recursive);
    const auto project{prepare_fake_project(projectName, "fakeProject", main_location::in_source_dir)};

    // As every run of the runner leaves them
    fs::create_directories(project.root / "output" / "DiagnosticsOutput");
    fs::create_directories(project.root / "output" / "TestSummaries");
    fs::create_directories(project.root / "output" / "TestsTemporaryData");
    fs::create_directories(project.root / "TestMaterials");

    // A second executable: creation registers each test in its main too, and removal must find it unprompted, as
    // removing from TestAll must find a test PersistentTestChamber registers.
    fs::copy(project.root / "TestSandbox", project.root / "OtherSandbox", fs::copy_options::recursive);

    // Files naming the tests where no main is - in the sources, the tests, the materials (as another test's fake
    // project may), the output, the dependencies and a hidden directory - which removal leaves be.
    fs::copy(auxiliary_materials() / "LifecycleDecoys", project.root, fs::copy_options::recursive);

    auto creation{
      [this, &project](std::vector<std::string> creationArgs) {
        return [this, &project, creationArgs](const fs::path& state) {
          set_state(project.root, state);

          commandline_arguments args{with_zeroth_arg(zeroth_arg(projectName), creationArgs)};
          std::stringstream stream{};
          [[maybe_unused]] const auto code{runner_for(args, {"OtherSandbox/TestSandbox.cpp"}, stream).execute()};

          return project.root;
        };
      }
    };

    auto removal{
      [this, &project](std::vector<std::string> tests, auto registerTests) {
        return [this, &project, tests, registerTests](const fs::path& state) {
          set_state(project.root, state);

          commandline_arguments args{with_zeroth_arg(zeroth_arg(projectName), removal_args(tests))};
          std::stringstream stream{};
          auto runner{runner_for(args, {}, stream)};
          registerTests(runner);
          [[maybe_unused]] const auto code{runner.execute()};

          return project.root;
        };
      }
    };

    auto running{
      [&project, runOutput{auxiliary_materials() / "LifecycleRunOutput"}](const fs::path& state) {
        set_state(project.root, state);
        fs::copy(runOutput, project.root, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        return project.root;
      }
    };

    auto utilitiesTests{
      [](test_runner& r) {
        r.register_test<utilities_free_test>();
        r.register_test<utilities_free_test_extras>();
      }
    };

    auto extrasTest{[](test_runner& r) { r.register_test<utilities_free_test_extras>(); }};

    auto widgetTests{
      [](test_runner& r) {
        r.register_test<widget_test>();
        r.register_test<widget_false_negative_test>();
      }
    };

    auto widgetTesterTest{[](test_runner& r) { r.register_test<widget_false_negative_test>(); }};

    const std::vector<std::string>
      createExtras        {"create", "free_test", "Utilities.h", "--fullname", "utilities_free_test_extras"},
      createUtilities     {"create", "free_test", "Utilities.h"},
      createExtrasThenUtilities{
        "create", "free_test", "Utilities.h", "--fullname", "utilities_free_test_extras",
        "create", "free_test", "Utilities.h"
      },
      createWidget        {"create", "regular_test", "stuff::widget", "std::vector<int>", "--gen-source", "Stuff"};

    const auto states{predictive_materials() / "Lifecycle"};

    project_graph g{
      { { edge_t{1, "Create two tests, one named as the other plus a suffix",
                 creation(createExtrasThenUtilities)},
          edge_t{4, "Create the test whose name is the longer", creation(createExtras)},
          edge_t{5, "Create a regular test, generating the class", creation(createWidget)}
        }, // 0: prepared
        { edge_t{0, "Remove both, by their classes",
                 removal({"utilities_free_test", "utilities_free_test_extras"}, utilitiesTests)},
          edge_t{2, "Run the tests", running},
          edge_t{4, "Remove the test whose name the other's begins with, by its class",
                 removal({"utilities_free_test"}, utilitiesTests)}
        }, // 1: two tests
        { edge_t{3,
                 "Remove the test whose name the other's begins with, by its source file, with output and materials",
                 removal({"Tests/Utilities/UtilitiesFreeTest.cpp"}, utilitiesTests)}
        }, // 2: two tests, run
        { edge_t{0,
                 "Remove the remaining test, with its output and materials",
                 removal({"utilities_free_test_extras"}, extrasTest)}
        }, // 3: one test, run
        { edge_t{0, "Remove the test", removal({"utilities_free_test_extras"}, extrasTest)},
          edge_t{1, "Create the test whose name is the shorter", creation(createUtilities)}
        }, // 4: one test
        { edge_t{6, "Remove the regular test, leaving its companions", removal({"widget_test"}, widgetTests)},
          edge_t{7, "Remove the regular test and the false-negative test of its testing utilities",
                 removal({"widget_test", "widget_false_negative_test"}, widgetTests)}
        }, // 5: regular test
        { edge_t{5, "Create the regular test again, beside its companions", creation(createWidget)},
          edge_t{7, "Remove the false-negative test", removal({"widget_false_negative_test"}, widgetTesterTest)}
        }, // 6: companions of a regular test
        { edge_t{5, "Create the regular test again, beside its testing utilities", creation(createWidget)}
        }  // 7: testing utilities
      },
      {states / "Prepared",
       states / "UtilitiesTests",
       states / "UtilitiesTestsRun",
       states / "ExtrasRun",
       states / "Extras",
       states / "Widget",
       states / "WidgetCompanions",
       states / "WidgetTestingUtilities"}
    };

    auto checkState{
      [this](std::string_view description, const fs::path& projectRoot, const fs::path& state) {
        check_state(description, projectRoot, state);
      }
    };

    check_state("The fake project as prepared", project.root, states / "Prepared");

    transition_checker_t::check(report("Creation and removal"), g, checkState);

    for(const auto dir : {"dependencies/Decoys", ".hidden"})
    {
      check(equivalence,
            std::format("Decoys in {} left be", dir),
            project.root / dir,
            auxiliary_materials() / "LifecycleDecoys" / dir);
    }
  }

  /** A refusal comes before anything is removed, and a report names what went, so each is checked on the fake project
      the graph leaves behind, set to the state holding a regular test and its companions.
   */
  /** Each refusal is checked on the state holding a regular test and its companions, with whatever else the refusal
      needs registered, so that nothing but the refusal stands between the request and a removal; and each is checked
      to leave the project as it was. The report of a removal is checked last.
   */
  void test_runner_test_creation::test_removal_refusals()
  {
    using namespace lifecycle;

    const auto root{auxiliary_materials() / projectName};
    const auto state{predictive_materials() / "Lifecycle" / "Widget"};
    const auto main{root / "TestSandbox" / "TestSandbox.cpp"};
    const auto mainCMakeLists{root / "TestSandbox" / "CMakeLists.txt"};

    auto removal{
      [this](std::vector<std::string> args, auto registerTests, std::ostream& stream) {
        commandline_arguments cmdArgs{with_zeroth_arg(zeroth_arg(projectName), std::move(args))};
        auto runner{runner_for(cmdArgs, {}, stream)};
        registerTests(runner);
        return runner.execute();
      }
    };

    // Registered in the main, and listed beside it, as `create` writes them
    auto registerInProject{
      [&main, &mainCMakeLists, &root](const std::string& test, const fs::path& source) {
        add_test_registrations(main, {test});
        add_to_cmake(mainCMakeLists,
                     root / "Tests",
                     source.is_absolute() ? source : root / source,
                     "target_sources(",
                     ")\n",
                     "${TestDir}/");
      }
    };

    // The refusal of a test whose source lies outside the project names paths on this machine
    auto withoutRoots{
      [&root](const project_paths& projPaths, std::string message) {
        replace_all(message, root.generic_string(), "<project>");
        replace_all(message, projPaths.project_root().generic_string(), "<repository>");
        return message;
      }
    };

    auto refused{
      [&, this](const reporter& description, std::vector<std::string> args, auto registerTests, auto prepare) {
        set_state(root, state);
        prepare();

        // The same leaf, since the comparison of two directories includes their names
        const auto before{working_materials() / "BeforeRefusal" / projectName},
                   after {working_materials() / "AfterRefusal"  / projectName};
        set_state(before, root);

        auto attempt{
          [&removal, &args, &registerTests]() {
            std::stringstream stream{};
            return removal(args, registerTests, stream);
          }
        };

        check_exception_thrown<std::runtime_error>(description, attempt, withoutRoots);

        set_state(after, root);
        check(equivalence, "The project is as it was before the refusal", after, before);
      }
    };

    auto widgetTests{
      [](test_runner& r) {
        r.register_test<widget_test>();
        r.register_test<widget_false_negative_test>();
      }
    };

    auto pairTests{
      [](test_runner& r) {
        r.register_test<widget_test>();
        r.register_test<pair_first_test>();
        r.register_test<pair_second_test>();
      }
    };

    auto twinTests{
      [](test_runner& r) {
        r.register_test<twin_stuff_test>();
        r.register_test<twin_utilities_test>();
      }
    };

    auto unlistedTest{[](test_runner& r) { r.register_test<unlisted_test>(); }};
    auto strayTest   {[](test_runner& r) { r.register_test<stray_test>(); }};

    auto asCreated{[]() {}};

    refused("A class registering no test", {"remove-test", "gizmo_test"}, widgetTests, asCreated);
    refused("A source file defining no registered test",
            {"remove-test", "Tests/Stuff/GizmoTest.cpp"},
            widgetTests,
            asCreated);
    refused("A test which is registered, beside one which is not",
            {"remove-test", "widget_test", "remove-test", "gizmo_test"},
            widgetTests,
            asCreated);
    refused("A source file named by a name two source files have",
            {"remove-test", "TwinTest.cpp"},
            twinTests,
            [&]() {
              registerInProject("twin_stuff_test", "Tests/Stuff/TwinTest.cpp");
              registerInProject("twin_utilities_test", "Tests/Utilities/TwinTest.cpp");
            });
    refused("A test whose source lies outside the project",
            {"remove-test", "stray_test"},
            strayTest,
            [&]() { registerInProject("stray_test", stray_test::source_file()); });
    refused("A test the main does not register", {"remove-test", "unlisted_test"}, unlistedTest, asCreated);
    refused("A test whose source the CMakeLists.txt beside the main does not list",
            {"remove-test", "widget_test"},
            widgetTests,
            [&]() {
              remove_from_cmake(mainCMakeLists, root / "Tests", root / "Tests/Stuff/WidgetTest.cpp", "${TestDir}/");
            });
    refused("The one test a class names, of the two its source file defines",
            {"remove-test", "pair_first_test"},
            pairTests,
            [&]() {
              write_to_file(root / "Tests/Stuff/PairTest.cpp", "// Two tests\n", std::ios_base::out);
              registerInProject("pair_first_test", "Tests/Stuff/PairTest.cpp");
              registerInProject("pair_second_test", "Tests/Stuff/PairTest.cpp");
            });
    refused("Removal, with a test to run",
            {"remove-test", "widget_test", "select", "WidgetTest.cpp"},
            widgetTests,
            asCreated);
    refused("Removal, with a test to exclude",
            {"remove-test", "widget_test", "exclude", "WidgetTest.cpp"},
            widgetTests,
            asCreated);

    set_state(root, state);

    std::stringstream stream{};
    check(equality,
          "Removal return code",
          removal({"remove-test", "widget_test"}, widgetTests, stream),
          return_code::success);

    const auto report{working_materials() / "RemovalReport"};
    fs::create_directories(report);
    write_to_file(report / "io.txt", stream.str(), std::ios_base::out);
    check(equivalence, "What a removal reports", report, predictive_materials() / "RemovalReport");
  }

  void test_runner_test_creation::check_state(std::string_view description,
                                              const fs::path& projectRoot,
                                              const fs::path& state)
  {
    const auto working{working_materials() / "Lifecycle" / back(state)};
    lifecycle::set_state(working, projectRoot);
    check(equivalence, description, working, state);
  }

  [[nodiscard]]
  test_runner_test_creation::fake_project
    test_runner_test_creation::prepare_fake_project(std::string_view projectName,
                                                    const std::optional<std::string>& sourceFolder,
                                                    main_location mainLocation)
  {
    const auto projectPath{auxiliary_materials() / projectName};
    const auto sourceFolderPath{source_paths{projectPath, sourceFolder}.project()};

    fs::copy(auxiliary_paths::repo(get_project_paths().project_root()),
             auxiliary_paths::repo(projectPath),
             fs::copy_options::recursive);

    fs::copy(source_paths{auxiliary_paths::project_template(get_project_paths().project_root())}.cmake_lists(),
             sourceFolderPath);

    fs::copy(get_project_paths().build_system().repo(),
             projectPath / "dependencies/sequoia/build_system",
             fs::copy_options::recursive);

    const auto cmakeCacheDir{projectPath / "build" / back(get_project_paths().build().cmake_cache_dir())};
    fs::create_directory(cmakeCacheDir);
    fs::copy(auxiliary_materials() / "FakeExe.txt", cmakeCacheDir);
    fs::copy(get_project_paths().build().cmake_cache_dir() / "CMakeCache.txt", cmakeCacheDir);

    // The copied cache records this build's top-level source directory, and `create` runs CMake from
    // the one its cache records, so it must record the fake project's instead.
    const auto cmakeSourceDir{projectPath / "TestSandbox"};
    record_cmake_source_dir(cmakeCacheDir / "CMakeCache.txt", cmakeSourceDir);

    const main_paths templateMain{auxiliary_paths::project_template(get_project_paths().project_root())
                                    / main_paths::default_main_cpp_from_root()};

    const auto fakeMainDir{
      mainLocation == main_location::in_source_dir ? cmakeSourceDir : cmakeSourceDir / "Outer" / "Inner"
    };
    const main_paths fakeMain{fakeMainDir / "TestSandbox.cpp"};

    fs::create_directories(fakeMain.dir());
    fs::copy(templateMain.file(), fakeMain.file());
    fs::copy(templateMain.dir() / "CMakePresets.json", cmakeSourceDir);

    auto cmakeLists{read_to_string(templateMain.cmake_lists(), std::ios_base::in).value()};
    replace_all(cmakeLists, "myProject", sourceFolder ? sourceFolder.value() : uncapitalize(projectName));
    if(mainLocation == main_location::in_source_dir)
    {
      replace_all(cmakeLists, "TestAllMain.cpp", "TestSandbox.cpp");
    }
    else
    {
      // The template's list of test sources, and the lines `--gen-source` uncomments, move beside the
      // main, since `create` edits the CMakeLists.txt there.
      const auto mainDir{fs::relative(fakeMain.dir(), cmakeSourceDir).generic_string()};
      constexpr std::string_view testSources{"target_sources(TestAll PRIVATE)\n"};
      const auto first{cmakeLists.find("#!")}, last{cmakeLists.find(testSources)};
      if((first == std::string::npos) || (last == std::string::npos) || (last < first))
        throw std::logic_error{"The project template's CMakeLists.txt no longer has the shape this fixture splits"};

      const auto count{last + testSources.size() - first};
      write_to_file(fakeMain.cmake_lists(), std::string_view{cmakeLists}.substr(first, count), std::ios_base::out);
      cmakeLists.replace(first, count, std::format("add_subdirectory({})\n", mainDir));
      replace_all(cmakeLists, "TestAllMain.cpp", mainDir + "/TestSandbox.cpp");

      // Presets between the main and the source directory, so that neither the main's parent nor the
      // nearest directory with presets is where CMake runs.
      fs::copy(templateMain.dir() / "CMakePresets.json", fakeMain.dir().parent_path());
    }

    write_to_file(cmakeSourceDir / "CMakeLists.txt", cmakeLists, std::ios_base::out);

    return {.root{projectPath}, .cmake_cache_dir{cmakeCacheDir}, .main{fakeMain}};
  }

  void test_runner_test_creation::test_creation(std::string_view projectName,
                                                std::optional<std::string> sourceFolder,
                                                main_location mainLocation)
  {
    const auto project{prepare_fake_project(projectName, sourceFolder, mainLocation)};
    const auto& projectPath{project.root};
    const auto& fakeMain{project.main};
    const auto sourceFolderName{back(source_paths{projectPath, sourceFolder}.project()).generic_string()};

    commandline_arguments args{{zeroth_arg(projectName)
                               , "create", "regular_test", "other::functional::maybe<class T>", "std::optional<T>"
                               , "create", "regular", "utilities::iterator", "int*"
                               , "create", "regular_test", "stuff::widget", "std::vector<int>", "--gen-source", "Stuff"
                               , "create", "regular_test", "maths::probability", "double", "-g", "Maths"
                               , "create", "regular_test", "maths::angle", "long double", "--gen-source", "Maths"
                               , "create", "regular_test", "human", "std::string", "-g", "hominins"
                               , "create", "regular_test", "stuff::thingummy<class T>", "std::vector<T>", "-g", "Thingummies"
                               , "create", "regular_test", "container<class T>", "const std::vector<T>"
                               , "create", "regular_test", "other::couple<class S, class T>", "std::pair<S, T>",
                                              "--header", "Couple.hpp"
                               , "create", "regular_test", "bar::things", "double", "--header", std::format("{}/Stuff/Things.hpp", sourceFolderName)
                               , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T"
                               , "create", "move_only", "variadic<class... T>", "std::tuple<T...>"
                               , "create", "move_only_test", "multiple<class... T>", "std::tuple<T...>", "--gen-source", "Utilities"
                               , "create", "move_only_test", "cloud", "double", "--gen-source", "Weather"
                               , "create", "free_test", "Utilities.h"
                               , "create", "free_test", std::format("Source/{}/Stuff/Baz.h", sourceFolderName), "--forename", "bazzer"
                               , "create", "free_test", std::format("Source/{}/Stuff/Baz.h", sourceFolderName), "--forename", "bazagain"
                               , "create", "free_test", "Stuff/Doohicky.hpp", "--gen-source", "bar::things"
                               , "create", "free_test", "Global/Stuff/Global.hpp", "--gen-source", "::"
                               , "create", "free_test", "Global/Stuff/Defs.hpp", "--gen-source", ""
                               , "create", "free", std::format("{}/Maths/Angle.hpp", sourceFolderName), "--diagnostics"
                               , "create", "regular_allocation_test", "container"
                               , "create", "move_only_allocation_test", "foo"
                               , "create", "performance_test", "Container.hpp"
                               , "create", "performance_test", "Container.hpp"
                               , "create", "free_test", "Utilities.h", "--fullname", "utility_functions_test"
                               , "create", "regular_test", "maths::angle", "long double",
                                              "--fullname", "angle_regular_test"
                               , "create", "move_only_test", "cloud", "double", "--fullname", "cloud_move_only_test"
                               // Named by a path, normalised, but included from the test's own directory by its name
                               , "create", "regular_test", "maths::probability", "double",
                                              "--fullname", "probability_family_test",
                                              "--testing-utilities", "Stuff/../Maths/ProbabilityTestingUtilities.hpp"
                               // Named bare, but included from another directory by its path beneath Tests
                               , "create", "regular_test", "human", "std::string",
                                              "--fullname", "human_shared_tester_test",
                                              "--testing-utilities", "WidgetTestingUtilities.hpp"
                               // Fresh types, so that every class each creation registers is new
                               , "create", "regular_test", "stuff::gizmo", "int", "-g", "Stuff",
                                              "--fullname", "gizmo_semantics_test"
                               , "create", "move_only_test", "stuff::gadget", "int", "-g", "Stuff",
                                              "--fullname", "gadget_family_test",
                                              "--testing-utilities", "WidgetTestingUtilities.hpp"
                               // Named by its full path
                               , "create", "regular_test", "maths::angle", "long double",
                                              "--fullname", "angle_family_test",
                                              "--testing-utilities",
                                              (projectPath / "Tests/Maths/AngleTestingUtilities.hpp").generic_string()
                               // A test whose name ends like a tester's is still among the common includes
                               , "create", "free_test", "Utilities.h", "--fullname", "string_utilities"
                               , "create", "regular_allocation_test", "container",
                                              "--fullname", "container_family_allocation_test",
                                              "--testing-utilities", "ContainerTestingUtilities.hpp"
                               , "create", "performance_test", "Container.hpp", "--fullname", "container_speed_test"}
    };

    std::stringstream outputStream{};
    test_runner tr{args.size(),
                   args.get(),
                   "Oliver Jacob Rosten",
                   "    ",
                   {.source_folder{sourceFolder},
                    .main_cpp{fs::relative(fakeMain.file(), projectPath).generic_string()},
                    .common_includes{"TestShared/SharedIncludes.hpp"}},
                   outputStream};

    check(equality, "Test creation return code", tr.execute(), return_code::success);

    if(std::ofstream file{projectPath / "output" / "io.txt"})
    {
      file << outputStream.str();
    }

    check_directory(projectName, "output");
    check_directory(projectName, "Source");
    check_directory(projectName, "Tests");
    check_directory(projectName, "TestSandbox");
    check_directory(projectName, "TestShared");

    test_foreign_source_dir_refusal(projectName, sourceFolder, project.cmake_cache_dir / "CMakeCache.txt", fakeMain);
    record_cmake_source_dir(project.cmake_cache_dir / "CMakeCache.txt", projectPath / "TestSandbox");
  }

  void test_runner_test_creation::test_foreign_source_dir_refusal(std::string_view projectName,
                                                                 const std::optional<std::string>& sourceFolder,
                                                                 const std::filesystem::path& cacheFile,
                                                                 const main_paths& fakeMain)
  {
    const auto projectPath{auxiliary_materials() / projectName};

    auto createAgain{
      [&]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg(projectName), "create", "free_test", "Utilities.h"}};
        test_runner tr{args.size(),
                       args.get(),
                       "Oliver Jacob Rosten",
                       "    ",
                       {.source_folder{sourceFolder},
                        .main_cpp{fs::relative(fakeMain.file(), projectPath).generic_string()},
                        .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};
      }
    };

    // The refusal names two paths, and the default postprocessor makes only the first relative.
    auto relativeToRoot{
      [](const project_paths& projPaths, std::string message) {
        replace_all(message, projPaths.project_root().generic_string() + "/", "");
        return message;
      }
    };

    record_cmake_source_dir(cacheFile, projectPath / "Absent");
    check_exception_thrown<std::runtime_error>(reporter{"Source directory absent"}, createAgain, relativeToRoot);

    record_cmake_source_dir(cacheFile, auxiliary_materials());
    check_exception_thrown<std::runtime_error>(reporter{"Source directory outside the project"},
                                               createAgain,
                                               relativeToRoot);
  }

  void test_runner_test_creation::record_cmake_source_dir(const std::filesystem::path& cacheFile,
                                                         const std::filesystem::path& sourceDir)
  {
    read_modify_write(
      cacheFile,
      [&sourceDir](std::string& text) {
        constexpr std::string_view entry{"\nCMAKE_HOME_DIRECTORY:INTERNAL="};
        if(const auto pos{text.find(entry)}; pos != std::string::npos)
        {
          const auto start{pos + entry.size()};
          text.replace(start, text.find_first_of("\r\n", start) - start, sourceDir.generic_string());
        }
      }
    );

    check(equality, "Recorded source directory", cmake_cache{cacheFile}.source_dir(), sourceDir);
  }

  void test_runner_test_creation::test_creation_failure()
  {
    const auto project{auxiliary_materials() / "FakeProject"};

    auto create{
      [this](std::initializer_list<std::string> creationArgs) {
        const auto argList{
          [&]() {
            std::vector<std::string> list{zeroth_arg("FakeProject"), "create"};
            list.append_range(creationArgs);
            return list;
          }()
        };

        std::stringstream outputStream{};
        commandline_arguments args{argList};
        test_runner tr{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};
      }
    };

    auto refused{
      [this, &create](std::string_view description, std::initializer_list<std::string> creationArgs) {
        check_exception_thrown<std::runtime_error>(reporter{description},
                                                   [&create, creationArgs]() { create(creationArgs); });
      }
    };

    refused("Plurgh.h does not exist", {"free", "Plurgh.h"});
    refused("Typo in specified class header",
            {"regular_test", "bar::things", "double", "--header", "fakeProject/Stuff/Thingz.hpp"});

    refused("Both a forename and a full name",
            {"free", "Utilities.h", "--forename", "utils", "--fullname", "utility_functions_test"});
    refused("A full name for a framework-diagnostics pair",
            {"free", "Utilities.h", "--diagnostics", "--fullname", "utilities_diagnostics"});
    refused("An empty full name", {"free", "Utilities.h", "--fullname", ""});
    refused("A full name which is not an identifier", {"free", "Utilities.h", "--fullname", "2nd_utilities_test"});
    refused("A full name already registered", {"free", "Utilities.h", "--fullname", "widget_test"});
    refused("A full name whose file is present, ignoring case",
            {"free", "Stuff/Doohicky.hpp", "--fullname", "widgettest"});

    refused("Testing utilities absent",
            {"regular_test", "bar::things", "double", "--testing-utilities", "AbsentTestingUtilities.hpp"});
    refused("Testing utilities outside the tests repository",
            {"regular_test", "bar::things", "double", "--testing-utilities", "../TestSandbox/TestSandbox.cpp"});

    // A second file of the same name makes a bare name ambiguous; a directory of that name does not.
    fs::copy(project / "Tests/Stuff/WidgetTestingUtilities.hpp", project / "Tests/Utilities");
    refused("Testing utilities ambiguous",
            {"regular_test", "bar::things", "double", "--testing-utilities", "WidgetTestingUtilities.hpp"});

    fs::create_directories(project / "Tests/Decoy/ProbabilityTestingUtilities.hpp");
    refused("Testing utilities found past a directory of their name, then a full name already registered",
            {"regular_test", "stuff::widget", "std::vector<int>",
             "--testing-utilities", "ProbabilityTestingUtilities.hpp",
             "--fullname", "widget_test"});

    // A refusal comes before anything is written: here the type is new, so every file would be too.
    refused("A full name whose file is a companion's",
            {"regular_test", "stuff::sprocket", "int", "-g", "Stuff", "--fullname", "sprocket_testing_utilities"});

    auto namesSprocket{
      [](const fs::directory_entry& entry) { return entry.path().filename().string().contains("Sprocket"); }
    };

    check("No file written for a refused test",
          std::ranges::none_of(fs::recursive_directory_iterator{project}, namesSprocket));

    auto mentionsSprocket{
      [&project](std::string_view file) {
        const auto text{read_to_string(project / file, std::ios_base::in).value()};
        return text.contains("sprocket") || text.contains("Sprocket");
      }
    };

    check("No registration for a refused test",
          std::ranges::none_of(std::array<std::string_view, 3>{"TestSandbox/TestSandbox.cpp",
                                                               "TestSandbox/CMakeLists.txt",
                                                               "TestShared/SharedIncludes.hpp"},
                               mentionsSprocket));
  }

  void test_runner_test_creation::check_directory(std::string_view projectName, std::string_view dirName)
  {
    const auto targetDir{(working_materials() /= projectName) /= dirName};
    fs::create_directories(targetDir);
    fs::copy((auxiliary_materials() /= projectName) /= dirName, targetDir, fs::copy_options::recursive);
    check(equivalence, "", targetDir, (predictive_materials() /= projectName) /= dirName);
  }
}
