////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "CommandLineArguments.hpp"
#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  template<>
  struct weak_equivalence_checker<parsing::commandline::operation>
  {
    using type = sequoia::parsing::commandline::operation;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& operation, const type& prediction)
    {
      const bool consistent{((operation.fn != nullptr) && (prediction.fn != nullptr))
          || ((operation.fn == nullptr) && (prediction.fn == nullptr))};
      unit_testing::check(combine_messages(description, "Existence of function objects differes"), logger, consistent);
      check_equality(combine_messages(description, "Operation Parameters differ"), logger, operation.parameters, prediction.parameters);
    }
  };
  
  template<>
  struct weak_equivalence_checker<std::vector<parsing::commandline::operation>>
  {
    using type = std::vector<parsing::commandline::operation>;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& operations, const type& prediction)
    {
      check_range_weak_equivalence(description, logger, std::begin(operations), std::end(operations), std::begin(prediction), std::end(prediction));
    }
  };

  struct function_object
  {
    void operator()(const std::vector<std::string>& args) const noexcept {}
  };
  
  template<std::size_t... Ns>
  class commandline_arguments
  {
  public:
    commandline_arguments(char const(&...args)[Ns])
      : m_Args{(char*)args...}
    {}

    [[nodiscard]]
    char** get() noexcept { return &m_Args[0]; }
  private:
    std::array<char*, sizeof...(Ns)> m_Args;
  };
}
