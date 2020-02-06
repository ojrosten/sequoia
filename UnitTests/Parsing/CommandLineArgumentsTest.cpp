////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArgumentsTest.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view commandline_arguments_test::source_file_name() const noexcept
  {
    return __FILE__;
  }
  
  void commandline_arguments_test::run_tests()
  {
    test_parser();
  }

  void commandline_arguments_test::test_parser()
  {
    using namespace sequoia::parsing::commandline;
    
    using ops = std::vector<operation>;
    using info = std::map<std::string, option_info>;
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
      commandline_arguments<4, 4> a{"foo", "-av"};
      
      check_weak_equivalence(LINE(""), parse(2, a.get(),
        info{{"--async", {fo{}, {}, {"-a"}}}, {"--verbose", {fo{}, {}, {"-v"}}}}), ops{{fo{}}, {fo{}}});
    }

    {
      commandline_arguments<4, 5> a{"foo", "-a-v"};
      
      check_weak_equivalence(LINE(""), parse(2, a.get(),
        info{{"--async", {fo{}, {}, {"-a"}}}, {"--verbose", {fo{}, {}, {"-v"}}}}), ops{{fo{}}, {fo{}}});
    }

    {
      commandline_arguments<4, 4, 3> a{"foo", "-av", "-p"};
      
      check_weak_equivalence(LINE(""), parse(3, a.get(),
        info{{"--async", {fo{}, {}, {"-a"}}}, {"--verbose", {fo{}, {}, {"-v"}}}, {"--pause", {fo{}, {}, {"-p"}}}}), ops{{fo{}}, {fo{}}, {fo{}}});
    }

    {
      commandline_arguments<4, 4> a{"foo", "-ac"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async", {fo{}, {}, {"-a"}}}});
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
}
