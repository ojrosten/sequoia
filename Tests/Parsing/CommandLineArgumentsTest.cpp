////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArgumentsTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view commandline_arguments_test::source_file() const noexcept
  {
    return __FILE__;
  }
  
  void commandline_arguments_test::run_tests()
  {
    test_flat_parsing();
    test_nested_parsing();
  }

  void commandline_arguments_test::test_flat_parsing()
  {
    using namespace sequoia::parsing::commandline;
    
    using ops = std::vector<operation>;
    using fo = function_object;
    
    {
      check_weak_equivalence(LINE(""), parse(0, nullptr, {}), ops{});
    }

    {
      commandline_arguments a{"foo", "--async"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} }), ops{{fo{}, {}}});
    }

    {
      commandline_arguments a{"foo", "-a"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}} }), ops{{fo{}, {}}});
    }

    {
      commandline_arguments a{"foo", "-a"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"--async", {"-as", "-a"}, {}, fo{}} }), ops{{fo{}, {}}});
    }

    {
      commandline_arguments a{"foo", "--asyng"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} });
        });
    }

    {
      commandline_arguments a{"foo", "-a"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {"-as"}, {}, fo{}} });
        });
    }

    {
      commandline_arguments a{"foo", "-av"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}},
                                                      {"--verbose", {"-v"}, {}, fo{}} }),
                             ops{{fo{}, {}}, {fo{}, {}}});
    }

    {
      commandline_arguments a{"foo", "-a-v"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}},
                                                      {"--verbose", {"-v"}, {}, fo{}} }),
                             ops{{fo{}, {}}, {fo{}, {}}});
    }

    {
      commandline_arguments a{"foo", "-av", "-p"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"--async",   {"-a"}, {}, fo{}},
                                                      {"--verbose", {"-v"}, {}, fo{}},
                                                      {"--pause",   {"-p"}, {}, fo{}} }),
                             ops{{fo{}, {}}, {fo{}, {}}, {fo{}, {}}});
    }

    {
      commandline_arguments a{"foo", "-ac"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}} });
        });
    }

    {
      commandline_arguments a{"foo", "test", "case"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"test", {}, {"case"}, fo{}} }), ops{{fo{}, {"case"}}});
    }

    {
      commandline_arguments a{"foo", "t", "case"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"test", {"t"}, {"case"}, fo{}} }), ops{{fo{}, {"case"}}});
    }

    {
      commandline_arguments a{"foo", "test"};

      check_exception_thrown<std::runtime_error>(LINE("Final argument missing"),
                                                 [&a](){
                                                   return parse(a.size(), a.get(), { {"test", {}, {"case"}, fo{}} });
        });
    }

    {
      commandline_arguments a{"foo", "create", "class", "dir"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create", {}, {"class_name", "directory"}, fo{}} }),
                             ops{{fo{}, {"class", "dir"}}});
    }

    {
      commandline_arguments a{"foo", "--async", "create", "class", "dir"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create",  {}, {"class_name", "directory"}, fo{}},
                                                        {"--async", {}, {}, fo{}} }),
                             ops{{fo{}, {}}, {fo{}, {"class", "dir"}}});
    }
  }

  void commandline_arguments_test::test_nested_parsing()
  {
    using namespace sequoia::parsing::commandline;
    
    using ops = std::vector<operation>;
    using fo = function_object;
    
    {
      commandline_arguments a{"", "create", "class", "dir"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create", {}, {"class_name", "directory"}, fo{},
                                                  { {"--equivalent-type", {}, {"type"}} }
                                                 } }),
                             ops{{fo{}, {"class", "dir"}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      }
                                   }),
                             ops{{fo{}, {"class", "dir", "foo"}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create", {}, {"class_name", "directory"}, fo{},
                                                         { {"--equivalent-type", {}, {"type"}, fo{}} }
                                                 } }),
                             ops{{ fo{}, {"class", "dir"}, { { fo{}, {"foo"}} } }});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "--generate", "bar"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}},
                                          {"--generate",        {}, {"file"}, fo{}} }
                                      }
                                   }),
                             ops{{ fo{}, {"class", "dir", "foo"}, { { fo{}, {"bar"}} } }});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-v"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      },
                                     {"--verbose", {"-v"}, {}, fo{}}
                                   }),
                             ops{{fo{}, {"class", "dir", "foo"}}, {fo{}, {}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-v", "-a"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      },
                                     {"--verbose", {"-v"}, {}, fo{}},
                                     {"--verbose", {"-a"}, {}, fo{}}
                                   }),
                             ops{{fo{}, {"class", "dir", "foo"}}, {fo{}, {}}, {fo{}, {}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-va"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      },
                                     {"--verbose", {"-v"}, {}, fo{}},
                                     {"--verbose", {"-a"}, {}, fo{}}
                                   }),
                             ops{{fo{}, {"class", "dir", "foo"}}, {fo{}, {}}, {fo{}, {}}});
    }

    {
      commandline_arguments a{"", "create", "regular_test", "maybe<class T>", "std::optional<T>"};

      check_weak_equivalence(LINE("Nested modes"),
                             parse(a.size(), a.get(),
                                   { {"create", {"c"}, {}, fo{},
                                        { { "regular_test",
                                            {"regular"},
                                            {"qualified::class_name<class T>", "equivalent type"},
                                            fo{}
                                          }
                                        }
                                      }
                                   }),
                             ops{{fo{}, {}, {{fo{}, {"maybe<class T>", "std::optional<T>"}}} }});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "blah"};

      check_exception_thrown<std::runtime_error>(LINE(""),
        [&a](){
          return parse(a.size(), a.get(),
                       { {"create", {}, {"class_name", "directory"}, fo{},
                          { {"--equivalent-type", {}, {"type"}} }
                          },
                         {"--verbose", {"-v"}, {}, fo{}}
                       });
        });
    }
  }
}
