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
    using args = std::vector<std::vector<std::string>>;
    using info = std::map<std::string, commandline_option_info>;
    
    {
      const auto parsed{parse(0, nullptr, info{})};
      check_equality(LINE(""), parsed, args{});
    }

    {
      commandline_arguments<4, 8> a{"foo", "--async"};
      
      check_equality(LINE(""), parse(2, a.get(), info{{"--async",{}}}), args{{"--async"}});
    }

    {
      commandline_arguments<4, 3> a{"foo", "-a"};
      
      check_equality(LINE(""), parse(2, a.get(), info{{"--async", {{}, {"-a"}}}}), args{{"--async"}});
    }

    {
      commandline_arguments<4, 3> a{"foo", "-a"};
      
      check_equality(LINE(""), parse(2, a.get(), info{{"--async", {{}, {"-as", "-a"}}}}), args{{"--async"}});
    }

    {
      commandline_arguments<4, 8> a{"foo", "--asyng"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async",{}}});
        });
    }

    {
      commandline_arguments<4, 3> a{"foo", "-a"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async",{{}, {"-as"}}}});
        });
    }

    {
      commandline_arguments<4, 5, 5> a{"foo", "test", "case"};
      
      check_equality(LINE(""), parse(3, a.get(), info{{"test",{{"case"}}}}), args{{{"test"}, {"case"}}});
    }

    {
      commandline_arguments<4, 2, 5> a{"foo", "t", "case"};
      
      check_equality(LINE(""), parse(3, a.get(), info{{"test",{{"case"}, {"t"}}}}), args{{{"test"}, {"case"}}});
    }

    {
      commandline_arguments<4, 5> a{"foo", "test"};

      check_exception_thrown<std::runtime_error>(LINE("Final argument missing"), [&a](){
          return parse(2, a.get(), info{{"test",{{"case"}}}});
        });
    }

    {
      commandline_arguments<4, 7, 6, 4>  a{
        "foo",
        "create",
        "class",
        "dir"
      };
      
      check_equality(LINE(""), parse(4, a.get(), info{{"create",{{"class_name", "directory"}}}})
                     , args{{{"create"}, {"class"}, {"dir"}}});
    }

    {
      commandline_arguments<4, 8, 7, 6, 4> a{
        "foo",
        "--async",
        "create",
        "class",
        "dir"
      };
      
      check_equality(LINE(""), parse(5, a.get(), info{{"create",{{"class_name", "directory"}}}, {"--async", {}}})
                     , args{{{"--async"}}, {{"create"}, {"class"}, {"dir"}}});
    }
  }

  void unit_test_runner_false_positive_test::run_tests()
  {
    test_parser();
  }

  void unit_test_runner_false_positive_test::test_parser()
  {
    using info = std::map<std::string, commandline_option_info>;
    
    {
      commandline_arguments<4, 5> a{"foo", "test"};

      check_exception_thrown<int>(LINE("Final argument missing"), [&a](){
          return parse(2, a.get(), info{{"test",{{"case"}}}});
        });
    }


    {
      commandline_arguments<4, 8> a{"foo", "--asyng"};
      
      check_exception_thrown<int>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async",{}}});
        });
    }
  }
}
