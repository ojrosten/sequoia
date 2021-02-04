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

#include "CoreInfrastructure.hpp"
#include "Indent.hpp"
#include "Preprocessor.hpp"

#include <filesystem>
#include <iostream>

#ifndef _MSC_VER
  #include <cxxabi.h>
#endif

namespace sequoia::testing
{
  [[nodiscard]]
  std::string emphasise(std::string_view s);

  [[nodiscard]]
  std::string display_character(char c);

  void end_block(std::string& s, const std::size_t newlines, std::string_view footer="");

  [[nodiscard]]
  std::string end_block(std::string_view s, const std::size_t newlines, std::string_view footer="");

  [[nodiscard]]
  std::string exception_message(std::string_view tag, const std::filesystem::path& filename, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected);

  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view retVal);

  [[nodiscard]]
  std::string prediction_message(char obtained, char predicted);

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted);

  template<serializable T>
    requires (!same_as<T, std::string>)
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

    constexpr bool delegate{has_detailed_equality_checker_v<T> || range<T>};
    if constexpr(!delegate)
    {
      append_lines(message, prediction_message(obtained, prediction));
    }

    return message;
  }

  [[nodiscard]]
  std::string footer();

  [[nodiscard]]
  std::string to_camel_case(std::string text);

  [[nodiscard]]
  std::string to_snake_case(std::string text);

  std::string& replace_all(std::string& text, std::string_view from, std::string_view to);

  struct replacement
  {
    std::string_view from, to;
  };

  std::string& replace_all(std::string& text, std::initializer_list<replacement> data);

  std::string& replace_all(std::string& text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to);

  template<invocable_r<bool, char> LeftPred, invocable_r<bool, char> RightPred>
  std::string& replace_all(std::string& text, LeftPred lPred, std::string_view from, RightPred rPred, std::string_view to)
  {
    constexpr auto npos{std::string::npos};
    std::string::size_type pos{};
    while((pos = text.find(from, pos)) != npos)
    {
      if(    (pos > 0)                             && lPred(text[pos - 1])
          && (pos + from.length() < text.length()) && rPred(text[pos + from.length()])
        )
      {
        text.replace(pos, from.length(), to);
        pos += to.length();
      }
      else
      {
        pos += text.length();
      }
    }

    return text;
  }

  [[nodiscard]]
  std::string report_line(const std::filesystem::path& file, int line, std::string_view message, const std::filesystem::path& repository={});

  #define LINE(message) report_line(__FILE__, __LINE__, message)

  std::string& tidy_name(std::string& name);

  std::string& tidy_msc_name(std::string& name);

  template<class T>
  [[nodiscard]]
  std::string demangle()
  {
    if constexpr(has_msvc_v)
    {
      std::string name{typeid(T).name()};

      return tidy_msc_name(name);
    }
    else
    {
      struct cxa_demangler
      {
        cxa_demangler(const char* name)
          : data{abi::__cxa_demangle(name, 0, 0, &status)}
        {}

        ~cxa_demangler() { std::free(data); }

        int status{-1};
        char* data;
      };

      const auto mangled{typeid(T).name()};
      cxa_demangler c{mangled};

      if(!c.status)
      {
        std::string name{c.data};
        tidy_name(name);
        return name;
      }

      return mangled;
    }
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
