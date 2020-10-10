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
#include <string>
#include <filesystem>

#ifndef _MSC_VER_
  #include <cxxabi.h>
#endif

namespace sequoia::testing
{
  /*! \brief Type-safe mechanism for indentations */
  struct indentation
  {
    constexpr explicit indentation(std::string_view sv)
      : data{sv}
    {}

    [[nodiscard]]
    std::string string() const
    {
      return std::string{data};
    }
    
    operator std::string() const
    {
      return string();
    }

    friend constexpr bool operator==(const indentation&, const indentation&) noexcept = default;
    friend constexpr bool operator!=(const indentation&, const indentation&) noexcept = default;

    std::string_view data;
  };
  
  constexpr indentation tab{"\t"};
  constexpr indentation no_indent{""};
  
  /// For a non-empty string_view prepends with an indentation; otherwise returns an empty string
  [[nodiscard]]
  std::string indent(std::string_view s, indentation ind);

  /*! \param s1 The target for appending 
      \param s2 The text to append
      \param indentation The absolute (not relative) indentation of s2

      If s1 and s2 are both non-empty, a new line is appended to s1, followed by the indentation
      and then s2.

      If s1 is empty, then s1 is set equal to s2, with no indentation.

      If s2 is empty, no action is taken.
   */
  std::string& append_indented(std::string& s1, std::string_view s2, indentation ind);

  namespace impl
  {
    template<class... Ts, std::size_t... I>
    std::string& append_indented(std::string& s, std::tuple<Ts...> strs, std::index_sequence<I...>)
    {
      (append_indented(s, std::get<I>(strs), std::get<sizeof...(Ts) - 1>(strs)), ...);
      return s;
    }

    template<class... Ts>
    std::string& append_indented(std::string& s, const std::tuple<Ts...>& strs)
    {
      return append_indented(s, strs, std::make_index_sequence<sizeof...(Ts) - 1>{});
    }

    template<class... Ts>
    [[nodiscard]]
    std::string indent(std::string_view sv, const std::tuple<Ts...>& strs)
    {
      auto s{indent(sv, std::get<sizeof...(Ts) - 1>(strs))};
      return append_indented(s, strs);
    }
  }
  
  template<class... Ts>
    requires (sizeof...(Ts) > 2)
  std::string& append_indented(std::string& s, Ts... strs)
  {
    return impl::append_indented(s, std::tuple<Ts...>{strs...});
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 1)
  [[nodiscard]]
  std::string indent(std::string_view sv, Ts&&... strs)
  {
    return impl::indent(sv, std::tuple<Ts...>{std::forward<Ts>(strs)...});
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 0)
  std::string& append_lines(std::string& s, Ts&&... strs)
  {
    return append_indented(s, std::forward<Ts>(strs)..., no_indent);
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 0)
  [[nodiscard]]
  std::string append_lines(std::string_view sv, Ts&&... strs)
  {
    return indent(sv, std::forward<Ts>(strs)..., no_indent);
  }  

  [[nodiscard]]
  std::string emphasise(std::string_view s);

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

  std::string& replace_all(std::string& text, std::string_view from, std::string_view to);

  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message);

  #define LINE(message) report_line(__FILE__, __LINE__, message)

  std::string& tidy_name(std::string& name);

  template<class T>
  [[nodiscard]]
  std::string demangle()
  {
    #ifndef _MSC_VER_
      int status;
      std::string name{abi::__cxa_demangle(typeid(T).name(), 0, 0, &status)};

      tidy_name(name);

      return name;
    #else
      return typeid(T).name();
    #endif
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
