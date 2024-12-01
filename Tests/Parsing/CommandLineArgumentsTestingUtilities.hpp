////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "Maths/Graph/GraphTestingUtilities.hpp"


namespace sequoia::testing
{
  struct function_object
  {
    explicit function_object(std::string t="")
      : tag{t}
    {}

    void operator()(const parsing::commandline::arg_list&) const noexcept {}

    std::string tag{};
  };

  template<>
  struct value_tester<parsing::commandline::operation>
  {
    using type = parsing::commandline::operation;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& operation, const type& prediction)
    {
      check_executor(logger, operation.early, prediction.early, "early");
      check_executor(logger, operation.late, prediction.late, "late");
      check(equality, "Operation Parameters differ", logger, operation.arguments, prediction.arguments);
    }
  private:
    using executor = parsing::commandline::executor;

    template<test_mode Mode>
    static void check_executor(test_logger<Mode>& logger, const executor& operation, const executor& prediction, std::string_view tag)
    {
      const bool consistent{(operation && prediction) || (!operation && !prediction)};
      testing::check(std::string{"Existence of"}.append(tag).append(" function objects differs"), logger, consistent);

      if(operation && prediction)
      {
        check(equality,
              "Function object tag",
              logger,
              operation.target<function_object>()->tag,
              prediction.target<function_object>()->tag);
      }
    }
  };


  template<>
  struct value_tester<parsing::commandline::outcome>
  {
    using type = parsing::commandline::outcome;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction)
    {
      check(equality, "Zeroth Argument", logger, obtained.zeroth_arg, prediction.zeroth_arg);
      check(weak_equivalence, "Operations", logger, obtained.operations, prediction.operations);
      check(equality, "Help", logger, obtained.help, prediction.help);
    }
  };

  template<>
  struct value_tester<parsing::commandline::argument_parser>
  {
    using type = parsing::commandline::argument_parser;
    using prediction_type = parsing::commandline::outcome;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const prediction_type& prediction)
    {
      check(weak_equivalence, "", logger, obtained.get(), prediction);
    }
  };

  class commandline_arguments
  {
  public:
    [[nodiscard]]
    int size() const noexcept
    {
      return static_cast<int>(m_Args.size());
    }

    commandline_arguments(std::vector<std::string> args);

    [[nodiscard]]
    char** get() noexcept
    {
      return !m_Ptrs.empty() ? &m_Ptrs[0] : nullptr;
    }
  private:
    std::vector<std::string> m_Args;
    std::vector<char*> m_Ptrs;
  };
}
