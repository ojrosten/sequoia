////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>
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
        const auto endBlock{std::ranges::min(text.find('\n', lastIncludePos), text.size())};
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

        // Standard headers first.  Cosmetic today; load-bearing the moment the
        // modules migration lands, because a quoted include here names a test
        // header and a test header will import the framework.  A textual
        // #include *after* an import re-parses whatever the module carried in
        // its global module fragment - the include guards were consumed in the
        // module's preprocessing context, not this one - and everything those
        // headers define is then defined twice.  Angled before quoted keeps
        // every textual include ahead of the first import.
        std::ranges::sort(entries, [](const std::string& lhs, const std::string& rhs) {
            const auto lAnglePos{lhs.find('<')}, rAnglePos{rhs.find('<')};
            if((lAnglePos < npos) && (rAnglePos == npos)) return true;
            if((lAnglePos == npos) && (rAnglePos < npos)) return false;

            return lhs < rhs;
          });

        const std::string sorted{
          [&entries](){
            std::string s{};
            std::ranges::for_each(entries, [&s](const std::string& e) { s.append(e); });
            return s;
          }()
        };

        if(const auto firstIncludePos{text.find(include)}; firstIncludePos < npos)
        {
          text.replace(firstIncludePos, endBlock + 1 - firstIncludePos, sorted);
        }
        else
        {
          // Nothing to extend.  Anchor on the first import rather than on endBlock:
          // with no #include anywhere in the file, rfind yields npos and endBlock
          // degenerates to text.size(), which appends the block after main() and
          // leaves whatever the header declares undeclared at every point of use.
          // The ordering above says includes belong ahead of the first import, and
          // that is where they go when there is no block to join.
          constexpr std::string_view importDecl{"import "};
          const auto firstImportPos{
            [&text, importDecl]() {
              if(text.starts_with(importDecl)) return std::string::size_type{};

              const auto pos{text.find(std::string{'\n'}.append(importDecl))};
              return pos == npos ? npos : pos + 1;
            }()
          };

          if(firstImportPos < npos)
            text.insert(firstImportPos, std::string{sorted}.append("\n"));
          else
            text.insert(endBlock, sorted);
        }
      }
    };

    read_modify_write(file, inserter);
  }

  void add_test_registrations(const std::filesystem::path& file, indentation indent, const std::vector<std::string>& tests)
  {
    if(tests.empty())
      throw std::logic_error{"No tests specified for registration"};

    auto contents{read_to_string(file, std::ios_base::in)};
    if(!contents)
      throw std::runtime_error{report_failed_read(file)};

    std::string& contentsStr{contents.value()};

    const auto pos{contentsStr.find("runner.execute")};
    if(pos == std::string::npos)
      throw std::runtime_error{std::string{"Unable to find the point of registration in "}.append(file.generic_string())};

    const auto linePos{contentsStr.rfind('\n', pos)};
    if(linePos == std::string::npos)
      throw std::runtime_error{std::string{"Unable to find the point of registration in "}.append(file.generic_string())};

    auto registrations{
      [&tests, &contentsStr, indent](){
        std::string str{};
        for(const auto& test : tests)
        {
          auto registration{std::string{"runner.register_test<"}.append(test).append(">();")};
          if(contentsStr.find(registration) == std::string::npos)
            append_indented(str, registration, indent + indent);
        }

        return str;
      }()
    };

    if(registrations.empty()) return;

    contentsStr.insert(linePos, registrations);
    write_to_file(file, contentsStr, std::ios_base::out);
  }

  void add_to_cmake(const std::filesystem::path& cmakeLists,
                    const std::filesystem::path& hostDir,
                    const std::filesystem::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrefix)
  {
    auto addEntry{
      [file{file.lexically_relative(hostDir)}, &cmakeLists, patternOpen, patternClose, cmakeEntryPrefix] (std::string& text) {
        constexpr auto npos{std::string::npos};

        if(auto startPos{text.find(patternOpen)}; startPos != npos)
        {
          if(auto endPos{text.find(patternClose, startPos + patternOpen.size())}; endPos != npos)
          {
            std::vector<std::string> entries{{std::string{cmakeEntryPrefix}.append(file.generic_string())}};
            auto newlinePos{npos}, next{startPos + patternOpen.size()};
            while((newlinePos = text.find("\n", next)) < endPos)
            {
              next = std::ranges::min(text.find("\n", newlinePos + 1), endPos);
              const auto entryStart{text.find_first_not_of(' ', newlinePos+1)};

              entries.push_back(text.substr(entryStart, next - entryStart));
            }

            const auto numSpaces{
              [patternOpen]() {
                if(const auto pos{patternOpen.find('(')}; pos < std::string_view::npos)
                  return pos + 1;

                return patternOpen.size();
              }()
            };

            std::ranges::sort(entries);
            std::string sorted{};
            std::ranges::for_each(entries, [&sorted, numSpaces](const std::string& e) {
              sorted.append("\n").append(numSpaces, ' ').append(e); });

            const auto startSection{std::ranges::min(text.find("\n", startPos + patternOpen.size()), endPos)};
            text.replace(startSection, endPos - startSection, sorted);

            return;
          }
        }

        throw std::runtime_error{std::string{"Unable to find appropriate place to add source file to "}.append(cmakeLists.generic_string())};
      }
    };

    read_modify_write(cmakeLists, addEntry);
  }

  namespace
  {
    /** \brief Contents with no NUL byte, which is the discriminator git uses too. */
    [[nodiscard]]
    bool is_text(std::string_view contents)
    {
      return contents.find('\0') == std::string_view::npos;
    }

    /** \brief Text differing only in CRLF versus LF is the same text: the line ending belongs to the tool
        which last wrote the file, not to the content.
     */
    void normalize_line_endings(std::string& contents)
    {
      if(is_text(contents)) replace_all(contents, "\r\n", "\n");
    }
  }

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction)
  {
    constexpr auto binary{std::ios_base::in | std::ios_base::binary};

    reduced_file_contents contents{read_to_string(file, binary), read_to_string(prediction, binary)};

    if(contents.working && contents.prediction)
    {
      normalize_line_endings(contents.working.value());
      normalize_line_endings(contents.prediction.value());

      if(file.extension() != seqpat)
      {
        namespace fs = std::filesystem;
        auto supplPath{[](fs::path f) { return f.replace_extension(seqpat); }(prediction)};
        if(fs::exists(supplPath))
        {
          if(auto exprContents{read_to_string(supplPath, binary)})
          {
            auto& expressions{exprContents.value()};
            normalize_line_endings(expressions);

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
