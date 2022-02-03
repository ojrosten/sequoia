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
  std::string_view test_runner_test_creation::source_file() const noexcept
  {
    return __FILE__;
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
    check_exception_thrown<std::logic_error>(LINE("Empty string"), []() { return handle_as_ref(""); });
    check_exception_thrown<std::logic_error>(LINE("Just spaces"), []() { return handle_as_ref(" "); });
    check(LINE("Letter"),        handle_as_ref("a"));
    check(LINE("int"),          !handle_as_ref("int"));
    check(LINE(" int"),         !handle_as_ref(" int"));
    check(LINE("  int"),        !handle_as_ref("  int"));
    check(LINE("int*"),         !handle_as_ref("int*"));
    check(LINE("int&"),         !handle_as_ref("int&"));
    check(LINE("int *"),        !handle_as_ref("int *"));
    check(LINE(" int "),        !handle_as_ref(" int "));
    check(LINE("long"),         !handle_as_ref("long"));
    check(LINE("longint"),      handle_as_ref("longint"));
    check(LINE("long int"),     !handle_as_ref("long int"));
    check(LINE("double"),       !handle_as_ref("double"));
    check(LINE("std::size_t"),  !handle_as_ref("std::size_t"));
    check(LINE("tuple<int>"),    handle_as_ref("tuple<int>"));
    check(LINE("tuple<int >"),   handle_as_ref("tuple<int >"));
    check(LINE("tuple< int >"),  handle_as_ref("tuple< int >"));
  }

  void test_runner_test_creation::test_template_data_generation()
  {
    check(LINE(""), generate_template_data("").empty());
    check_exception_thrown<std::runtime_error>(LINE("Unmatched <"),
                                               [](){ return generate_template_data("<"); });
    check_exception_thrown<std::runtime_error>(LINE("Backwards delimiters"),
                                               [](){ return generate_template_data("><"); });
    check_exception_thrown<std::runtime_error>(LINE("Missing symbol"),
                                               [](){ return generate_template_data("<class>"); });
    check_exception_thrown<std::runtime_error>(LINE("Missing symbol"),
                                               [](){ return generate_template_data("< class>"); });

    check(equality, LINE("Specialization"), generate_template_data("<>"), template_data{{}});
    check(equality, LINE("Class template parameter"),
                   generate_template_data("<class T>"), template_data{{"class", "T"}});
    check(equality, LINE("Class template parameter"),
                   generate_template_data("<class T >"), template_data{{"class", "T"}});
    check(equality, LINE("Class template parameter"),
                   generate_template_data("< class T>"), template_data{{"class", "T"}});

    check(equality, LINE("Two template parameters"),
                   generate_template_data("<class T, typename S>"),
                   template_data{{"class", "T"}, {"typename", "S"}});
    check(equality, LINE("Two template parameters"),
                   generate_template_data("< class  T,  typename S >"),
                   template_data{{"class", "T"}, {"typename", "S"}});

    check(equality, LINE("Variadic template"),
                   generate_template_data("<class... T>"), template_data{{"class...", "T"}});

    check(equality, LINE("Variadic template"),
                   generate_template_data("<class ... T>"), template_data{{"class ...", "T"}});
  }

  void test_runner_test_creation::test_creation()
  {
    auto working{
      [&mat{working_materials()}]() { return mat / "FakeProject"; }
    };

    namespace fs = std::filesystem;

    const auto root{test_repository().parent_path()};
    fs::copy(aux_files_path(root), aux_files_path(working()), fs::copy_options::recursive);
    fs::create_directory(working() / "TestSandbox");
    fs::copy(project_template_path(root) / "Source" / "CMakeLists.txt", working() / "Source");
    fs::copy(project_template_path(root) / "TestAll" / "CMakeLists.txt", working() / "TestSandbox");
    fs::copy(project_template_path(root) / "TestAll"/ "TestAllMain.cpp", working() / "TestSandbox" / "TestSandbox.cpp");
    read_modify_write(working() / "TestSandbox" / "CMakeLists.txt" , [](std::string& text) {
        replace_all(text, "TestAllMain.cpp", "TestSandbox.cpp");
      }
    );

    const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};

    const project_paths paths{working(), testMain, includeTarget};

    commandline_arguments args{"", "create", "regular_test", "other::functional::maybe<class T>", "std::optional<T>"
                                 , "create", "regular", "utilities::iterator", "int*"
                                 , "create", "regular_test", "stuff::widget", "std::vector<int>", "gen-source", "Stuff"
                                 , "create", "regular_test", "maths::probability", "double", "g", "Maths"
                                 , "create", "regular_test", "maths::angle", "long double", "gen-source", "Maths"
                                 , "create", "regular_test", "stuff::thingummy<class T>", "std::vector<T>", "g", "Thingummies"
                                 , "create", "regular_test", "container<class T>", "const std::vector<T>"
                                 , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                                "-f", "partners", "-h", "Couple.hpp"
                                 , "create", "regular_test", "bar::things", "double", "-h", "fakeProject/Stuff/Things.hpp"
                                 , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T", "--family", "Iterator"
                                 , "create", "move_only", "variadic<class... T>", "std::tuple<T...>"
                                 , "create", "move_only_test", "multiple<class... T>", "std::tuple<T...>", "gen-source", "Utilities"
                                 , "create", "free_test", "Utilities.h"
                                 , "create", "free_test", "Source/fakeProject/Stuff/Baz.h", "--forename", "bazzer"
                                 , "create", "free_test", "Source/fakeProject/Stuff/Baz.h", "--forename", "bazagain", "--family", "Bazzer"
                                 , "create", "free_test", "Stuff/Doohicky.hpp", "gen-source", "bar::things"
                                 , "create", "free_test", "Global/Stuff/Global.hpp", "gen-source", "::"
                                 , "create", "free_test", "Global/Stuff/Defs.hpp", "gen-source", ""
                                 , "create", "regular_allocation_test", "container"
                                 , "create", "move_only_allocation_test", "foo", "--family", "Iterator"
                                 , "create", "performance_test", "Container.hpp"
                                 , "create", "performance_test", "Container.hpp"
    };

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver Jacob Rosten", paths, "    ", outputStream};

    tr.execute();

    if(std::ofstream file{working() / "output" / "io.txt"})
    {
      file << outputStream.str();
    }

    check(equivalence, LINE(""), working(), predictive_materials() / "FakeProject");
  }

  void test_runner_test_creation::test_creation_failure()
  {
    auto working{
      [&mat{working_materials()}]() { return mat / "FakeProject"; }
    };

    auto pathTrimmer{
      [](std::string mess) {
        const auto pos{mess.find("output/")};
        if(pos < mess.size()) mess.erase(0, pos);

        return mess;
      }
    };

    check_exception_thrown<std::runtime_error>(
      LINE("Test Main has empty path"),
      [working]() {
        std::stringstream outputStream{};
        const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working(), "", includeTarget}, "  ", outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Test Main does not exist"),
      [working]() {
        std::stringstream outputStream{};
        const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working(), "FooMain.cpp", includeTarget}, "  ", outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Include Target has empty path"),
      [working]() {
        const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
        std::stringstream outputStream{};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working(), testMain, ""}, "  ", outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Include Target does not exist"),
      [working]() {
        const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
        std::stringstream outputStream{};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working(), testMain, "FooPath.hpp"}, "  ", outputStream};
      });

     check_exception_thrown<std::runtime_error>(
       LINE("Project root is empty"),
       [working]() {
         const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
         const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
         std::stringstream outputStream{};
         commandline_arguments args{"", "create", "free", "Plurgh.h"};
         test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{"", testMain, includeTarget}, "  ", outputStream};
       });

     check_exception_thrown<std::runtime_error>(
       LINE("Project root does not exist"),
       [working]() {
         const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
         const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
         std::stringstream outputStream{};
         commandline_arguments args{"", "create", "free", "Plurgh.h"};
         test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working() / "FooRepo", testMain, includeTarget}, "  ", outputStream};
       }, pathTrimmer);

     check_exception_thrown<std::runtime_error>(
       LINE("Project root not a directory"),
       [working]() {
         const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
         const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
         std::stringstream outputStream{};
         commandline_arguments args{"", "create", "free", "Plurgh.h"};
         test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{includeTarget, testMain, includeTarget}, "  ", outputStream};
       }, pathTrimmer);

      check_exception_thrown<std::runtime_error>(
        LINE("Plurgh.h does not exist"),
        [working]() {
          const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
          const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
          std::stringstream outputStream{};
          commandline_arguments args{"", "create", "free", "Plurgh.h"};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working(), testMain, includeTarget}, "  ", outputStream};
          tr.execute();
        });

      check_exception_thrown<std::runtime_error>(
        LINE("Typo in specified class header"),
        [working]() {
          const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
          const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
          std::stringstream outputStream{};
          commandline_arguments args{"", "create", "regular_test", "bar::things", "double", "-h", "fakeProject/Stuff/Thingz.hpp"};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", project_paths{working(), testMain, includeTarget}, "  ", outputStream};
        });
  }
}
