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
      std::array<char*, 2> a{new char[4]{"foo"}, new char[8]{"--async"}};
      
      check_equality(LINE(""), parse(2, &a[0], info{{"--async",{}}}), args{{"--async"}});

      for(auto e : a) delete e;
    }

    {
      std::array<char*, 2> a{new char[4]{"foo"}, new char[8]{"--asyng"}};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&](){
          return parse(2, &a[0], info{{"--async",{}}});
        });

      for(auto e : a) delete e;
    }

    {
      std::array<char*, 3> a{new char[4]{"foo"}, new char[5]{"test"}, new char[5]{"case"}};
      
      check_equality(LINE(""), parse(3, &a[0], info{{"test",{{"case"}}}}), args{{{"test"}, {"case"}}});

      for(auto e : a) delete e;
    }

    {
      std::array<char*, 2> a{new char[4]{"foo"}, new char[5]{"test"}};

      check_exception_thrown<std::runtime_error>(LINE("Final argument missing"), [&a](){
          return parse(2, &a[0], info{{"test",{{"case"}}}});
        });

      for(auto e : a) delete e;
    }

    {
      std::array<char*, 4> a{
        new char[4]{"foo"},
        new char[7]{"create"},
        new char[6]{"class"},
        new char[4]{"dir"}
      };
      
      check_equality(LINE(""), parse(4, &a[0], info{{"create",{{"class_name", "directory"}}}})
                     , args{{{"create"}, {"class"}, {"dir"}}});

      for(auto e : a) delete e;
    }

    {
      std::array<char*, 5> a{
        new char[4]{"foo"},
        new char[8]{"--async"},
        new char[7]{"create"},
        new char[6]{"class"},
        new char[4]{"dir"}
      };
      
      check_equality(LINE(""), parse(5, &a[0], info{{"create",{{"class_name", "directory"}}}, {"--async", {}}})
                     , args{{{"--async"}}, {{"create"}, {"class"}, {"dir"}}});

      for(auto e : a) delete e;
    }
  }
}
