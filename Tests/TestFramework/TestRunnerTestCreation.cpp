////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTestCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/TestCreator.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <array>
#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path test_runner_test_creation::source_file()
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

  void test_runner_test_creation::test_creation(std::string_view projectName,
                                                std::optional<std::string> sourceFolder,
                                                main_location mainLocation)
  {
    const auto projectPath{auxiliary_materials() / projectName};
    const source_paths sourcePaths{projectPath, sourceFolder};
    const auto sourceFolderPath{sourcePaths.project()};
    const auto sourceFolderName{back(sourceFolderPath).generic_string()};

    fs::copy(auxiliary_paths::repo(get_project_paths().project_root()), auxiliary_paths::repo(projectPath), fs::copy_options::recursive);

    fs::copy(source_paths{auxiliary_paths::project_template(get_project_paths().project_root())}.cmake_lists(), sourceFolderPath);

    fs::copy(get_project_paths().build_system().repo(), projectPath / "dependencies/sequoia/build_system", fs::copy_options::recursive);

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

    test_foreign_source_dir_refusal(projectName, sourceFolder, cmakeCacheDir / "CMakeCache.txt", fakeMain);
    record_cmake_source_dir(cmakeCacheDir / "CMakeCache.txt", cmakeSourceDir);
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
