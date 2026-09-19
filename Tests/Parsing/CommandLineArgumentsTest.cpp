////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArgumentsTest.hpp"

namespace sequoia::testing
{
  using parsing::commandline::outcome;
  using fo = function_object;

  namespace
  {
    using namespace parsing::commandline;

    [[nodiscard]]
    outcome parse(commandline_arguments args, const options_forest& options)
    {
      return parse(args.size(), args.get(), options);
    }
  }

  [[nodiscard]]
  std::filesystem::path commandline_arguments_test::source_file()
  {
    return std::source_location::current().file_name();
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
    check_exception_thrown<std::logic_error>("Empty name",      []() { return option{"", {}, {}, fo{}}; });
    check_exception_thrown<std::logic_error>("Empty alias",     []() { return option{"test", {""}, {}, fo{}}; });
    check_exception_thrown<std::logic_error>("Empty parameter", []() { return option{"test", {}, {""}, fo{}}; });
    check_exception_thrown<std::logic_error>("Top-level option with neither invocable", [](){
      return parse({{"foo", "--async"}}, {{{"--async", {}, {}}}});
    });
 
    check(weak_equivalence, "", parsing::commandline::parse(0, nullptr, {}), outcome{});
 
    check(weak_equivalence,
          "Early",
          parse({{"foo", "--async"}}, {{{"--async", {}, {}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {}}}}});

    check(weak_equivalence,
          "Late",
          parse({{"foo", "--async"}}, {{{"--async", {}, {}, nullptr, fo{}}}}),
          outcome{"foo", {{{nullptr, fo{}, {}}}}});
 
    check(weak_equivalence,
          "Both",
          parse({{"foo", "--async"}}, {{{"--async", {}, {}, fo{"x"}, fo{"y"}}}}),
          outcome{"foo", {{{fo{"x"}, fo{"y"}, {}}}}});

    check(weak_equivalence,
          "Alias",
          parse({{"bar", "-a"}}, {{{"--async", {"-a"}, {}, fo{}}}}),
          outcome{"bar", {{{fo{}, nullptr, {}}}}});

    check(weak_equivalence,
          "Ignored empty option",
          parse({{"bar", "", "-a"}}, {{{"--async", {"-a"}, {}, fo{}}}}),
          outcome{"bar", {{{fo{}, nullptr, {}}}}});

    check(weak_equivalence,
          "Multiple shorthands for a single option",
          parse({{"foo", "-a"}}, {{{"--async", {"-as", "-a"}, {}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {}}}}});

    check_exception_thrown<std::runtime_error>("Typo in argument", [](){
      return parse({{"foo", "--asyng"}}, {{{"--async", {}, {}, fo{}}}});
    });

    check_exception_thrown<std::runtime_error>("Alias mismatch", [](){
      return parse({{"foo", "-a"}}, {{{"--async", {"-as"}, {}, fo{}}}});
    });

    check_exception_thrown<std::runtime_error>("Missing alias", []() {
      return parse({{"foo", "-"}}, {{{"--async", {"-a"}, {}, fo{}}}});
    });

    check_exception_thrown<std::runtime_error>("Extra space in argument", []() {
      return parse({{"foo", "- a"}}, {{{"--async", {"-a"}, {}, fo{}}}});
    });

    check(weak_equivalence,
          "Concatenated alias",
          parse({{"foo", "-av"}}, {{{"--async",   {"-a"}, {}, fo{}}},
                                 {{"--verbose", {"-v"}, {}, fo{}}}}),
          outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }});


    check(weak_equivalence,
          "Concatenated alias with dash",
          parse({{"foo", "-a-v"}}, {{{"--async", {"-a"}, {}, fo{}}},
                                  {{"--verbose", {"-v"}, {}, fo{}}}}),
          outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }});


    check(weak_equivalence,
          "Concatenated alias and single alias",
          parse({{"foo", "-av", "-p"}}, {{{"--async",   {"-a"}, {}, fo{}}},
                                       {{"--verbose", {"-v"}, {}, fo{}}},
                                       {{"--pause",   {"-p"}, {}, fo{}}}}),
          outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }});

    check_exception_thrown<std::runtime_error>("Concatenated alias with only partial match", [](){
      return parse({{"foo", "-ac"}}, {{{"--async", {"-a"}, {}, fo{}}}});
    });

    check_exception_thrown<std::runtime_error>("Concatenated alias naming an option with parameters", [](){
      return parse({{"foo", "-at"}}, {{{"--async", {"-a"}, {}, fo{}}},
                                      {{"test",    {"-t"}, {"case"}, fo{}}}});
    });

    check_exception_thrown<std::runtime_error>("Concatenated aliases without the leading dash", [](){
      return parse({{"foo", "xa"}}, {{{"--async", {"-a"}, {}, fo{}}}});
    });

    check_exception_thrown<std::runtime_error>("A dash after something other than a dash", [](){
      return parse({{"foo", "x-"}}, {{{"--async", {"-a"}, {}, fo{}}}});
    });

    check(weak_equivalence,
          "Alias without leading dash",
          parse({{"bar", "c"}}, {{{"create", {"c"}, {}, fo{}}}}),
          outcome{"bar", {{{fo{}, nullptr, {}}}}});

    check(weak_equivalence,
          "Option with paramater",
          parse({{"foo", "test", "thing"}}, {{{"test", {}, {"case"}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {"thing"}}}}});

    check(weak_equivalence,
          "Aliased option with parameter",
          parse({{"foo", "t", "thing"}}, {{{"test", {"t"}, {"case"}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {"thing"}}}}});

    check(weak_equivalence,
          "Empty parameter",
          parse({{"foo", "test", ""}}, {{{"test", {}, {"case"}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {""}}}}});


    check_exception_thrown<std::runtime_error>("Final argument missing",
      [](){
        return parse({{"foo", "test"}}, {{{"test", {}, {"case"}, fo{}}}});
      });

    check(weak_equivalence,
          "Two parameter option",
          parse({{"foo", "create", "class", "dir"}}, {{{"create", {}, {"class_name", "directory"}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {"class", "dir"}}}}});

    check(weak_equivalence,
          "Two options",
          parse({{"foo", "--async", "create", "class", "dir"}},
                {{{"create",  {}, {"class_name", "directory"}, fo{}}},
                 {{"--async", {}, {}, fo{}}}}),
          outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {"class", "dir"}}} }});


    {
      commandline_arguments a{{"foo", "--async", "create", "class", "dir"}};

      check(weak_equivalence,
            "Two options, invoked with argument_parser",
            argument_parser{a.size(), a.get(), { {{"create",  {}, {"class_name", "directory"}, fo{}}},
                                                               {{"--async", {}, {}, fo{}}} }},
            outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {"class", "dir"}}}}});
    }
  }

  void commandline_arguments_test::test_flat_parsing_help()
  {
    check(weak_equivalence,
          "Single option help",
          parse({{"foo", "--help"}}, {{{"--async", {}, {}, fo{}}}}),
          outcome{"foo", {}, "--async\n"});

    check(weak_equivalence,
          "Single option alias help",
          parse({{"foo", "--help"}}, {{{"--async", {"-a"}, {}, fo{}}}}),
          outcome{"foo", {}, "--async | -a |\n"});

    check(weak_equivalence,
          "Single option multi-alias help",
          parse({{"foo", "--help"}}, {{{"--async", {"-a","-as"}, {}, fo{}}}}),
          outcome{"foo", {}, "--async | -a -as |\n"});

    check(weak_equivalence,
          "Multi-option help",
          parse({{"foo", "--help"}},
                { {{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                  {{"--async", {}, {}, fo{}}}}),
          outcome{"foo", {}, "create | -c | class_name, directory\n--async\n"});

    {
      commandline_arguments a{{"foo", "--help"}};

      check(weak_equivalence,
            "Multi-option help, with argument_parser",
            argument_parser{a.size(), a.get(), { {{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                                                               {{"--async", {}, {}, fo{}}} }},
            outcome{"foo", {}, "create | -c | class_name, directory\n--async\n"});
    }

    check(weak_equivalence,
          "Help requested after an option without parameters or nested options describes that option",
          parse({{"foo", "--async", "--help"}},
                { {{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                  {{"--async", {}, {}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {}}}}, "--async\n"});

    check(weak_equivalence,
          "Help requested after a concatenated alias group describes the group's last option",
          parse({{"foo", "-av", "--help"}},
                { {{"--async",   {"-a"}, {}, fo{}}},
                  {{"--verbose", {"-v"}, {}, fo{}}}}),
          outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }, "--verbose | -v |\n"});

    check(weak_equivalence,
          "Help requested after a concatenated alias group describes the group's last option, not the forest's last",
          parse({{"foo", "-va", "--help"}},
                { {{"--async",   {"-a"}, {}, fo{}}},
                  {{"--verbose", {"-v"}, {}, fo{}}}}),
          outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }, "--async | -a |\n"});

    check(weak_equivalence,
          "Help requested part way through an option's parameters is help, not a parameter",
          parse({{"foo", "create", "class", "--help"}},
                { {{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                  {{"--async", {}, {}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {"class"}}}}, "create | -c | class_name, directory\n"});

    check(weak_equivalence,
          "Help requested before an option's parameters describes that option",
          parse({{"foo", "create", "--help"}},
                { {{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                  {{"--async", {}, {}, fo{}}}}),
          outcome{"foo", {{{fo{}, nullptr, {}}}}, "create | -c | class_name, directory\n"});
  }

  void commandline_arguments_test::test_nested_parsing()
  {
     check(weak_equivalence,
           "A nested option, not bound to a function object, not called",
           parse({{"", "create", "class", "dir"}},
                 {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {{"--equivalent-type", {}, {"type"}}} } } }}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir"}}}}});

     check(weak_equivalence,
           "A nested option, not bound to a function object, utilized",
           parse({{"bar", "create", "class", "dir", "--equivalent-type", "foo"}},
                 {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {{"--equivalent-type", {}, {"type"}}} } } }}),
           outcome{"bar", {{{fo{}, nullptr, {"class", "dir", "foo"}}}}});

     check_exception_thrown<std::runtime_error>("A nested option, not bound to a function object, missing its argument", [](){
       return parse({{"bar", "create", "class", "--equivalent-type"}},
                    {{ {"create", {}, {"class_name"}, fo{}, {},
                         { {{"--equivalent-type", {}, {"type"}}} } } }});
     });

     check_exception_thrown<std::runtime_error>("A nested option, not bound to a function object, missing two of its three arguments", [](){
       return parse({{"bar", "create", "class", "dir", "--equivalent-type", "foo"}},
                    {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                         { {{"--equivalent-type", {}, {"type", "header", "namespace"}}} } } }});
     });

     check_exception_thrown<std::runtime_error>("A dash after something other than a dash, at a nested level", [](){
       return parse({{"bar", "create", "class", "dir", "x-"}},
                    {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                         { {{"--equivalent-type", {}, {"type"}}} } } }});
     });

     check(weak_equivalence,
           "A nested option, bound to a function object, utilized",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo"}},
                 {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {{"--equivalent-type", {}, {"type"}, fo{}}} } } }}),
           outcome{"", {{{ fo{}, nullptr, {"class", "dir"}, { { fo{}, nullptr, {"foo"}} } }}}});

     check(weak_equivalence,
           "Two nested options",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "--generate", "bar"}},
                 {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {{"--equivalent-type", {}, {"type"}}},
                        {{"--generate",        {}, {"file"}, fo{}} }}}}}),
           outcome{"", {{{ fo{}, nullptr, {"class", "dir", "foo"}, { { fo{}, nullptr, {"bar"}} } }}}});

     check(weak_equivalence,
           "Two options, one with nesting, the other aliased",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "-v"}},
                 { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {"--equivalent-type", {}, {"type"}} } 
                   }},
                   {{"--verbose", {"-v"}, {}, fo{}}}}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}}});

     check(weak_equivalence,
           "A nested option, for which the optional alias could potentially clash with a different option",
           parse({{"", "create", "class", "dir", "--e"}},
                 { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {"--equivalent-type", {"-e"}, {"type"}}}
                   }},
                   {{"--e", {}, {}, fo{"e"}}}}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir"}}}, {{fo{"e"}, nullptr, {}}}}});

     check(weak_equivalence,
           "Two options, one with nesting, the other aliased without a leading dash",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "u"}},
                 {{{"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {"--equivalent-type", {}, {"type"}} }
                   }},
                   {{"update", {"u"}, {}, fo{}}}}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}}});

     check(weak_equivalence,
           "Three options, one with nesting, the other two aliased",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "-v", "-a"}},
                 { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {"--equivalent-type", {}, {"type"}} }
                   }},
                   {{"--verbose", {"-v"}, {}, fo{}}},
                   {{"--async", {"-a"}, {}, fo{}}}}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}}});

     check(weak_equivalence,
           "Three options, one with nesting, the other two aliased; invoked with concatenated alias",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "-a", "-v"}},
                 { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                       { {"--equivalent-type", {}, {"type"}} }
                    }},
                    {{"--verbose", {"-v"}, {}, fo{}}},
                    {{"--async", {"-a"}, {}, fo{}}}}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}}});

     check(weak_equivalence,
           "Three options, one with nesting, the other two aliased; invoked with concatenated alias",
           parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "-va"}},
                 {{{"create", {}, {"class_name", "directory"}, fo{}, {},
                      { {"--equivalent-type", {}, {"type"}} }
                   }},
                   {{"--verbose", {"-v"}, {}, fo{}}},
                   {{"--async", {"-a"}, {}, fo{}}}}),
           outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}}});

     check(weak_equivalence,
           "Nested mode",
           parse({{"", "create", "regular_test", "maybe<class T>", "std::optional<T>"}},
                 {{{"create", {"c"}, {}, fo{}, {},
                      {{ "regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent type"},
                          fo{}
                      }}
                 }}}),
           outcome{"", {{{fo{}, nullptr, {}, {{fo{}, nullptr, {"maybe<class T>", "std::optional<T>"}}} }}}});

     check(weak_equivalence,
           "Nested mode, invoked with short-hand",
           parse({{"", "c", "regular", "maybe<class T>", "std::optional<T>"}},
                 {{{"create", {"c"}, {}, fo{}, {},
                      {{ "regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent type"},
                          fo{}
                      }}
                 }}}),
           outcome{"", {{{fo{}, nullptr, {}, {{fo{}, nullptr, {"maybe<class T>", "std::optional<T>"}}} }}}});

      // This is subtle! After the first 'create' is parsed, the second one is not
      // recognized as a nested option and so it is re-parsed as a top-level option.
     check(weak_equivalence,
           "Nested mode with duplicated command",
           parse({{"", "create", "create", "regular", "maybe<class T>", "std::optional<T>"}},
                 {{{"create", {"c"}, {}, fo{}, {},
                      {{ "regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent type"},
                          fo{}
                      }}
                 }}}),
           outcome{"", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}, {{fo{}, nullptr, {"maybe<class T>", "std::optional<T>"}}} }} }});

     check_exception_thrown<std::runtime_error>("Two options, one with nesting, illegal argument",
       []() {
         return parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "blah"}},
                      {{{"create", {}, {"class_name", "directory"}, fo{}, {},
                           { {"--equivalent-type", {}, {"type"}} }
                        }},
                        {{"--verbose", {"-v"}, {}, fo{}}}});
       });
  }

  void commandline_arguments_test::test_nested_parsing_help()
  {
     check(weak_equivalence,
           "Nested help",
           parse({{"", "--help"}},
                 { {{"create", {"c"}, {}, fo{}, {},
                      {{"regular_test",
                         {"regular"},
                         {"qualified::class_name<class T>", "equivalent_type"},
                         fo{}
                      }}
                    }} }),
           outcome{"",
                   {},
                   "create | c |\n  regular_test | regular | "
                   "qualified::class_name<class T>, equivalent_type\n"});

      check(weak_equivalence,
            "Help requested after an option without parameters but with nested options describes that option",
            parse({{"", "create", "--help"}},
                  { {{"create", {"c"}, {}, fo{}, {},
                       {{"regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent_type"},
                          fo{}
                       }}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {}}}},
                    "create | c |\n  regular_test | regular | "
                    "qualified::class_name<class T>, equivalent_type\n"});

      check(weak_equivalence,
            "Help requested after an option's parameters describes that option",
            parse({{"", "create", "class", "dir", "--help"}},
                  {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                       { {{"--equivalent-type", {}, {"type"}}} } } }}),
            outcome{"", {{{fo{}, nullptr, {"class", "dir"}}}}, "create class_name, directory\n  --equivalent-type type\n"});

      check(weak_equivalence,
            "Help requested after the parameters of an option without nested options describes that option",
            parse({{"", "create", "class", "dir", "--help"}},
                  { {{"create", {}, {"class_name", "directory"}, fo{}}},
                    {{"--async", {}, {}, fo{}}}}),
            outcome{"", {{{fo{}, nullptr, {"class", "dir"}}}}, "create class_name, directory\n"});

      check(weak_equivalence,
            "Help requested after a completed nested option describes that option, not the enclosing one",
            parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "--help"}},
                  {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                       { {{"--equivalent-type", {}, {"type"}}} } } }}),
            outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}}, "--equivalent-type type\n"});

      check(weak_equivalence,
            "Help requested after a top-level option which follows a nested one describes the top-level option",
            parse({{"", "create", "class", "dir", "--equivalent-type", "foo", "-v", "--help"}},
                  { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                       { {"--equivalent-type", {}, {"type"}} }
                    }},
                    {{"--verbose", {"-v"}, {}, fo{}}}}),
            outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}}, "--verbose | -v |\n"});

      check(weak_equivalence,
            "Help requested before a nested option's parameters describes that option",
            parse({{"", "create", "regular_test", "--help"}},
                  { {{"create", {"c"}, {}, fo{}, {},
                       {{"regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent_type"},
                          fo{}
                       }}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {}, {{fo{}, nullptr, {}}}}}},
                    "regular_test | regular | "
                    "qualified::class_name<class T>, equivalent_type\n"});

      check(weak_equivalence,
            "Help requested before the parameters of an option with nested options describes its whole sub-tree",
            parse({{"", "init", "--help"}},
                  { {{"init", {"i"}, {"copyright owner", "path"}, fo{}, {},
                       {{"--no-build", {}, {}, fo{}}}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {}}}},
                    "init | i | copyright owner, path\n  --no-build\n"});

      check(weak_equivalence,
            "Help requested while a nested option's parameters are being collected, with arguments after it",
            parse({{"", "create", "regular_test", "--help", "unrecognized"}},
                  { {{"create", {"c"}, {}, fo{}, {},
                       {{"regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent_type"},
                          fo{}
                       }}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {}, {{fo{}, nullptr, {}}}}}},
                    "regular_test | regular | "
                    "qualified::class_name<class T>, equivalent_type\n"});

      check(weak_equivalence,
            "Help requested while the parameters of a nested option without a function object are being collected",
            parse({{"", "create", "class", "dir", "--equivalent-type", "--help"}},
                  {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                       { {{"--equivalent-type", {}, {"type"}}} } } }}),
            outcome{"", {{{fo{}, nullptr, {"class", "dir"}}}}, "--equivalent-type type\n"});

      check(weak_equivalence,
            "Help requested part way through an option's parameters leaves the partial operation in the forest",
            parse({{"", "init", "owner", "--help"}},
                  { {{"init", {"i"}, {"copyright owner", "path"}, fo{}, {},
                       {{"--no-build", {}, {}, fo{}}}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {"owner"}}}},
                    "init | i | copyright owner, path\n  --no-build\n"});

      check(weak_equivalence,
            "Help requested between a nested option's parameters and one of its own nested options "
            "describes that option and parses nothing further",
            parse({{"", "create", "regular_test", "a", "b", "--help", "--header", "h"}},
                  { {{"create", {"c"}, {}, fo{}, {},
                       {{"regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent_type"},
                          fo{},
                          {},
                          {{"--header", {"-h"}, {"header"}}}
                       }}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {}, {{fo{}, nullptr, {"a", "b"}}}}}},
                    "regular_test | regular | qualified::class_name<class T>, equivalent_type\n  --header | -h | header\n"});

      check(weak_equivalence,
            "Nothing after help is parsed",
            parse({{"", "create", "--help", "unrecognized"}},
                  { {{"create", {"c"}, {}, fo{}, {},
                       {{"regular_test",
                          {"regular"},
                          {"qualified::class_name<class T>", "equivalent_type"},
                          fo{}
                       }}
                  }} }),
            outcome{"",
                    {{{fo{}, nullptr, {}}}},
                    "create | c |\n  regular_test | regular | "
                    "qualified::class_name<class T>, equivalent_type\n"});
    }
}
