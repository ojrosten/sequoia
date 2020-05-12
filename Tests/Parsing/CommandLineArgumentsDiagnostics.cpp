////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArgumentsDiagnostics.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view commandline_arguments_false_positive_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void commandline_arguments_false_positive_test::run_tests()
  {
    test_parser();
  }

  void commandline_arguments_false_positive_test::test_parser()
  {
    using namespace sequoia::parsing::commandline;

    using info = std::map<std::string, option_info>;
    using fo = function_object;
    
    {
      commandline_arguments<4, 5> a{"foo", "test"};

      check_exception_thrown<int>(LINE("Final argument missing"), [&a](){
          return parse(2, a.get(), info{{"test", {fo{}, {"case"}, {}}}});
        });
    }


    {
      commandline_arguments<4, 8> a{"foo", "--asyng"};
      
      check_exception_thrown<int>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async", {fo{}, {}, {}}}});
        });      
    }

    {
      commandline_arguments<4, 8> a{"foo", "--async"};
      
      check_exception_thrown<int>(LINE("No bound function object"), [&a](){
          return parse(2, a.get(), info{{"--async", {nullptr, {}, {}}}});
        });      
    }

    {
      commandline_arguments<4, 4> a{"foo", "-ac"};
      
      check_exception_thrown<int>(LINE("Unexpected argument"), [&a](){
          return parse(2, a.get(), info{{"--async", {fo{}, {}, {"-a"}}}});
        });
    }
  }
}
