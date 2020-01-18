////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestRunnerTest.hpp"

#include "UnitTestRunner.hpp"

namespace sequoia::unit_testing
{
  void unit_test_runner_test::run_tests()
  {
    test_parser();
  }

  void unit_test_runner_test::test_parser()
  {
    using ops = std::vector<commandline_operation>;
    using info = std::map<std::string, commandline_option_info>;
    using fo = function_object;
    
    {
      check_weak_equivalence(LINE(""), parse(0, nullptr, info{}), ops{});
    }

    {
      commandline_arguments<4, 8> a{"foo", "--async"};
      
      check_weak_equivalence(LINE(""), parse(2, a.get(), info{{"--async", {fo{}}}}), ops{{fo{}}});
    }

    {
      commandline_arguments<4, 3> a{"foo", "-a"};
      
      check_weak_equivalence(LINE(""), parse(2, a.get(), info{{"--async", {fo{}, {}, {"-a"}}}}), ops{{fo{}}});
    }

    {
      commandline_arguments<4, 3> a{"foo", "-a"};
      
      check_weak_equivalence(LINE(""), parse(2, a.get(), info{{"--async", {fo{}, {}, {"-as", "-a"}}}}), ops{{fo{}}});
    }

    {
      commandline_arguments<4, 8> a{"foo", "--asyng"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async", {fo{}}}});
        });
    }

    {
      commandline_arguments<4, 3> a{"foo", "-a"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async", {fo{}, {}, {"-as"}}}});
        });
    }

    {
      commandline_arguments<4, 5, 5> a{"foo", "test", "case"};
      
      check_weak_equivalence(LINE(""), parse(3, a.get(), info{{"test", {fo{}, {"case"}, {}}}}), ops{{fo{}, {"case"}}});
    }

    {
      commandline_arguments<4, 2, 5> a{"foo", "t", "case"};
      
      check_weak_equivalence(LINE(""), parse(3, a.get(), info{{"test", {fo{}, {"case"}, {"t"}}}}), ops{{fo{}, {"case"}}});
    }

    {
      commandline_arguments<4, 5> a{"foo", "test"};

      check_exception_thrown<std::runtime_error>(LINE("Final argument missing"), [&a](){
          return parse(2, a.get(), info{{"test", {fo{}, {"case"}}}});
        });
    }

    {
      commandline_arguments<4, 7, 6, 4>  a{
        "foo",
        "create",
        "class",
        "dir"
      };
      
      check_weak_equivalence(LINE(""), parse(4, a.get(), info{{"create", {fo{}, {"class_name", "directory"}}}})
                             , ops{{fo{}, {"class", "dir"}}});
    }

    {
      commandline_arguments<4, 8, 7, 6, 4> a{
        "foo",
        "--async",
        "create",
        "class",
        "dir"
      };
      
      check_weak_equivalence(LINE(""), parse(5, a.get(), info{{"create", {fo{}, {"class_name", "directory"}}}, {"--async", {fo{}}}})
                             , ops{{fo{}}, {fo{}, {"class", "dir"}}});
    }
  }

  void unit_test_runner_false_positive_test::run_tests()
  {
    test_parser();
  }

  void unit_test_runner_false_positive_test::test_parser()
  {
    using info = std::map<std::string, commandline_option_info>;
    using fo = function_object;
    
    {
      commandline_arguments<4, 5> a{"foo", "test"};

      check_exception_thrown<int>(LINE("Final argument missing"), [&a](){
          return parse(2, a.get(), info{{"test", {fo{}, {"case"}}}});
        });
    }


    {
      commandline_arguments<4, 8> a{"foo", "--asyng"};
      
      check_exception_thrown<int>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async", {fo{}}}});
        });      
    }
  }
}
