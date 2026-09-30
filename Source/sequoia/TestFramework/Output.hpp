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

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TextProcessing/Indent.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <source_location>
#include <type_traits>
#include <typeinfo>

namespace sequoia::testing
{
  /** \brief A count of line breaks, typed so that it cannot be passed where another count is meant. */
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

  /** \brief `s` between `--` and `--`; empty if `s` is empty. */
  [[nodiscard]]
  std::string emphasise(std::string_view s);

  /** \brief The code unit `c` as a failure report shows it.
      \returns
      -# For an alert, backspace, form feed, newline, carriage return, tab, vertical tab or NUL: its escape sequence,
         in single quotes;
      -# For a space: the space, in single quotes;
      -# For any other printable ASCII character: the character;
      -# Otherwise: the code unit's value as a hexadecimal escape sequence, in single quotes. A byte of a multi-byte
         UTF-8 character shows as, for example, `'\xc3'`, and U+010A shows as `'\x10a'`.
   */
  template<character Char>
  [[nodiscard]]
  std::string display_character(Char c)
  {
    const auto codeUnit{static_cast<std::uint32_t>(static_cast<std::make_unsigned_t<Char>>(c))};
    switch(codeUnit)
    {
    case '\a': return "'\\a'";
    case '\b': return "'\\b'";
    case '\f': return "'\\f'";
    case '\n': return "'\\n'";
    case '\r': return "'\\r'";
    case '\t': return "'\\t'";
    case '\v': return "'\\v'";
    case '\0': return "'\\0'";
    case ' ':  return "' '";
    }

    const bool printableAscii{(codeUnit > ' ') && (codeUnit <= '~')};
    return printableAscii ? std::string(1, static_cast<char>(codeUnit)) : std::format("'\\x{:02x}'", codeUnit);
  }

  /** \brief Appends line breaks until a non-empty `s` ends with at least `newlines` of them, then appends `footer`;
             an empty `s` stays empty.
   */
  void end_block(std::string& s, line_breaks newlines, std::string_view footer="");

  /** \brief `s`, ended as the overload taking a `std::string&` ends it. */
  [[nodiscard]]
  std::string end_block(std::string_view s, line_breaks newlines, std::string_view footer="");

  /** \brief The report of an exception that escaped a test.

      The report gives `tag` and `exceptionMessage`, then:
      -# If `info` holds a top-level check: whether the exception was thrown during that check or after it, and
         the check's message;
      -# Otherwise: the test's `filename`.
   */
  [[nodiscard]]
  std::string exception_message(std::string_view tag,
                                const std::filesystem::path& filename,
                                const uncaught_exception_info& info,
                                std::string_view exceptionMessage);

