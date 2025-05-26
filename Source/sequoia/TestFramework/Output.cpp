////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Output.hpp
 */

#include "sequoia/TestFramework/Output.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <bit>
#include <charconv>
#include <cstdlib>
#include <format>
#include <numeric>
#include <sstream>

#ifndef _MSC_VER
  #include <cxxabi.h>
#endif

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr auto npos{std::string::npos};
    using size_type = std::string::size_type;
    
    std::string& remove_enum_spec(std::string& name)
    {
      std::string::size_type pos{};

      while(pos != npos)
      {
        const auto[open, close]{find_matched_delimiters(name, '(', ')', pos)};
        if((open != close) && (close < name.size()) && std::isdigit(name[close]))
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

    template<std::floating_point To, std::integral From=std::conditional_t<std::is_same_v<To,float>, int, std::conditional_t<sizeof(double) == sizeof(long), long, long long>>>
    size_type hex_to_floating_point(std::string& name, size_type start, size_type end, long long val)
    {
       name.erase(start, end - start);
       const auto str{std::format("{:f}", std::bit_cast<To>(static_cast<From>(val)))};
       name.insert(start, str);
       return start + str.size();
    }
  
    std::string& process_literals(std::string& name)
    {
      std::string::size_type pos{};
      while(pos < name.size())
      {
        const auto open{name.find_first_of("< ", pos)};
        pos = open;
        while((pos < name.size() - 1) && !std::isdigit(name[++pos])) {}
        if(pos < name.size() - 1)
        {
          if((name[pos - 1] == '_') || std::isalpha(name[pos - 1]))
          {
            pos = name.find_first_of(",>", pos);
            continue;
          }

          if(name[pos - 1] == '[')
          {
            // GCC seems to represent floating-point NTTPs as
            // a reinterpretation of a implicit hex number
            // e.g. (double)[40687....] (note: no Ox);
            const auto close{name.find(']', pos)};
            if(close != npos)
            {
              std::stringstream ss{name.substr(pos, close - pos)};
              long long hexNum{};
              if(ss >> std::hex >> hexNum)
              {
                const auto closeParen{pos - 2};
                if(auto openParen{name.rfind('(', pos - 1)}; (openParen != npos) && name[closeParen] == ')')
                {
                  const auto type{name.substr(openParen + 1, closeParen - openParen - 1)};
                  if(type == "float")
                  {
                    pos = hex_to_floating_point<float>(name, openParen, close + 1, hexNum);
                  }
                  else if(type == "double")
                  {
                    pos = hex_to_floating_point<double>(name, openParen, close + 1, hexNum);
                  }
                  else
                  {
                    pos = close;
                  }
                }
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

          while((pos < name.size() - 1) && std::isdigit(name[++pos])) {}
        
          if((pos < name.size()) && (pos-1 > open) && std::isalpha(name[pos]))
          {
            if(const auto close{name.find_first_of(",>", pos)}; close < name.size())
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

    std::string& tidy_name(std::string& name)
    {
      if constexpr(sizeof(unsigned long) == sizeof(unsigned long long))
      {
        // Do this first, to avoid the second replace_all potentially 
        // leading to unsigned long long long long (!)
        replace_all(name, "long long", "long");
        replace_all(name, "long", "long long");
      }

      // It is a pity to have to make the following substitutions, but it appears
      // to be by far the easiest way to ensure compiler-independent de-mangling.

      replace_all(name, " <", "true", ",>", "1");
      replace_all(name, " <", "false", ",>", "0");

      remove_enum_spec(name);
      auto openPos{name.find('(')};
      auto pos{openPos};
      int64_t open{};
      while(pos != npos)
      {
        if(name[pos] == '(')      ++open;
        else if(name[pos] == ')') --open;

        ++pos;

        if(!open)
        {
          if((pos < name.size()) && std::isdigit(name[pos]))
            name.erase(openPos, pos-openPos);

          openPos = name.find('(', pos);
          pos = openPos;
        }
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
    replace_all(name, ">>", ">> ");
    replace_all(name, "__cxx11::", "");
    replace_all(name, "_V2::", "");
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

    replace_all(name, "<,", "struct ", "", "");
    replace_all(name, "<,", "class ", "", "");
    replace_all(name, "<,", "enum ", "", "");

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

      if(!c.status) mangled = c.data;
    }

    return mangled;
  }
}
