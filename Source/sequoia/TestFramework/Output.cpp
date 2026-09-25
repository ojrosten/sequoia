////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/Output.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <numeric>
#include <optional>
#include <ranges>
#include <type_traits>

#ifndef _MSC_VER
  #include "cxxabi.h"
#endif

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr auto npos{std::string::npos};
    using size_type = std::string::size_type;

    constexpr auto is_digit{[](char c){ return std::isdigit(static_cast<unsigned char>(c)) != 0; }};
    constexpr auto is_alpha{[](char c){ return std::isalpha(static_cast<unsigned char>(c)) != 0; }};

    constexpr auto is_word_delimiter{
      [](char c){ return !(std::isalnum(static_cast<unsigned char>(c)) || (c == '_')); }
    };
    
    std::string& remove_enum_spec(std::string& name)
    {
      std::string::size_type pos{};

      while(pos != npos)
      {
        const auto[open, close]{find_matched_delimiters(name, '(', ')', pos)};
        if((open != close) && (close < name.size()) && is_digit(name[close]))
        {
          name.erase(open, close - open);
          pos = close;
        }
        else
        {
          break;
        }
      }

      return name;
    }

    /** The value whose bits `hex` spells, most significant digit first, as the Itanium ABI mangles a floating-point
        literal; a pattern shorter than the type is zero-extended. None if `hex` is empty, is wider than the type or
        holds a character that is not a hexadecimal digit.
     */
    template<std::floating_point T>
    [[nodiscard]]
    std::optional<T> from_bit_pattern(std::string_view hex)
    {
      if(hex.empty() || (hex.size() > 2 * sizeof(T)))
        return std::nullopt;

      std::array<std::byte, sizeof(T)> bytes{};
      for(const auto [i, digit] : hex | std::views::reverse | std::views::enumerate)
      {
        unsigned nibble{};
        if(std::from_chars(&digit, &digit + 1, nibble, 16).ec != std::errc{})
          return std::nullopt;

        bytes[i / 2] |= static_cast<std::byte>(nibble << (4 * (i % 2)));
      }

      if constexpr(std::endian::native == std::endian::big)
        std::ranges::reverse(bytes);

      return std::bit_cast<T>(bytes);
    }

    [[nodiscard]]
    std::optional<std::string> format_bit_pattern(std::string_view type, std::string_view hex)
    {
      constexpr auto formatFixed{[](std::floating_point auto value){ return std::format("{:f}", value); }};

      if(type == "float")
        return from_bit_pattern<float>(hex).transform(formatFixed);

      if(type == "double")
        return from_bit_pattern<double>(hex).transform(formatFixed);

      if(type == "long double")
        return from_bit_pattern<long double>(hex).transform(formatFixed);

      return std::nullopt;
    }

    std::string& process_literals(std::string& name)
    {
      std::string::size_type pos{};
      while(pos < name.size())
      {
        const auto open{name.find_first_of("< {", pos)};
        if(open >= name.size())
          break;

        pos = open+1;

        // A reinterpreted floating-point literal, `(float)[FF]`, need not contain a decimal digit
        while((pos < name.size() - 1) && !is_digit(name[pos]) && (name[pos - 1] != '['))
        {
          ++pos;
        }

        if(pos < name.size() - 1)
        {
          if((name[pos - 1] == '_') || is_alpha(name[pos - 1]))
          {
            const auto identifierEnd{std::ranges::find_if(name.begin() + pos, name.end(), is_word_delimiter)};
            pos = std::ranges::distance(name.begin(), identifierEnd);
            continue;
          }

          if(name[pos - 1] == '[')
          {
            // libstdc++'s demangler spells a floating-point literal as its type and its bit pattern:
            // (double)[40687...]
            const auto close{name.find(']', pos)};
            const auto openParen{name.rfind('(', pos - 1)};
            const auto closeParen{pos - 2};
            if((close != npos) && (openParen != npos) && (name[closeParen] == ')'))
            {
              const auto type{std::string_view{name}.substr(openParen + 1, closeParen - openParen - 1)};
              const auto hex{std::string_view{name}.substr(pos, close - pos)};
              if(const auto value{format_bit_pattern(type, hex)})
              {
                name.replace(openParen, close + 1 - openParen, *value);
                pos = openParen + value->size();
              }
              else
              {
                pos = close;
              }
            }
          }

          if(name[pos + 1] == 'x')
          {
            char* end{};
            const auto start{name.data() + pos};
            const auto val{std::strtold(start, &end)};
            if(const auto dist{std::ranges::distance(start, end)}; dist > 0)
            {
              const auto str{std::format("{:f}", val)};
              name.replace(pos, dist, str);
              pos += (str.size() - 1);
            }
          }

          if(pos < name.size() - 1)
          {
            const auto digitsEnd{std::ranges::find_if_not(name.begin() + pos + 1, name.end(), is_digit)};
            pos = std::ranges::distance(name.begin(), digitsEnd);
          }

          if((pos < name.size()) && (pos-1 > open) && is_alpha(name[pos]))
          {
            if(const auto close{name.find_first_of(",>}", pos)}; close < name.size())
            {
              name.erase(pos, close - pos);
              pos++;
            }
          }
            
        }
      }
      
      return name;
    }

    std::string& process_spans(std::string& name)
    {
      if(auto[start, end]{find_sandwiched_text(name, "::span<", ">")}; start != end)
      {
        start = name.find(',', start);
        if(start < end - 1)
        {
          ++start;
          if(name[start] == ' ') ++start;

          auto startPtr{std::ranges::next(name.data(), start)}, endPtr{std::ranges::next(name.data(), end)};
          std::size_t val{};
          const auto[ptr, ec]{std::from_chars(startPtr, endPtr, val)};
          if((ptr == endPtr) && (val == std::numeric_limits<std::size_t>::max()))
          {
            // Use the MSVC convention
            name.replace(start, end-start, "-1");
          }
        }
      }
      
      return name;
    }

    /** Respells the MS STL's array iterators, which are class templates, as the pointers libc++ and libstdc++ use:
        `std::_Array_const_iterator<T, N>` as `T const*` and `std::_Array_iterator<T, N>` as `T*`.
     */
    void process_array_iterators(std::string& name)
    {
      const auto respell{
        [&name](std::string_view opening, std::string_view pointerSuffix){
          for(auto start{name.find(opening)}; start != npos; start = name.find(opening, start))
          {
            const auto [open, close]{find_matched_delimiters(name, '<', '>', start)};
            if(close <= open)
              break;

            const auto arguments{std::string_view{name}.substr(open + 1, close - open - 2)};
            const auto pointer{std::format("{}{}", arguments.substr(0, arguments.rfind(',')), pointerSuffix)};
            name.replace(start, close - start, pointer);

            const auto end{start + pointer.size()};
            if((end + 1 < name.size()) && (name[end] == ' ') && (name[end + 1] == '>'))
              name.erase(end, 1);
          }
        }
      };

      respell("std::_Array_const_iterator<", " const*");
      respell("std::_Array_iterator<",       "*");
    }

    std::string& tidy_name(std::string& name)
    {
      if constexpr(sizeof(unsigned long) == sizeof(unsigned long long))
      {
        // Collapse before expanding; the other way round, the collapse undoes the expansion
        replace_all(name, is_word_delimiter, "long long", is_word_delimiter, "long");
        replace_all(name, is_word_delimiter, "long",      is_word_delimiter, "long long");

        // The expansion cannot see that `long double` is not a `long`
        replace_all(name, is_word_delimiter, "long long double", is_word_delimiter, "long double");
      }

      // It is a pity to have to make the following substitutions, but it appears
      // to be by far the easiest way to ensure compiler-independent de-mangling.

      replace_all(name, " <{", "true",  ",>}", "1");
      replace_all(name, " <{", "false", ",>}", "0");

      remove_enum_spec(name);
      auto openPos{name.find('(')};
      auto pos{openPos};
      std::int64_t open{};
      while(pos != npos)
      {
        if(name[pos] == '(')      ++open;
        else if(name[pos] == ')') --open;

        ++pos;

        if(!open)
        {
          if((pos < name.size()) && is_digit(name[pos]))
            name.erase(openPos, pos-openPos);

          openPos = name.find('(', pos);
          pos = openPos;
        }
      }

      return name;
    }

    /** Whether the mangled name may hold an Itanium source-name - a name's length, then the name - spelling `name`.
        Every occurrence of that text counts, since the digit before one may end a previous name, as in `3ns24inff`.
     */
    [[nodiscard]]
    bool may_hold_source_name(std::string_view mangled, std::string_view name)
    {
      return mangled.contains(std::format("{}{}", name.size(), name));
    }

    /** The code of a floating-point type in an Itanium mangled literal - `L`, the code, the bit pattern, `E` - and
        libc++abi's spelling of a NaN of the type, a spelling without a sign.
     */
    template<std::floating_point T>
    struct itanium_literal;

    template<>
    struct itanium_literal<float>
    {
      constexpr static char             code{'f'};
      constexpr static std::string_view libcxxabi_nan{"nanf"};
    };

    template<>
    struct itanium_literal<double>
    {
      constexpr static char             code{'d'};
      constexpr static std::string_view libcxxabi_nan{"nan"};
    };

    template<>
    struct itanium_literal<long double>
    {
      constexpr static char             code{'e'};
      constexpr static std::string_view libcxxabi_nan{"nanL"};
    };

    /** `nan` or `-nan`, whichever sign every NaN among the mangled name's literals of type `T` shares; none if there
        is no such NaN, or if there are NaNs of both signs.
     */
    template<std::floating_point T>
    [[nodiscard]]
    std::optional<std::string_view> nan_spelling(std::string_view mangled)
    {
      bool positive{}, negative{};
      const std::string literalStart{'L', itanium_literal<T>::code};
      for(auto pos{mangled.find(literalStart)}; pos != npos; pos = mangled.find(literalStart, pos + 1))
      {
        const auto hexStart{pos + literalStart.size()};
        const auto value{from_bit_pattern<T>(mangled.substr(hexStart, mangled.find('E', hexStart) - hexStart))};
        if(value && std::isnan(*value))
          (std::signbit(*value) ? negative : positive) = true;
      }

      if(positive == negative)
        return std::nullopt;

      return negative ? "-nan" : "nan";
    }

    /** The respelling of libc++abi's non-finite literals that `demangle` promises. */
    [[nodiscard]]
    std::string respell_non_finite_literals(std::string demangled, std::string_view mangled)
    {
      const auto respell{
        [&demangled, mangled](std::string_view from, std::string_view to){
          if(!may_hold_source_name(mangled, from))
            replace_all(demangled, is_word_delimiter, from, is_word_delimiter, to);
        }
      };

      const auto respellNans{
        [&respell, mangled]<std::floating_point T>(std::type_identity<T>){
          if(const auto spelling{nan_spelling<T>(mangled)})
            respell(itanium_literal<T>::libcxxabi_nan, *spelling);
        }
      };

      respell("inff", "inf");
      respell("infL", "inf");

      respellNans(std::type_identity<double>{});
      respellNans(std::type_identity<float>{});
      respellNans(std::type_identity<long double>{});

      return demangled;
    }

    /** The position of the colon with which MSVC ends the type of the member of a class-type template argument that
        starts at `start`; npos if that member has no type before its value.
     */
    [[nodiscard]]
    size_type member_type_end(std::string_view name, size_type start)
    {
      constexpr std::string_view delimiters{"<>(){},:"};
      std::size_t depth{};
      for(auto pos{name.find_first_of(delimiters, start)}; pos != npos; pos = name.find_first_of(delimiters, pos + 1))
      {
        switch(name[pos])
        {
        case '<':
        case '(':
          ++depth;
          break;
        case '>':
        case ')':
          if(!depth)
            return npos;
          --depth;
          break;
        case ':':
          if((pos + 1 < name.size()) && (name[pos + 1] == ':'))
            ++pos;
          else if(!depth)
            return pos;
          break;
        default:
          if(!depth)
            return npos;
        }
      }

      return npos;
    }

    /** Removes the type MSVC writes before each member of a class-type template argument: `{int:1,float:2.000000}`
        becomes `{1,2.000000}`.
     */
    std::string& remove_member_types(std::string& name)
    {
      for(auto pos{name.find_first_of("{,")}; pos != npos; pos = name.find_first_of("{,", pos + 1))
      {
        if(const auto end{member_type_end(name, pos + 1)}; end != npos)
          name.erase(pos + 1, end - pos);
      }

      return name;
    }

    [[nodiscard]]
    std::string nullable_type_message(const bool holdsValue)
    {
      return std::string{holdsValue ? "not " : ""}.append("null");
    }
  }

  [[nodiscard]]
  std::string footer()
  {
    return "=======================================\n";
  }

  [[nodiscard]]
  std::string instability_footer()
  {
    return "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n";
  }

  void end_block(std::string& s, const line_breaks newlines, std::string_view footer)
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
  std::string end_block(std::string_view s, const line_breaks newlines, std::string_view footer)
  {
    std::string text{s};
    end_block(text, newlines, footer);

    return text;
  }

  [[nodiscard]]
  std::string emphasise(std::string_view s)
  {
    if(s.empty()) return "";

    constexpr std::string_view emph{"--"};
    return std::string{emph}.append(s).append(emph);
  }

  [[nodiscard]]
  std::string exception_message(std::string_view tag,
                                const fs::path& filename,
                                const uncaught_exception_info& info,
                                std::string_view exceptionMessage)
  {
    auto mess{append_lines(std::string{"Error -- "}.append(tag).append(" Exception:"), exceptionMessage).append("\n")};

    const auto& currentMessage{info.top_level_message};
    if(!currentMessage.empty())
    {
      std::string_view suffix{info.num ? "during last check" : "after check completed"};
      append_lines(mess, std::string{"Exception thrown "}.append(suffix), "Last Recorded Message:\n", currentMessage);
    }
    else
    {
      append_lines(mess, "Exception thrown before any checks performed in file", filename.generic_string());
    }

    return mess;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view opRetVal)
  {
    return std::string{"operator"}.append(op).append(" returned ").append(opRetVal);
  }

  [[nodiscard]]
  std::string equality_operator_failure_message()
  {
    return operator_message("==", "false");
  }

  [[nodiscard]]
  std::string pointer_prediction_message()
  {
    return "Pointers both non-null, but they point to different addresses";
  }

  [[nodiscard]]
  std::string default_prediction_message(std::string_view obtained, std::string_view prediction)
  {
    return append_lines(std::string{"Obtained : "}.append(obtained), std::string{"Predicted: "}.append(prediction));
  }

  [[nodiscard]]
  std::string prediction_message(const std::string& obtained, const std::string& prediction)
  {
    return default_prediction_message(obtained, prediction);
  }

  [[nodiscard]]
  std::string nullable_type_message(const bool obtainedHoldsValue, const bool predictedHoldsValue)
  {
    return std::string{"Obtained : "}.append(nullable_type_message(obtainedHoldsValue)).append("\n")
               .append("Predicted: ").append(nullable_type_message(predictedHoldsValue));
  }

  [[nodiscard]]
  fs::path path_for_reporting(const fs::path& file, const fs::path& repository)
  {
    if(file.is_relative())
    {
      auto it{std::ranges::find_if_not(file, [](const fs::path& p) { return p == ".."; })};
      return std::accumulate(it, file.end(), fs::path{}, [](fs::path lhs, const fs::path& rhs){ return lhs /= rhs; });
    }
    else if(!repository.empty())
    {
      auto [filepathIter, repoIter]{std::ranges::mismatch(file, repository)};
      return std::accumulate(filepathIter, file.end(), back(repository), [](fs::path lhs, const fs::path& rhs){ return lhs /= rhs; });
    }

    return file;
  }

  [[nodiscard]]
  std::string report_line(std::string_view message, const fs::path& repository, const std::source_location loc)
  {
    return append_lines(path_for_reporting(loc.file_name(), repository).generic_string().append(", Line ").append(std::to_string(loc.line())), message).append("\n");
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, clang_type)
  {
    replace_all(name, "::__1::", "::");
    replace_all(name, "::__fs::", "::");
    replace_all_recursive(name, ">>", "> >");
    process_literals(name);
    process_spans(name);

    return tidy_name(name);
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, gcc_type)
  {
    replace_all(name, "__cxx11::", "");
    replace_all(name, "_V2::", "");  
    replace_all_recursive(name, ">>", "> >");
    process_literals(name);
    process_spans(name);

    return tidy_name(name);
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, msvc_type)
  {
    auto peel{
      [](std::string& s, std::string_view prefix){
        if(s.size() >= prefix.size())
        {
          std::string_view sv{s};
          auto start{sv.substr(0, prefix.size())};
          if(start == prefix)
            s.erase(0, prefix.size());
        }
      }
    };

    peel(name, "struct ");
    peel(name, "class ");
    peel(name, "enum ");

    replace_all(name, "<,{", "struct ", "", "");
    replace_all(name, "<,{", "class ", "", "");
    replace_all(name, "<,{", "enum ", "", "");

    remove_member_types(name);
    replace_all(name, "nan(ind)", "nan");

    replace_all(name, ",", ", ");
    replace_all(name, " ,", ",");

    replace_all(name, " & __ptr64", "&");
    replace_all(name, " * __ptr64", "*");

    replace_all(name, "`anonymous namespace'", "(anonymous namespace)");

#ifdef _MSC_VER
    if constexpr(sizeof(__int64) == sizeof(long))
    {
      replace_all(name, "__int64", "long");
    }
    else if constexpr(sizeof(__int64) == sizeof(long long))
    {
      replace_all(name, "__int64", "long long");
    }
#endif

    replace_all(name, "__cdecl(void)", "()");
    replace_all(name, "__cdecl", "");
    replace_all(name, ")(void)", ")()");

    process_array_iterators(name);

    return name;
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, other_compiler_type)
  {
    return name;
  }

  [[nodiscard]]
  std::string demangle(std::string mangled)
  {
    if constexpr(with_clang_v || with_gcc_v)
    {
      struct cxa_demangler
      {
        cxa_demangler(const std::string& name)
          : data{demangle(name)}
        {}

        ~cxa_demangler() { std::free(data); }

#ifndef _MSC_VER
        char* demangle(const std::string& name)
        {
          return abi::__cxa_demangle(name.data(), 0, 0, &status);
        }
#else
        char* demangle(const std::string&) { return nullptr; }
#endif

        int status{-1};
        char* data;
      };

      cxa_demangler c{mangled};

      if(!c.status)
        return respell_non_finite_literals(c.data, mangled);
    }

    return mangled;
  }
}
