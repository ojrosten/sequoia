////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTestCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

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

    const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};

    const repositories repos{working()};

    commandline_arguments args{"", "create", "regular_test", "other::functional::maybe<class T>", "std::optional<T>"
                                 , "create", "regular", "utilities::iterator", "int*"
                                 , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T", "--family", "Iterator"
                                 , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                                "-f", "partners", "-ch", "Couple.hpp"
                                 , "create", "regular_test", "bar::things", "double", "-ch", "fakeProject/Stuff/Things.hpp"
                                 , "create", "free_test", "Utilities.h"
                                 , "create", "free_test", "Source/fakeProject/Stuff/Baz.h", "--forename", "bazzer"
                                 , "create", "move_only_allocation_test", "foo", "--family", "Iterator"
                                 , "create", "regular_test", "container<class T>", "std::vector<T>"
                                 , "create", "regular_allocation_test", "container"
                                 , "create", "performance_test", "Container.hpp"
                                 , "create", "performance_test", "Container.hpp"
                                 , "create" , "move_only", "variadic<class... T>", "std::tuple<T...>"
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

    check_exception_thrown<std::runtime_error>(LINE(""),
                                               [working](){
                                                 std::stringstream outputStream{};
                                                 const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
                                                 commandline_arguments args{"", "create", "free", "Plurgh.h"};
                                                 test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "", includeTarget, repositories{working()}, outputStream};
                                               });

    check_exception_thrown<std::runtime_error>(LINE(""),
                                               [working](){
                                                 const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
                                                 std::stringstream outputStream{};
                                                 commandline_arguments args{"", "create", "free", "Plurgh.h"};
                                                 test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, "", repositories{working()}, outputStream};
                                               });

     check_exception_thrown<std::runtime_error>(LINE(""),
                                               [working](){
                                                 const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
                                                 const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
                                                 std::stringstream outputStream{};
                                                 commandline_arguments args{"", "create", "free", "Plurgh.h"};
                                                 test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repositories{""}, outputStream};
                                               });

      check_exception_thrown<std::runtime_error>(LINE(""),
                                               [working](){
                                                 const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
                                                 const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};
                                                 std::stringstream outputStream{};
                                                 commandline_arguments args{"", "create", "free", "Plurgh.h"};
                                                 test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repositories{working()}, outputStream};
                                                 tr.execute();
                                               });
  }
}
