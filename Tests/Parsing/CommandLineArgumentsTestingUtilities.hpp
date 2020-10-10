////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "CommandLineArguments.hpp"
#include "RegularTestCore.hpp"

namespace sequoia::testing
{
  template<>
  struct weak_equivalence_checker<parsing::commandline::operation>
  {
    using type = sequoia::parsing::commandline::operation;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& operation, const type& prediction)
    {
      const bool consistent{   ((operation.fn != nullptr) && (prediction.fn != nullptr))
                            || ((operation.fn == nullptr) && (prediction.fn == nullptr))};
      testing::check("Existence of function objects differs", logger, consistent);
      check_equality("Operation Parameters differ", logger, operation.parameters, prediction.parameters);
    }
  };

  template<>
  struct weak_equivalence_checker<parsing::commandline::outcome>
  {
    using type = sequoia::parsing::commandline::outcome;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& obtained, const type& prediction)
    {
      check_equality("Zeroth Argument", logger, obtained.zeroth_arg, prediction.zeroth_arg);
      check_weak_equivalence("Operations", logger, obtained.operations, prediction.operations);
      check_equality("Help", logger, obtained.help, prediction.help);
    }
  };

  template<>
  struct weak_equivalence_checker<parsing::commandline::argument_parser>
  {
    using type = parsing::commandline::argument_parser;
    using prediction_type = sequoia::parsing::commandline::outcome;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& obtained, const prediction_type& prediction)
    {
      check_weak_equivalence("", logger, obtained.get(), prediction);
    }
  };

  struct function_object
  {
    void operator()(const std::vector<std::string>&) const noexcept {}
  };

  class commandline_arguments
  {
  public:
    int size() const noexcept
    {
      return static_cast<int>(m_Args.size());
    }
    
    commandline_arguments(std::initializer_list<std::string_view> args)
      : m_Args(args.begin(), args.end())
    {
      m_Ptrs.reserve(m_Args.size());
      std::transform(m_Args.begin(), m_Args.end(), m_Ptrs.begin(), [](std::string& s){ return s.data(); });
    }

    [[nodiscard]]
    char** get() noexcept { return &m_Ptrs[0]; }
  private:
    std::vector<std::string> m_Args;
    std::vector<char*> m_Ptrs;
  };
}
