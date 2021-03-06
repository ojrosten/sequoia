////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArgumentsDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view commandline_arguments_false_positive_test::source_file() const noexcept
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
    using fo = function_object;
    
    {
      commandline_arguments a{"foo", "test"};

      check_exception_thrown<int>(LINE("Final argument missing"), [&a](){
          return parse(a.size(), a.get(), { {"test", {}, {"case"}, fo{}} });
        });
    }


    {
      commandline_arguments a{"foo", "--asyng"};
      
      check_exception_thrown<int>(LINE("Unexpected argument"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} });
        });      
    }

    {
      commandline_arguments a{"foo", "--async"};
      
      check_exception_thrown<int>(LINE("No bound function object"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {}, {}, nullptr} });
        });      
    }

    {
      commandline_arguments a{"foo", "-ac"};
      
      check_exception_thrown<int>(LINE("Unexpected argument"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}} });
        });
    }

    {
      check_weak_equivalence(LINE("Mismatched zeroth argument"), parse(0, nullptr, {}), outcome{"foo", {}});
    }

    {
      commandline_arguments a{"foo", "--async"};
      
      check_weak_equivalence(LINE("Early function object not generated"), parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} }), outcome{"foo", {{nullptr, nullptr, {}}}});

      check_weak_equivalence(LINE("Late function object not generated"), parse(a.size(), a.get(), { {"--async", {}, {}, nullptr, {}, fo{}} }), outcome{"foo", {{nullptr, nullptr, {}}}});

      check_weak_equivalence(LINE("Unexpected early function object"), parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} }), outcome{"foo", {{fo{"x"}, nullptr, {}}}});

      check_weak_equivalence(LINE("Unexpected late function object"), parse(a.size(), a.get(), { {"--async", {}, {}, nullptr, {}, fo{}} }), outcome{"foo", {{nullptr, fo{"y"}, {}}}});

      check_weak_equivalence(LINE("Mixed-up function objects"), parse(a.size(), a.get(), { {"--async", {}, {}, fo{"x"}, {}, fo{"y"}} }), outcome{"foo", {{fo{"y"}, fo{"x"}, {}}}});
    }

    {
      commandline_arguments a{"foo", "--help"};
      
      check_weak_equivalence(LINE("Help not generated"), parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} }), outcome{"foo", {}, ""});
    }
  }
}
