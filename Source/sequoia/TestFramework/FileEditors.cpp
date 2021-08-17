////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileEditors.hpp
 */

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"
#include "sequoia/TestFramework/Output.hpp"

#include <fstream>
#include <regex>

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& file, std::string_view includePath)
  {
    auto inserter{
      [&includePath](std::string& text) {

        std::string_view include{"#include"};
        std::vector<std::string> entries{std::string{include}.append(" \"").append(includePath).append("\"\n")};

        constexpr auto npos{std::string::npos};

        const auto lastIncludePos{text.rfind(include)};
        const auto endBlock{std::min(text.find('\n', lastIncludePos), text.size())};
        if(lastIncludePos != npos)
        {
          std::string::size_type start{npos}, end{};
          while((start = text.find(include, end)) < endBlock)
          {
            end = text.find('\n', start);
            if(end <= endBlock)
            {
                entries.push_back(text.substr(start, end + 1 - start));
            }
          }
        }

        std::sort(entries.begin(), entries.end(), [](const std::string& lhs, const std::string& rhs) {
            auto lAnglePos{lhs.find('<')}, rAnglePos{rhs.find('>')};
            if((lAnglePos < npos) && (rAnglePos == npos)) return false;
            if((lAnglePos == npos) && (rAnglePos < npos)) return true;

            return lhs < rhs;
          });

        std::string sorted{};
        std::for_each(entries.begin(), entries.end(), [&sorted](const std::string& e) { sorted.append(e); });

        if(const auto firstIncludePos{text.find(include)}; firstIncludePos < npos)
        {
          text.replace(firstIncludePos, endBlock + 1 - firstIncludePos, sorted);
        }
        else
        {
          text.insert(endBlock, sorted);
        }
      }
    };

    read_modify_write(file, inserter);
  }

  void add_to_family(const std::filesystem::path& file, std::string_view familyName, indentation indent, const std::vector<std::string>& tests)
  {
    namespace fs = std::filesystem;

    if(tests.empty())
      throw std::logic_error{std::string{"No tests specified to be added to the test family \""}.append(familyName).append("\"")};

    const auto text{
      [&file, familyName, &tests, indent]() -> std::string {
        constexpr auto npos{std::string::npos};
        const auto pattern{std::string{"\""}.append(familyName).append("\",")};
        auto contents{read_to_string(file)};
        if(!contents)
          throw std::runtime_error{report_failed_read(file)};

        std::string& text{contents.value()};
        if(auto pos{text.find(pattern)}; pos != npos)
        {
          if(const auto linePos{text.rfind('\n', pos)}; linePos != npos)
          {
            std::string_view preamble{"add_test_family("};
            if((linePos > preamble.size()) && (text.find(preamble, linePos - preamble.size()) != npos))
            {
              if(const auto nextLinePos{text.find('\n', pos)}; nextLinePos != npos)
              {
                const auto endpos{text.find(");", pos)};
                for(const auto& t : tests)
                {
                  std::string_view textView{text};
                  std::string_view subtextView{textView.substr(pos, endpos - pos)};
                  if(subtextView.find(t) == npos)
                  {
                    text.insert(nextLinePos + 1, std::string{indent + indent + indent}.append(t).append(",\n"));
                  }
                }

                return text;
              }
            }
          }
        }
        else if(pos = text.find("runner.execute"); pos != npos)
        {
          if(const auto linePos{text.rfind('\n', pos)}; linePos != npos)
          {
            auto builder{
              [&tests, familyName, indent](){
                const indentation indent_0{indent + indent};
                auto str{std::string{"\n"}.append(indent_0).append("runner.add_test_family(")};
                const indentation indent_1{indent_0 + indent};

                append_indented(str, std::string{"\""}.append(familyName).append("\","), indent_1);

                for(auto i{tests.cbegin()}; i != tests.cend() - 1; ++i)
                {
                  append_indented(str, std::string{*i}.append(","), indent_1);
                }

                append_indented(str, tests.back(), indent_1);
                append_indented(str, ");\n", indent_0);

                return str;
              }
            };

            text.insert(linePos, builder());
            return text;
          }
        }

        return "";
      }()
    };

    if(text.empty())
    {
      throw std::runtime_error{"Unable to find appropriate place to add test family"};
    }

    write_to_file(file, text);
  }

  void add_to_cmake(const std::filesystem::path& cmakeDir,
                    const std::filesystem::path& hostDir,
                    const std::filesystem::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrexfix)
  {
    const auto cmakeLists{cmakeDir / "CMakeLists.txt"};

    auto addEntry{
      [file{file.lexically_relative(hostDir)}, &cmakeLists, patternOpen, patternClose, cmakeEntryPrexfix] (std::string& text) {
        constexpr auto npos{std::string::npos};

        if(auto startPos{text.find(patternOpen)}; startPos != npos)
        {
          if(auto endPos{text.find(patternClose, startPos + patternOpen.size())}; endPos != npos)
          {
            std::vector<std::string> entries{{std::string{cmakeEntryPrexfix}.append(file.generic_string())}};
            auto newlinePos{npos}, next{startPos + patternOpen.size()};
            while((newlinePos = text.find("\n", next)) < endPos)
            {
              next = std::min(text.find("\n", newlinePos + 1), endPos);
              const auto entryStart{text.find_first_not_of(' ', newlinePos+1)};

              entries.push_back(text.substr(entryStart, next - entryStart));
            }

            std::sort(entries.begin(), entries.end());
            std::string sorted{};
            std::for_each(entries.begin(), entries.end(), [&sorted, patternOpen](const std::string& e) {
              sorted.append("\n").append(patternOpen.size(), ' ').append(e); });

            const auto startSection{std::min(text.find("\n", startPos + patternOpen.size()), endPos)};
            text.replace(startSection, endPos - startSection, sorted);

            return;
          }
        }

        throw std::runtime_error{std::string{"Unable to find appropriate place to add source file to "}.append(cmakeLists.generic_string())};
      }
    };

    read_modify_write(cmakeLists, addEntry);
  }

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction)
  {
    reduced_file_contents contents{read_to_string(file), read_to_string(prediction)};

    if(contents.working && contents.prediction)
    {
      if(file.extension() != seqpat)
      {
        namespace fs = std::filesystem;
        auto supplPath{[](fs::path f) { return f.replace_extension(seqpat); }(prediction)};
        if(fs::exists(supplPath))
        {
          if(auto exprContents{read_to_string(supplPath)})
          {
            const auto& expressions{exprContents.value()};

            std::string::size_type pos{};
            while(pos < expressions.size())
            {
              const auto next{std::min(expressions.find("\n", pos), expressions.size())};
              if(const auto count{next - pos})
              {
                std::basic_regex rgx{expressions.data() + pos, count};
                contents.working = std::regex_replace(contents.working.value(), rgx, std::string{});
                contents.prediction = std::regex_replace(contents.prediction.value(), rgx, std::string{});
                pos = next + 1;
              }
              else
              {
                break;
              }
            }
          }
          else
          {
            throw std::runtime_error{report_failed_read(supplPath)};
          }
        }
      }
    }

    return contents;
  }
}
