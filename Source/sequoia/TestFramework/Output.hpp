////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A collection of functions for formatting test output.
 */

#include "sequoia/Core/Meta/TypeName.hpp"
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
  consteval line_breaks operator ""_linebreaks(unsigned long long int n) noexcept
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

  constexpr void end_block(std::string& s, const line_breaks newlines, std::string_view footer="")
  {
    if(!s.empty())
    {
      std::size_t n{};
      for(; n < std::ranges::min(s.size(), newlines.value()); ++n)
      {
        if(s[s.size() - 1 - n] != '\n') break;
      }

      for(; n<newlines.value(); ++n)
      {
        s.append("\n");
      }

      s.append(footer);
    }
  }

  [[nodiscard]]
  constexpr std::string end_block(std::string_view s, const line_breaks newlines, std::string_view footer="")
  {
    std::string text{s};
    end_block(text, newlines, footer);

    return text;
  }

  [[nodiscard]]
  std::string exception_message(std::string_view tag,
                                const std::filesystem::path& filename,
                                const uncaught_exception_info& info,
                                std::string_view exceptionMessage);

  [[nodiscard]]
  constexpr std::string operator_message(std::string_view op, std::string_view opRetVal)
  {
    return std::string{"operator"}.append(op).append(" returned ").append(opRetVal);
  }

  [[nodiscard]]
  constexpr std::string nullable_type_message(const bool holdsValue)
  {
    return std::string{holdsValue ? "not " : ""}.append("null");
  }

  [[nodiscard]]
  constexpr std::string nullable_type_message(const bool obtainedHoldsValue, const bool predictedHoldsValue)
  {
    return std::string{"Obtained : "}.append(nullable_type_message(obtainedHoldsValue)).append("\n")
               .append("Predicted: ").append(nullable_type_message(predictedHoldsValue));
  }

  [[nodiscard]]
  constexpr std::string equality_operator_failure_message()
  {
    return operator_message("==", "false");
  }

  [[nodiscard]]
  constexpr std::string pointer_prediction_message()
  {
    return "Pointers both non-null, but they point to different addresses";
  }

  [[nodiscard]]
  constexpr std::string default_prediction_message(std::string_view obtained, std::string_view prediction)
  {
    return append_lines(std::string{"Obtained : "}.append(obtained), std::string{"Predicted: "}.append(prediction));
  }

  [[nodiscard]]
  constexpr std::string prediction_message(const std::string& obtained, const std::string& prediction)
  {
    return default_prediction_message(obtained, prediction);
  }

  template<class Char>
    requires is_character_v<Char>
  [[nodiscard]]
  constexpr std::string prediction_message(Char obtained, Char prediction)
  {
    return prediction_message(display_character(obtained), display_character(prediction));
  }

  template<class Ptr>
    requires std::is_pointer_v<Ptr> || is_const_pointer_v<Ptr>
  [[nodiscard]]
  constexpr std::string prediction_message(Ptr obtained, Ptr prediction)
  {
    return (obtained && prediction) ? pointer_prediction_message() : nullable_type_message(obtained, prediction);
  }

  template<serializable T>
    requires (!is_character_v<T> && !std::is_pointer_v<T> && !is_const_pointer_v<T>)
  [[nodiscard]]
  constexpr std::string prediction_message(const T& obtained, const T& prediction)
  {
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

  template<reportable T>
  [[nodiscard]]
  constexpr std::string failure_message(is_final_message_t, const T& obtained, const T& prediction)
  {
    auto message{equality_operator_failure_message()};

    append_lines(message, prediction_message(obtained, prediction));

    return message;
  }

  template<class T>
  [[nodiscard]]
  constexpr std::string failure_message(is_not_final_message_t, const T&, const T&)
  {
    return equality_operator_failure_message();
  }

  [[nodiscard]]
  constexpr std::string footer()
  {
    return "=======================================\n";
  }

  [[nodiscard]]
  constexpr std::string instability_footer()
  {
    return "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n";
  }

  [[nodiscard]]
  std::string report_line(std::string_view message, const std::filesystem::path& repository, const std::source_location loc);

  [[nodiscard]]
  std::filesystem::path path_for_reporting(const std::filesystem::path& file, const std::filesystem::path& repository);

  struct no_source_location_t{};
  inline constexpr no_source_location_t no_source_location{};

  class reporter
  {
  public:
    reporter(const char* message, const std::source_location loc = std::source_location::current())
      : reporter{std::string{message},loc}
    {}

    reporter(std::string_view message, const std::source_location loc = std::source_location::current())
      : reporter{std::string{message},loc}
    {}

    reporter(std::string message, const std::source_location loc = std::source_location::current())
      : m_Message{std::move(message)}
      , m_Loc{loc}
    {}

    reporter(std::string message, no_source_location_t)
      : m_Message{std::move(message)}
    {}

    reporter(std::string_view message, no_source_location_t)
      : reporter{std::string{message}, no_source_location}
    {}

    [[nodiscard]]
    const std::string& message() const noexcept { return m_Message; }

    [[nodiscard]]
    const std::optional<std::source_location>& location() const noexcept { return m_Loc; }
  private:
    std::string m_Message{};
    std::optional<std::source_location> m_Loc{};
  };

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

  template<class T, invocable_exact_r<std::string, std::string> Tidy>
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

  /** \brief Specialize this struct template to customize the way in which type info is generated for a given class.
      This is particularly useful for class templates where standard de-mangling may be hard to read!

      \anchor type_demangler_primary
   */

  template<class T>
  struct type_demangler
  {
    [[nodiscard]]
    constexpr static std::string make()
    {
      // The demangler reads typeid at run time; a constant evaluation has the compiler's spelling instead
      if consteval
      {
        return std::string{meta::tidy_type_name(meta::type_name<type_normalizer_t<T>>())};
      }
      else
      {
        return demangle<T>();
      }
    }
  };


  /// Demangles T; if U... is not empty, appends each demangled element of U on a new line
  template<class T, class... U>
  struct type_list_demangler
  {
    [[nodiscard]]
    constexpr static std::string make()
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
  constexpr std::string make_type_info()
  {
    return std::string{"["}.append(type_list_demangler<T, U...>::make()).append("]");
  }

  template<class T, class... U>
  [[nodiscard]]
  constexpr std::string add_type_info(std::string description)
  {
    return append_lines(std::move(description), make_type_info<T, U...>());
  }
}
