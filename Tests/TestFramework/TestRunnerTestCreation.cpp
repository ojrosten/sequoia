////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTestCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_test_creation::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_test_creation::run_tests()
  {
    test_template_data_generation();
    test_creation();
    test_creation_failure();
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

    check_equality(LINE("Specialization"), generate_template_data("<>"), template_data{{}});
    check_equality(LINE("Class template parameter"),
                   generate_template_data("<class T>"), template_data{{"class", "T"}});
    check_equality(LINE("Class template parameter"),
                   generate_template_data("<class T >"), template_data{{"class", "T"}});
    check_equality(LINE("Class template parameter"),
                   generate_template_data("< class T>"), template_data{{"class", "T"}});

    check_equality(LINE("Two template parameters"),
                   generate_template_data("<class T, typename S>"),
                   template_data{{"class", "T"}, {"typename", "S"}});
    check_equality(LINE("Two template parameters"),
                   generate_template_data("< class  T,  typename S >"),
                   template_data{{"class", "T"}, {"typename", "S"}});

    check_equality(LINE("Variadic template"),
                   generate_template_data("<class... T>"), template_data{{"class...", "T"}});

    check_equality(LINE("Variadic template"),
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
    fs::copy(project_template_path(root) / "Source/CMakeLists.txt", working() / "Source");
    fs::copy(project_template_path(root) / "TestAll/CMakeLists.txt", working() / "TestSandbox");
    read_modify_write(working() / "TestSandbox" / "CMakeLists.txt" , [](std::string& text) {
        replace_all(text, "TestMain.cpp", "TestSandbox.cpp");
      }
    );

    const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};

    const project_paths repos{working()};

    commandline_arguments args{"", "create", "regular_test", "other::functional::maybe<class T>", "std::optional<T>"
                                 , "create", "regular", "utilities::iterator", "int*"
                                 , "create", "regular_test", "stuff::widget", "std::vector<int>", "gen-source", "Stuff"
                                 , "create", "regular_test", "stuff::thingummy<class T>", "std::vector<T>", "gen-source", "Thingummies"
                                 , "create", "regular_test", "container<class T>", "std::vector<T>"
                                 , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                                "-f", "partners", "-ch", "Couple.hpp"
                                 , "create", "regular_test", "bar::things", "double", "-ch", "fakeProject/Stuff/Things.hpp"
                                 , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T", "--family", "Iterator"
                                 , "create", "move_only", "variadic<class... T>", "std::tuple<T...>"
                                 , "create", "move_only_test", "multiple<class... T>", "std::tuple<T...>", "gen-source", "Utilities"
                                 , "create", "free_test", "Utilities.h"
                                 , "create", "free_test", "Source/fakeProject/Stuff/Baz.h", "--forename", "bazzer"
                                 , "create", "free_test", "Stuff/Doohicky.hpp", "gen-source", "bar::things"
                                 , "create", "free_test", "Global/Stuff/Global.hpp", "gen-source", "::"
                                 , "create", "free_test", "Global/Stuff/Defs.hpp", "gen-source", ""
                                 , "create", "regular_allocation_test", "container"
                                 , "create", "move_only_allocation_test", "foo", "--family", "Iterator"
                                 , "create", "performance_test", "Container.hpp"
                                 , "create", "performance_test", "Container.hpp"
    };

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    tr.execute();

    if(std::ofstream file{working() / "output" / "io.txt"})
    {
      file << outputStream.str();
    }

    check_equivalence(LINE(""), working(), predictive_materials() / "FakeProject");
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
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "", includeTarget, project_paths{working()}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Test Main does not exist"),
      [working]() {
        std::stringstream outputStream{};
        const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "FooMain.cpp", includeTarget, project_paths{working()}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Include Target has empty path"),
      [working]() {
        const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
        std::stringstream outputStream{};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, "", project_paths{working()}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      LINE("Include Target does not exist"),
      [working]() {
        const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
        std::stringstream outputStream{};
        commandline_arguments args{"", "create", "free", "Plurgh.h"};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, "FooPath.hpp", project_paths{working()}, outputStream};
      });

     check_exception_thrown<std::runtime_error>(
       LINE("Project root is empty"),
       [working]() {
         const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
         const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
         std::stringstream outputStream{};
         commandline_arguments args{"", "create", "free", "Plurgh.h"};
         test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, project_paths{""}, outputStream};
       });

     check_exception_thrown<std::runtime_error>(
       LINE("Project root does not exist"),
       [working]() {
         const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
         const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
         std::stringstream outputStream{};
         commandline_arguments args{"", "create", "free", "Plurgh.h"};
         test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, project_paths{working() / "FooRepo"}, outputStream};
       }, pathTrimmer);

     check_exception_thrown<std::runtime_error>(
       LINE("Project root not a directory"),
       [working]() {
         const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
         const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
         std::stringstream outputStream{};
         commandline_arguments args{"", "create", "free", "Plurgh.h"};
         test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, project_paths{includeTarget}, outputStream};
       }, pathTrimmer);

      check_exception_thrown<std::runtime_error>(
        LINE("Plurgh.h does not exist"),
        [working]() {
          const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
          const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
          std::stringstream outputStream{};
          commandline_arguments args{"", "create", "free", "Plurgh.h"};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, project_paths{working()}, outputStream};
          tr.execute();
        });

      check_exception_thrown<std::runtime_error>(
        LINE("Typo in specified class header"),
        [working]() {
          const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
          const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
          std::stringstream outputStream{};
          commandline_arguments args{"", "create", "regular_test", "bar::things", "double", "-ch", "fakeProject/Stuff/Thingz.hpp"};
          test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, project_paths{working()}, outputStream};
        });
  }
}
