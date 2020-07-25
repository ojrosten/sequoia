////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of functions for formatting test output. 
 */

#include <string>

#ifndef _MSC_VER_
  #include <cxxabi.h>
#endif

namespace sequoia::testing
{
  constexpr std::string_view tab{"\t"};
  
  /// For a non-empty string_view prepends with an indentation; otherwise returns an empty string
  [[nodiscard]]
  std::string indent(std::string_view s, std::string_view indentation=tab);

  /*! \param s1 The target for appending 
      \param s2 The text to append
      \param indentation The absolute (not relative) indentation of s2

      If s1 and s2 are both non-empty, a new line is appended to s1, followed by the indentation
      and then s2.

      If s1 is empty, then s1 is set equal to s2, with no indentation.

      If s2 is empty, no action is taken.
   */
  std::string& append_indented(std::string& s1, std::string_view s2, std::string_view indentation=tab);

  [[nodiscard]]
  std::string append_indented(std::string_view s1, std::string_view s2, std::string_view indentation=tab);

  [[nodiscard]]
  std::string emphasise(std::string_view s);

  void end_block(std::string& s, const std::size_t newlines, std::string footer="");

  [[nodiscard]]
  std::string exception_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected);

  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view retVal);

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted);

  std::string footer(std::string_view indentation=tab);

  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message);

  #define LINE(message) report_line(__FILE__, __LINE__, message)

  void tidy_name(std::string& name);

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

  template<class T, class... U>
  struct type_list_demangler
  {
    /// Demangles T; if U... is not empty, indents each demangled element of U
    [[nodiscard]]
    static std::string make([[maybe_unused]] std::string_view indent=tab)
    {
      auto info{type_demangler<T>::make()};
      if constexpr(sizeof...(U) > 0)
      {
        info += ',';
        append_indented(info, type_list_demangler<U...>::make(), indent);
      }

      return info;
    }
  };
  
  template<class T, class... U>
  [[nodiscard]]
  std::string make_type_info([[maybe_unused]] std::string_view indent=tab)
  {
    std::string info{"["};
    info.append(type_list_demangler<T, U...>::make(indent));
    info.append("]");

    return info;
  }

  template<class T, class... U>
  [[nodiscard]]
  std::string add_type_info(std::string_view description, std::string_view indent=tab)
  {
    std::string info{description};
    append_indented(info, make_type_info<T, U...>(indent), indent);

    return info;
  }

  class advice_data
  {
  public:
    template<class Advisor, class T>
    advice_data(const Advisor& advisor, const T& value, const T& prediction)
    {
      if constexpr(std::is_invocable_r_v<std::string, Advisor, T, T>)
      {
        m_Advice = advisor(value, prediction);
      }

      
      m_AdviceTypeName = type_demangler<Advisor>::make();
    }

    void append_and_tidy(std::string& message) const;    
  private:
    std::string m_Advice{}, m_AdviceTypeName{};
    
    void tidy(std::string& message) const;
  };

  void append_advice(std::string& message, const advice_data& adviceData);
}
