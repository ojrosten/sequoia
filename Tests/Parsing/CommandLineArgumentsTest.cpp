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
    test_flat_parsing_help();
    test_nested_parsing();
    test_nested_parsing_help();
  }

  void commandline_arguments_test::test_flat_parsing()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;
    
    {
      check_weak_equivalence(LINE(""), parse(0, nullptr, {}), outcome{});
    }

    {
      commandline_arguments a{"foo", "--async"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} }), outcome{"foo", {{fo{}, {}}}});
    }

    {
      commandline_arguments a{"bar", "-a"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}} }), outcome{"bar", {{fo{}, {}}}});
    }

    {
      commandline_arguments a{"foo", "-a"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"--async", {"-as", "-a"}, {}, fo{}} }), outcome{"foo", {{fo{}, {}}}});
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
                             outcome{"foo", {{fo{}, {}}, {fo{}, {}}}});
    }

    {
      commandline_arguments a{"foo", "-a-v"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}},
                                                      {"--verbose", {"-v"}, {}, fo{}} }),
                             outcome{"foo", {{fo{}, {}}, {fo{}, {}}}});
    }

    {
      commandline_arguments a{"foo", "-av", "-p"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"--async",   {"-a"}, {}, fo{}},
                                                      {"--verbose", {"-v"}, {}, fo{}},
                                                      {"--pause",   {"-p"}, {}, fo{}} }),
                             outcome{"foo", {{fo{}, {}}, {fo{}, {}}, {fo{}, {}}}});
    }

    {
      commandline_arguments a{"foo", "-ac"};
      
      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
          return parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}} });
        });
    }

    {
      commandline_arguments a{"foo", "test", "case"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"test", {}, {"case"}, fo{}} }), outcome{"foo", {{fo{}, {"case"}}}});
    }

    {
      commandline_arguments a{"foo", "t", "case"};
      
      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), { {"test", {"t"}, {"case"}, fo{}} }), outcome{"foo", {{fo{}, {"case"}}}});
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
                             outcome{"foo", {{fo{}, {"class", "dir"}}}});
    }

    {
      commandline_arguments a{"foo", "--async", "create", "class", "dir"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create",  {}, {"class_name", "directory"}, fo{}},
                                                        {"--async", {}, {}, fo{}} }),
                             outcome{"foo", {{fo{}, {}}, {fo{}, {"class", "dir"}}}});
    }
  }

  void commandline_arguments_test::test_flat_parsing_help()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;

    {
      commandline_arguments a{"foo", "--help"};
      
      check_weak_equivalence(LINE("Single option help"), parse(a.size(), a.get(), { {"--async", {}, {}, fo{}} }),
                             outcome{"foo", {}, "--async\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};
      
      check_weak_equivalence(LINE("Single option alias help"), parse(a.size(), a.get(), { {"--async", {"-a"}, {}, fo{}} }),
                             outcome{"foo", {}, "--async | -a |\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};
      
      check_weak_equivalence(LINE("Single option multi-alias help"), parse(a.size(), a.get(), { {"--async", {"-a","-as"}, {}, fo{}} }),
                             outcome{"foo", {}, "--async | -a -as |\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};
      
      check_weak_equivalence(LINE("Multi-option help"),
                             parse(a.size(), a.get(), { {"create",  {"-c"}, {"class_name", "directory"}, fo{}},
                                                        {"--async", {}, {}, fo{}} }),
                             outcome{"foo", {}, "create | -c | class_name directory\n--async\n"});
    }
  }

  void commandline_arguments_test::test_nested_parsing()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;
    
    {
      commandline_arguments a{"", "create", "class", "dir"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create", {}, {"class_name", "directory"}, fo{},
                                                  { {"--equivalent-type", {}, {"type"}} }
                                                 } }),
                             outcome{"", {{fo{}, {"class", "dir"}}}});
    }

    {
      commandline_arguments a{"bar", "create", "class", "dir", "--equivalent-type", "foo"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      }
                                   }),
                             outcome{"bar", {{fo{}, {"class", "dir", "foo"}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(), { {"create", {}, {"class_name", "directory"}, fo{},
                                                         { {"--equivalent-type", {}, {"type"}, fo{}} }
                                                 } }),
                             outcome{"", {{ fo{}, {"class", "dir"}, { { fo{}, {"foo"}} } }}});
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
                             outcome{"", {{ fo{}, {"class", "dir", "foo"}, { { fo{}, {"bar"}} } }}});
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
                             outcome{"", {{fo{}, {"class", "dir", "foo"}}, {fo{}, {}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-v", "-a"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      },
                                     {"--verbose", {"-v"}, {}, fo{}},
                                     {"--async", {"-a"}, {}, fo{}}
                                   }),
                             outcome{"", {{fo{}, {"class", "dir", "foo"}}, {fo{}, {}}, {fo{}, {}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-va"};
      
      check_weak_equivalence(LINE(""),
                             parse(a.size(), a.get(),
                                   { {"create", {}, {"class_name", "directory"}, fo{},
                                        { {"--equivalent-type", {}, {"type"}} }
                                      },
                                     {"--verbose", {"-v"}, {}, fo{}},
                                     {"--async", {"-a"}, {}, fo{}}
                                   }),
                             outcome{"", {{fo{}, {"class", "dir", "foo"}}, {fo{}, {}}, {fo{}, {}}}});
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
                             outcome{"", {{fo{}, {}, {{fo{}, {"maybe<class T>", "std::optional<T>"}}} }}});
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

  void commandline_arguments_test::test_nested_parsing_help()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;

    {
      commandline_arguments a{"", "--help"};

      check_weak_equivalence(LINE("Nested help"),
                             parse(a.size(), a.get(),
                                   { {"create", {"c"}, {}, fo{},
                                        { { "regular_test",
                                            {"regular"},
                                            {"qualified::class_name<class T>", "equivalent_type"},
                                            fo{}
                                          }
                                        }
                                      }
                                   }),
                             outcome{"", {}, "create | c |\n  regular_test | regular | "
                                               "qualified::class_name<class T> equivalent_type\n"});
    }
  }
}
