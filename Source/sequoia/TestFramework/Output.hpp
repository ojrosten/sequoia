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

#include <cmath>
#include <filesystem>
#include <source_location>

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
  consteval line_breaks operator "" _linebreaks(unsigned long long int n) noexcept
  {
    return line_breaks{static_cast<std::size_t>(n)};
  }

  [[nodiscard]]
  std::string emphasise(std::string_view s);

  template<class Char>
  inline constexpr bool is_character_v{
       std::is_same_v<std::remove_cvref_t<Char>, char>
    || std::is_same_v<std::remove_cvref_t<Char>, wchar_t>
    || std::is_same_v<std::remove_cvref_t<Char>, char8_t>
    || std::is_same_v<std::remove_cvref_t<Char>, char16_t>
    || std::is_same_v<std::remove_cvref_t<Char>, char32_t>
  };

  template<class Char>
    requires is_character_v<Char>
  [[nodiscard]]
  std::string display_character(Char c)
  {
    if(c == '\a') return "'\\a'";
    if(c == '\b') return "'\\b'";
    if(c == '\f') return "'\\f'";
    if(c == '\n') return "'\\n'";
    if(c == '\r') return "'\\r'";
    if(c == '\t') return "'\\t'";
    if(c == '\v') return "'\\v'";
    if(c == '\0') return "'\\0'";
    if(c == ' ')  return "' '";

    return std::string(1, static_cast<char>(c));
  }

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
  std::string nullable_type_message(bool obtainedHoldsValue, bool predictedHoldsValue);

  [[nodiscard]]
  std::string equality_operator_failure_message();

  [[nodiscard]]
  std::string pointer_prediction_message();

  [[nodiscard]]
  std::string default_prediction_message(std::string_view obtained, std::string_view prediction);

  [[nodiscard]]
  std::string prediction_message(const std::string& obtained, const std::string& prediction);

  template<class Char>
    requires is_character_v<Char>
  [[nodiscard]]
  std::string prediction_message(Char obtained, Char prediction)
  {
    return prediction_message(display_character(obtained), display_character(prediction));
  }

  template<class Ptr>
    requires std::is_pointer_v<Ptr> || is_const_pointer_v<Ptr>
  [[nodiscard]]
  std::string prediction_message(Ptr obtained, Ptr prediction)
  {
    return (obtained && prediction) ? pointer_prediction_message() : nullable_type_message(obtained, prediction);
  }

  template<serializable T>
    requires (!std::is_floating_point_v<T> && !is_character_v<T> && !std::is_pointer_v<T> && !is_const_pointer_v<T>)
  [[nodiscard]]
  std::string prediction_message(const T& obtained, const T& prediction)
  {
    return default_prediction_message(to_string(obtained), to_string(prediction));
  }

  template<std::floating_point T>
  [[nodiscard]]
  std::string prediction_message(T obtained, T prediction)
  {
    if((obtained > T{}) && (prediction > T{}) || ((obtained < T{}) && (prediction < T{})))
    {
      if(auto diff{std::abs(obtained - prediction)}; diff < T(1))
      {
        const auto precision{1 + static_cast<int>(std::ceil(std::abs(std::log10(diff))))};
        auto toString{
          [precision](T val){
            std::ostringstream os{};
            os << std::setprecision(precision) << val;
            return os.str();
          }
        };

        return default_prediction_message(toString(obtained), toString(prediction));
      }
    }

    return default_prediction_message(to_string(obtained), to_string(prediction));
  }

  template<bool IsFinalMessage>
  struct final_message_constant : std::bool_constant<IsFinalMessage> {};

  using is_final_message_t     = final_message_constant<true>;
  using is_not_final_message_t = final_message_constant<false>;

  inline constexpr is_final_message_t is_final_message{};
  inline constexpr is_not_final_message_t is_not_final_message{};

  template<class T>
  concept reportable = serializable<T> || is_character_v<T>;

  /// To prevent implicit conversions to bool
  template<reportable T>
    requires std::is_same_v<T, bool>
  [[nodiscard]]
  std::string failure_message(is_final_message_t, T, T)
  {
    return "check failed";
  }

  template<reportable T>
    requires (!std::is_same_v<T, bool>)
  [[nodiscard]]
  std::string failure_message(is_final_message_t, const T& obtained, const T& prediction)
  {
    auto message{equality_operator_failure_message()};

    append_lines(message, prediction_message(obtained, prediction));

    return message;
  }

  template<class T>
  [[nodiscard]]
  std::string failure_message(is_not_final_message_t, const T&, const T&)
  {
    return equality_operator_failure_message();
  }

  [[nodiscard]]
  std::string footer();

  [[nodiscard]]
  std::string instability_footer();

  [[nodiscard]]
  std::string report_line(std::string_view message, const std::source_location loc = std::source_location::current(), const std::filesystem::path& repository = {});

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
    return demangle<type_normalizer_t<T>>([](std::string name) -> std::string { return tidy_name(name, compiler_constant{}); });
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