  /** \brief A message of the form `operator== returned false`. */
  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view retVal);

  /** \brief The obtained and predicted states of a nullable value, each `null` or `not null`, laid out as
             `default_prediction_message` lays out values.
   */
  [[nodiscard]]
  std::string nullable_type_message(bool obtainedHoldsValue, bool predictedHoldsValue);

  [[nodiscard]]
  std::string equality_operator_failure_message();

  /** \brief The message that two pointers, neither null, point to different addresses. */
  [[nodiscard]]
  std::string pointer_prediction_message();

  /** \brief `obtained` after `Obtained : ` and, on the next line, `prediction` after `Predicted: `. */
  [[nodiscard]]
  std::string default_prediction_message(std::string_view obtained, std::string_view prediction);

  [[nodiscard]]
  std::string prediction_message(const std::string& obtained, const std::string& prediction);

  template<character Char>
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
    requires (!character<T> && !std::is_pointer_v<T> && !is_const_pointer_v<T>)
  [[nodiscard]]
  std::string prediction_message(const T& obtained, const T& prediction)
  {
    return default_prediction_message(to_string(obtained), to_string(prediction));
  }

  /** \brief Whether the failure message of a comparison is final, and so shows the obtained and predicted values, or
             is followed by finer-grained checks that show them.
   */
  template<bool IsFinalMessage>
  struct final_message_constant : std::bool_constant<IsFinalMessage> {};

  using is_final_message_t     = final_message_constant<true>;
  using is_not_final_message_t = final_message_constant<false>;

  inline constexpr is_final_message_t is_final_message{};
  inline constexpr is_not_final_message_t is_not_final_message{};

  template<class T>
  concept reportable = serializable<T> || character<T>;

  template<reportable T>
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

  /** \brief The line that ends each failure report. */
  [[nodiscard]]
  std::string footer();

  /** \brief The line that ends the report of an instability: a check whose outcome differs between runs. */
  [[nodiscard]]
  std::string instability_footer();

  /** \brief The file and line of `loc`, the file as `path_for_reporting` gives it, then `message` beneath them. */
  [[nodiscard]]
  std::string report_line(std::string_view message, const std::filesystem::path& repository, const std::source_location loc);

  /** \brief `file` as a report shows it.
      \returns
      -# For a relative `file`: `file` without its leading `..` components;
      -# For an absolute `file` and an absolute `repository`: the name of the directory `repository`, followed by the
         components of `file` after the leading components that `file` shares with the directory `repository`;
      -# Otherwise: `file`.
   */
  [[nodiscard]]
  std::filesystem::path path_for_reporting(const std::filesystem::path& file, const std::filesystem::path& repository);

  struct no_source_location_t{};
  inline constexpr no_source_location_t no_source_location{};

  /** \brief The description of a check, with the source location of the check; by default, the location of the
             constructor's call site. A `reporter` made with `no_source_location` has none.
   */
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

  /** \brief Respells a type name, as `demangle(std::string)` returns it when compiled by clang, into the spelling
             shared by every supported toolchain, so that output which names a type does not depend on the compiler.
             The overload is chosen by compiler, but its respellings are those libc++'s names need.
   */
  [[nodiscard]]
  std::string tidy_name(std::string name, clang_type);

  /** \brief Respells a type name, as `demangle(std::string)` returns it when compiled by gcc, into the shared
             spelling; its respellings are those libstdc++'s names need.
   */
  [[nodiscard]]
  std::string tidy_name(std::string name, gcc_type);

  /** \brief Respells a type name as MSVC's `type_info::name` writes it into the shared spelling. */
  [[nodiscard]]
  std::string tidy_name(std::string name, msvc_type);

  /** \brief `name`, unchanged. */
  [[nodiscard]]
  std::string tidy_name(std::string name, other_compiler_type);

  /** \brief Demangles an Itanium-ABI name; a name that does not demangle, as every name under MSVC, is returned
             unchanged.

      libc++abi's spellings of non-finite floating-point values - `inff` and `infL`, and `nanf`, `nan` and `nanL`,
      which carry no sign - are respelled `inf`, `nan` or `-nan`, as `tidy_name` renders libstdc++'s bit patterns.
      A libc++abi spelling is kept in two cases:
      -# The mangled name may hold an entity with that spelling as its name, such as a type `inff`;
      -# The value is a NaN, and the name holds NaNs of its type with both signs.
   */
  [[nodiscard]]
  std::string demangle(std::string mangled);

  template<class T, invocable_exact_r<std::string, std::string> Tidy>
  [[nodiscard]]
  std::string demangle(Tidy tidy)
  {
    return tidy(demangle({typeid(T).name()}));
  }

  /** \brief The name of the type `info` describes, in the spelling shared by every supported toolchain. */
  [[nodiscard]]
  std::string demangle(const std::type_info& info);

  /** \brief The name of `T` in the spelling shared by every supported toolchain.

      A 32- or 64-bit unsigned integer type is named as the fixed-width type of its size, in the platform's spelling.
   */
  template<class T>
  [[nodiscard]]
  std::string demangle()
  {
    return demangle(typeid(type_normalizer_t<T>));
  }

  /** \brief Specialize this struct template to customize the way in which type info is generated for a given class.
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


  /** \brief The name `type_demangler` makes for `T`; then, for each of `U...`, a comma and, on a new line, the
             name `type_demangler` makes for that type.
   */
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
  std::string add_type_info(std::string description)
  {
    return append_lines(std::move(description), make_type_info<T, U...>());
  }
}
