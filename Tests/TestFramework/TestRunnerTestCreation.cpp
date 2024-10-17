////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TestRunnerTestCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/TestCreator.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_runner_test_creation::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::string test_runner_test_creation::zeroth_arg(std::string_view projectName) const
  {
    return (working_materials() / projectName / "build/CMade").generic_string();
  }

  void test_runner_test_creation::run_tests()
  {
    test_type_handling();
    test_template_data_generation();
    test_creation("FakeProject", std::nullopt);
    test_creation("AnotherFakeProject", "curlew");
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

  void test_runner_test_creation::test_creation(std::string_view projectName, std::optional<std::string> sourceFolder)
  {
    namespace fs = std::filesystem;

    const auto projectPath{working_materials() / projectName};
    const source_paths sourcePaths{projectPath, sourceFolder};
    const auto sourceFolderPath{sourcePaths.project()};
    const auto sourceFolderName{back(sourceFolderPath).generic_string()};

    fs::copy(auxiliary_paths::repo(project_root()), auxiliary_paths::repo(projectPath), fs::copy_options::recursive);
    fs::create_directory(projectPath / "TestSandbox");

    fs::copy(source_paths{auxiliary_paths::project_template(project_root())}.cmake_lists(), sourceFolderPath);

    const main_paths templateMain{auxiliary_paths::project_template(project_root()) / main_paths::default_main_cpp_from_root()},
                     fakeMain{projectPath / "TestSandbox" / "TestSandbox.cpp"};

    fs::copy(templateMain.file(), fakeMain.file());
    fs::copy(templateMain.cmake_lists(), fakeMain.cmake_lists());
    read_modify_write(fakeMain.cmake_lists(), [](std::string& text) { replace_all(text, "TestAllMain.cpp", "TestSandbox.cpp"); } );

    commandline_arguments args{{zeroth_arg(projectName)
                               , "create", "regular_test", "other::functional::maybe<class T>", "std::optional<T>"
                               , "create", "regular", "utilities::iterator", "int*"
                               , "create", "regular_test", "stuff::widget", "std::vector<int>", "gen-source", "Stuff"
                               , "create", "regular_test", "maths::probability", "double", "g", "Maths"
                               , "create", "regular_test", "maths::angle", "long double", "gen-source", "Maths"
                               , "create", "regular_test", "human", "std::string", "g", "hominins"
                               , "create", "regular_test", "stuff::thingummy<class T>", "std::vector<T>", "g", "Thingummies"
                               , "create", "regular_test", "container<class T>", "const std::vector<T>"
                               , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                              "-s", "partners", "-h", "Couple.hpp"
                               , "create", "regular_test", "bar::things", "double", "-h", std::format("{}/Stuff/Things.hpp", sourceFolderName)
                               , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T", "--suite", "Iterator"
                               , "create", "move_only", "variadic<class... T>", "std::tuple<T...>"
                               , "create", "move_only_test", "multiple<class... T>", "std::tuple<T...>", "gen-source", "Utilities"
                               , "create", "move_only_test", "cloud", "double", "gen-source", "Weather"
                               , "create", "free_test", "Utilities.h"
                               , "create", "free_test", std::format("Source/{}/Stuff/Baz.h", sourceFolderName), "--forename", "bazzer"
                               , "create", "free_test", std::format("Source/{}/Stuff/Baz.h", sourceFolderName), "--forename", "bazagain", "--suite", "Bazzer"
                               , "create", "free_test", "Stuff/Doohicky.hpp", "gen-source", "bar::things"
                               , "create", "free_test", "Global/Stuff/Global.hpp", "gen-source", "::"
                               , "create", "free_test", "Global/Stuff/Defs.hpp", "gen-source", ""
                               , "create", "free", std::format("{}/Maths/Angle.hpp", sourceFolderName), "--diagnostics"
                               , "create", "regular_allocation_test", "container"
                               , "create", "move_only_allocation_test", "foo", "--suite", "Iterator"
                               , "create", "performance_test", "Container.hpp"
                               , "create", "performance_test", "Container.hpp"}
    };

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver Jacob Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp", sourceFolder}, "    ", outputStream};

    tr.execute();

    if(std::ofstream file{projectPath / "output" / "io.txt"})
    {
      file << outputStream.str();
    }

    check(equivalence, "", projectPath, predictive_materials() /= projectName);
  }

  void test_runner_test_creation::test_creation_failure()
  {
      check_exception_thrown<std::runtime_error>(
        reporter{"Plurgh.h does not exist"},
        [this]() {
          std::stringstream outputStream{};
          commandline_arguments args{{zeroth_arg("FakeProject"), "create", "free", "Plurgh.h"}};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
          tr.execute();
        });

      check_exception_thrown<std::runtime_error>(
        reporter{"Typo in specified class header"},
        [this]() {
          std::stringstream outputStream{};
          commandline_arguments args{{zeroth_arg("FakeProject"), "create", "regular_test", "bar::things", "double", "-h", "fakeProject/Stuff/Thingz.hpp"}};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
        });
  }
}
