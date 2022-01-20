////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Output.hpp.
 */

#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#ifndef _MSC_VER
  #include <cxxabi.h>
#endif

namespace sequoia::testing
{
  namespace
  {
    std::string& remove_enum_spec(std::string& name)
    {
      std::string::size_type pos{};

      while(pos != std::string::npos)
      {
        const auto[open, close]{find_matched_delimiters(name, pos)};
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

    std::string& tidy_name(std::string& name)
    {
      if constexpr(sizeof(std::size_t) == sizeof(uint64_t))
      {
        const auto replacement{demangle<uint64_t>([](std::string name) -> std::string { return name; })};
        replace_all(name, "", "unsigned long", ",>", replacement);
      }

      // It is a pity to have to make the following substitutions, but it appears
      // to be by far the easiest way to ensure compiler-independent de-mangling.

      replace_all(name, " <", "true", ",>", "1");
      replace_all(name, " <", "false", ",>", "0");
      replace_all(name, [](char c) { return std::isdigit(c) > 0; }, "ul", [](char) { return true; }, "");

      remove_enum_spec(name);
      constexpr auto npos{std::string::npos};
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
      for(; n < std::min(s.size(), newlines.value()); ++n)
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
                                const std::filesystem::path& filename,
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
  std::string failure_message(is_final_message_t, bool, bool)
  {
    return "check failed";
  }

  [[nodiscard]]
  std::string report_line(const std::filesystem::path& file, const int line, std::string_view message, const std::filesystem::path& repository)
  {
    auto pathToString{
      [&file,&repository](){
        if(file.is_relative())
        {
          auto it{file.begin()};
          while(it != file.end() && (*it == ".."))
          {
            ++it;
          }

          std::filesystem::path p{};
          for(; it != file.end(); ++it)
          {
            p /= *it;
          }

          return p.generic_string();
        }
        else if(!repository.empty())
        {
          auto filepathIter{file.begin()}, repoIter{repository.begin()};
          while((repoIter != repository.end()) && (filepathIter != file.end()))
          {
            if(*repoIter != *filepathIter) break;

            ++repoIter;
            ++filepathIter;
          }

          std::filesystem::path p{*(--repository.end())};
          for(; filepathIter != file.end(); ++filepathIter)
          {
            p /= *filepathIter;
          }

          return p.generic_string();
        }

        return file.generic_string();
      }

    };

    return append_lines(pathToString().append(", Line ").append(std::to_string(line)), message).append("\n");
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, clang_type)
  {
    replace_all(name, "::__", "::", "");
    return tidy_name(name);
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, gcc_type)
  {
    replace_all(name, ">>", ">> ");
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

    replace_all(name, " & __ptr64", "&");

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
