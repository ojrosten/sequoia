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
  std::filesystem::path test_runner_test_creation::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  std::string test_runner_test_creation::zeroth_arg() const
  {
    return (fake_project() / "build/CMade").generic_string();
  }

  void test_runner_test_creation::run_tests()
  {
    test_type_handling();
    test_template_data_generation();
    test_creation();
    test_creation_failure();
  }

  void test_runner_test_creation::test_type_handling()
  {
    check_exception_thrown<std::logic_error>(report_line("Empty string"), []() { return handle_as_ref(""); });
    check_exception_thrown<std::logic_error>(report_line("Just spaces"), []() { return handle_as_ref(" "); });
    check(report_line("Letter"),        handle_as_ref("a"));
    check(report_line("int"),          !handle_as_ref("int"));
    check(report_line(" int"),         !handle_as_ref(" int"));
    check(report_line("  int"),        !handle_as_ref("  int"));
    check(report_line("int*"),         !handle_as_ref("int*"));
    check(report_line("int&"),         !handle_as_ref("int&"));
    check(report_line("int *"),        !handle_as_ref("int *"));
    check(report_line(" int "),        !handle_as_ref(" int "));
    check(report_line("long"),         !handle_as_ref("long"));
    check(report_line("longint"),      handle_as_ref("longint"));
    check(report_line("long int"),     !handle_as_ref("long int"));
    check(report_line("double"),       !handle_as_ref("double"));
    check(report_line("std::size_t"),  !handle_as_ref("std::size_t"));
    check(report_line("tuple<int>"),    handle_as_ref("tuple<int>"));
    check(report_line("tuple<int >"),   handle_as_ref("tuple<int >"));
    check(report_line("tuple< int >"),  handle_as_ref("tuple< int >"));
  }

  void test_runner_test_creation::test_template_data_generation()
  {
    check(report_line(""), generate_template_data("").empty());
    check_exception_thrown<std::runtime_error>(report_line("Unmatched <"),
                                               [](){ return generate_template_data("<"); });
    check_exception_thrown<std::runtime_error>(report_line("Backwards delimiters"),
                                               [](){ return generate_template_data("><"); });
    check_exception_thrown<std::runtime_error>(report_line("Missing symbol"),
                                               [](){ return generate_template_data("<class>"); });
    check_exception_thrown<std::runtime_error>(report_line("Missing symbol"),
                                               [](){ return generate_template_data("< class>"); });

    check(equality, report_line("Specialization"), generate_template_data("<>"), template_data{{}});
    check(equality, report_line("Class template parameter"),
                   generate_template_data("<class T>"), template_data{{"class", "T"}});
    check(equality, report_line("Class template parameter"),
                   generate_template_data("<class T >"), template_data{{"class", "T"}});
    check(equality, report_line("Class template parameter"),
                   generate_template_data("< class T>"), template_data{{"class", "T"}});

    check(equality, report_line("Two template parameters"),
                   generate_template_data("<class T, typename S>"),
                   template_data{{"class", "T"}, {"typename", "S"}});
    check(equality, report_line("Two template parameters"),
                   generate_template_data("< class  T,  typename S >"),
                   template_data{{"class", "T"}, {"typename", "S"}});

    check(equality, report_line("Variadic template"),
                   generate_template_data("<class... T>"), template_data{{"class...", "T"}});

    check(equality, report_line("Variadic template"),
                   generate_template_data("<class ... T>"), template_data{{"class ...", "T"}});
  }

  void test_runner_test_creation::test_creation()
  {
    namespace fs = std::filesystem;

    fs::copy(auxiliary_paths::repo(project_root()), auxiliary_paths::repo(fake_project()), fs::copy_options::recursive);
    fs::create_directory(fake_project() / "TestSandbox");

    fs::copy(source_paths{auxiliary_paths::project_template(project_root())}.cmake_lists(), source_paths{fake_project()}.project());

    const main_paths templateMain{auxiliary_paths::project_template(project_root()) / main_paths::default_main_cpp_from_root()},
                     fakeMain{fake_project() / "TestSandbox" / "TestSandbox.cpp"};

    fs::copy(templateMain.file(), fakeMain.file());
    fs::copy(templateMain.cmake_lists(), fakeMain.cmake_lists());
    read_modify_write(fakeMain.cmake_lists(), [](std::string& text) { replace_all(text, "TestAllMain.cpp", "TestSandbox.cpp"); } );

    commandline_arguments args{  zeroth_arg()
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
                               , "create", "regular_test", "bar::things", "double", "-h", "fakeProject/Stuff/Things.hpp"
                               , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T", "--suite", "Iterator"
                               , "create", "move_only", "variadic<class... T>", "std::tuple<T...>"
                               , "create", "move_only_test", "multiple<class... T>", "std::tuple<T...>", "gen-source", "Utilities"
                               , "create", "move_only_test", "cloud", "double", "gen-source", "Weather"
                               , "create", "free_test", "Utilities.h"
                               , "create", "free_test", "Source/fakeProject/Stuff/Baz.h", "--forename", "bazzer"
                               , "create", "free_test", "Source/fakeProject/Stuff/Baz.h", "--forename", "bazagain", "--suite", "Bazzer"
                               , "create", "free_test", "Stuff/Doohicky.hpp", "gen-source", "bar::things"
                               , "create", "free_test", "Global/Stuff/Global.hpp", "gen-source", "::"
                               , "create", "free_test", "Global/Stuff/Defs.hpp", "gen-source", ""
                               , "create", "free", "fakeProject/Maths/Angle.hpp", "--diagnostics"
                               , "create", "regular_allocation_test", "container"
                               , "create", "move_only_allocation_test", "foo", "--suite", "Iterator"
                               , "create", "performance_test", "Container.hpp"
                               , "create", "performance_test", "Container.hpp"
    };

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver Jacob Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "    ", outputStream};

    tr.execute();

    if(std::ofstream file{fake_project() / "output" / "io.txt"})
    {
      file << outputStream.str();
    }

    check(equivalence, report_line(""), fake_project(), predictive_materials() /= "FakeProject");
  }

  void test_runner_test_creation::test_creation_failure()
  {
      check_exception_thrown<std::runtime_error>(
        report_line("Plurgh.h does not exist"),
        [this]() {
          std::stringstream outputStream{};
          commandline_arguments args{zeroth_arg(), "create", "free", "Plurgh.h"};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
          tr.execute();
        });

      check_exception_thrown<std::runtime_error>(
        report_line("Typo in specified class header"),
        [this]() {
          std::stringstream outputStream{};
          commandline_arguments args{zeroth_arg(), "create", "regular_test", "bar::things", "double", "-h", "fakeProject/Stuff/Thingz.hpp"};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {"TestSandbox/TestSandbox.cpp", {}, "TestShared/SharedIncludes.hpp"}, "  ", outputStream};
        });
  }
}
