////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of functions for formatting test output.
 */

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TextProcessing/Indent.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <filesystem>
#include <iostream>

namespace sequoia::testing
{
  class line_breaks
  {
  public:
    constexpr line_breaks() = default;

    constexpr explicit line_breaks(std::size_t n) : m_Breaks{n}
    {}

    [[nodiscard]]
    constexpr std::size_t value() const noexcept
    {
      return m_Breaks;
    }
  private:
    std::size_t m_Breaks{};
  };

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  line_breaks operator "" _linebreaks(unsigned long long int n) noexcept
  {
    return line_breaks{static_cast<std::size_t>(n)};
  }

  [[nodiscard]]
  std::string emphasise(std::string_view s);

  [[nodiscard]]
  std::string display_character(char c);

  void end_block(std::string& s, line_breaks newlines, std::string_view footer="");

  [[nodiscard]]
  std::string end_block(std::string_view s, line_breaks newlines, std::string_view footer="");

  [[nodiscard]]
  std::string exception_message(std::string_view tag,
                                const std::filesystem::path& filename,
                                const uncaught_exception_info& info,
                                std::string_view exceptionMessage);

  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view retVal);

  [[nodiscard]]
  std::string prediction_message(char obtained, char predicted);

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted);

  template<serializable T>
    requires (!std::same_as<T, std::string>)
  [[nodiscard]]
  std::string prediction_message(const T& obtained, const T& prediction)
  {
    return prediction_message(to_string(obtained), to_string(prediction));
  }

  [[nodiscard]]
  std::string failure_message(bool, bool);

  template<class T>
  [[nodiscard]]
  std::string failure_message([[maybe_unused]] const T& obtained, [[maybe_unused]] const T& prediction)
  {
    auto message{operator_message("==", "false")};

    constexpr bool delegate{has_detailed_equality_checker_v<T> || sequoia::range<T>};
    if constexpr(!delegate)
    {
      append_lines(message, prediction_message(obtained, prediction));
    }

    return message;
  }

  [[nodiscard]]
  std::string footer();

  [[nodiscard]]
  std::string instability_footer();

  [[nodiscard]]
  std::string report_line(const std::filesystem::path& file, int line, std::string_view message, const std::filesystem::path& repository={});

  #define LINE(message) report_line(__FILE__, __LINE__, message)

  [[nodiscard]]
  std::string tidy_name(std::string name, clang_type);

  [[nodiscard]]
  std::string tidy_name(std::string name, gcc_type);

  [[nodiscard]]
  std::string tidy_name(std::string name, msvc_type);

  [[nodiscard]]
  std::string tidy_name(std::string name, other_compiler_type);

  [[nodiscard]]
  std::string demangle(std::string mangled);

  template<class T, invocable_r<std::string, std::string> Tidy>
  [[nodiscard]]
  std::string demangle(Tidy tidy)
  {
    return tidy(demangle({typeid(T).name()}));
  }

  template<class T>
  [[nodiscard]]
  std::string demangle()
  {
    return demangle<T>([](std::string name) -> std::string { return tidy_name(name, compiler_constant{}); });
  }

  /*! \brief Specialize this struct template to customize the way in which type info is generated for a given class.
      This is particularly useful for class templates where standard de-mangling may be hard to read!

      \anchor type_demangler_primary
   */

  template<class T>
  struct type_demangler
  {
    [[nodiscard]]
    static std::string make()
    {
      return demangle<T>();
    }
  };


  /// Demangles T; if U... is not empty, appends each demangled element of U on a new line
  template<class T, class... U>
  struct type_list_demangler
  {
    [[nodiscard]]
    static std::string make()
    {
      auto info{type_demangler<T>::make()};
      if constexpr(sizeof...(U) > 0)
      {
        info += ',';
        append_lines(info, type_list_demangler<U...>::make());
      }

      return info;
    }
  };

  template<class T, class... U>
  [[nodiscard]]
  std::string make_type_info()
  {
    return std::string{"["}.append(type_list_demangler<T, U...>::make()).append("]");
  }

  template<class T, class... U>
  [[nodiscard]]
  std::string add_type_info(std::string_view description)
  {
    return append_lines(description, make_type_info<T, U...>());
  }
}
