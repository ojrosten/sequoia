////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTest.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "CommandLineArgumentsTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_test::run_tests()
  {
    test_template_data_generation();
    test_creation();
  }

  void test_runner_test::test_template_data_generation()
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

  void test_runner_test::test_creation()
  {
    namespace fs = std::filesystem;

    auto working{
      [&mat{working_materials()}]() {
        return mat / "FakeProject";
      }
    };
    
    const auto testMain{working().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{working().append("TestShared").append("SharedIncludes.hpp")};

    const repositories repos{working()};

    std::stringstream outputStream{};
    commandline_arguments args{"", "create", "regular_test", "other::functional::maybe<class T>", "std::optional<T>"
                                 , "create", "regular_test", "utilities::iterator", "int*"
                                 , "create", "move_only_test", "bar::baz::foo<maths::floating_point T>", "T", "--family", "iterator"
                                 , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                      "-h", (repos.tests / "Partners").string(), "-f", "partners", "-ch", "Couple.hpp"};
    
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    tr.execute();

    check_equivalence(LINE(""), working(), predictive_materials() / "FakeProject");
  }
}
