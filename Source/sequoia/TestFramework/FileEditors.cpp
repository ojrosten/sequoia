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
#include <format>
#include <fstream>
#include <functional>
#include <ranges>
#include <regex>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  void add_include(const fs::path& file, std::string_view includePath)
  {
    auto inserter{
      [&includePath](std::string& text) {

        std::string_view include{"#include"};
        std::vector<std::string> entries{std::format("{} \"{}\"\n", include, includePath)};

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

  void add_test_registrations(const fs::path& file, const std::vector<std::string>& tests)
  {
    if(tests.empty())
      throw std::logic_error{"No tests specified for registration"};

    auto contents{read_to_string(file, std::ios_base::in)};
    if(!contents)
      throw std::runtime_error{report_failed_read(file)};

    std::string& contentsStr{contents.value()};

    constexpr auto npos{std::string::npos};
    const auto executionPos{contentsStr.find("runner.execute")};
    if(executionPos == npos)
      throw std::runtime_error{std::format("Unable to find the point of registration in {}", file.generic_string())};

    const auto executionLineStart{
      [&contentsStr, executionPos]() -> std::string::size_type {
        const auto newlinePos{contentsStr.rfind('\n', executionPos)};
        return newlinePos == npos ? 0 : newlinePos + 1;
      }()
    };

    // The registrations go straight after the last line before the execution's which holds anything.
    // Placing them after the last *registration* would put them inside an `#if` or a block which ends
    // just before the execution, making them conditional; this way the blank lines which separate
    // the registrations from the execution also stay where they are.
    const auto insertionPos{
      [&contentsStr, executionLineStart]() -> std::string::size_type {
        if(executionLineStart == 0)
          return 0;

        constexpr std::string_view blanksAndNewlines{" \t\r\n"};
        const auto lastNonBlankPos{contentsStr.find_last_not_of(blanksAndNewlines, executionLineStart - 1)};
        if(lastNonBlankPos == npos)
          return 0;

        const auto lineEnd{contentsStr.find('\n', lastNonBlankPos)};
        return lineEnd + 1;
      }()
    };

    constexpr std::string_view blanks{" \t\r"};
    std::string_view contentsView      {contentsStr};
    const auto       executionIndentEnd{contentsView.find_first_not_of(blanks, executionLineStart)};
    std::string_view executionIndent   {contentsView.substr(executionLineStart, executionIndentEnd - executionLineStart)};

    auto registrationOf{
      [](std::string_view test) { return std::format("runner.register_test<{}>();", test); }
    };

    auto isRegistered{
      [contentsView, blanks, registrationOf](std::string_view test) {
        const auto registration{registrationOf(test)};
        auto startsWithRegistration{
          [&registration, blanks](auto&& line) {
            std::string_view lineView    {std::ranges::begin(line), std::ranges::end(line)};
            const auto       contentStart{std::ranges::min(lineView.find_first_not_of(blanks), lineView.size())};
            return lineView.substr(contentStart).starts_with(registration);
          }
        };

        return std::ranges::any_of(contentsView | std::views::split('\n'), startsWithRegistration);
      }
    };

    auto registrationLineOf{
      [executionIndent, registrationOf](std::string_view test) {
        return std::format("{}{}\n", executionIndent, registrationOf(test));
      }
    };

    const auto registrations{
        tests
      | std::views::filter(std::not_fn(isRegistered))
      | std::views::transform(registrationLineOf)
      | std::views::join
      | std::ranges::to<std::string>()
    };

    if(registrations.empty())
      return;

    contentsStr.insert(insertionPos, registrations);
    write_to_file(file, contentsStr, std::ios_base::out);
  }

  void add_to_cmake(const fs::path& cmakeLists,
                    const fs::path& hostDir,
                    const fs::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrefix)
  {
    const auto parenthesisPos{patternOpen.find('(')};
    if(parenthesisPos == std::string_view::npos)
      throw std::logic_error{
        std::format("No parenthesis in '{}' to align the entries of {} with", patternOpen, cmakeLists.generic_string())
      };

    auto addEntry{
      [file{file.lexically_relative(hostDir)}, &cmakeLists, patternOpen, patternClose, cmakeEntryPrefix,
       numSpaces{parenthesisPos + 1}] (std::string& text) {
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

            std::ranges::sort(entries);
            std::string sorted{};
            std::ranges::for_each(entries, [&sorted, numSpaces](const std::string& e) {
              sorted.append(std::format("\n{:{}}{}", "", numSpaces, e)); });

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

    /** \brief A line of at least one character, every one a space or a tab. A carriage return is not among
        them: CRLF line endings are normalised to LF before a `.seqpat` is split into lines.
     */
    [[nodiscard]]
    bool is_whitespace_only(std::string_view line) noexcept
    {
      return !line.empty() && (line.find_first_not_of(" \t") == std::string_view::npos);
    }

    [[nodiscard]]
    std::string seqpat_error_message(const fs::path& seqpatFile, std::size_t line, std::string_view problem)
    {
      return std::format("Line {} of a .seqpat {}\n{}", line, problem, seqpatFile.generic_string());
    }

    /** \brief The regular expression `pattern`, from `line` of `seqpatFile`.

        \throws std::runtime_error naming the line and the file if `pattern` holds only whitespace, or is
                not a valid regular expression
     */
    [[nodiscard]]
    std::regex seqpat_regex(std::string_view pattern, const fs::path& seqpatFile, std::size_t line)
    {
      if(is_whitespace_only(pattern))
      {
        throw std::runtime_error{
          seqpat_error_message(
            seqpatFile,
            line,
            "holds only whitespace, which is ambiguous: delete the line, or write the pattern explicitly, "
            "such as [ ] or [ \\t]"
          )
        };
      }

      try
      {
        return std::regex{pattern.begin(), pattern.end()};
      }
      catch(const std::regex_error&)
      {
        // Not std::regex_error's own message: each standard library words it differently
        throw std::runtime_error{
          seqpat_error_message(seqpatFile, line, std::format("is not a valid regular expression: {}", pattern))
        };
      }
    }
  }

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const fs::path& file, const fs::path& prediction)
  {
    constexpr auto binary{std::ios_base::in | std::ios_base::binary};

    reduced_file_contents contents{read_to_string(file, binary), read_to_string(prediction, binary)};

    if(contents.working && contents.prediction)
    {
      normalize_line_endings(contents.working.value());
      normalize_line_endings(contents.prediction.value());

      if(file.extension() != seqpat)
      {
        auto supplPath{[](fs::path f) { return f.replace_extension(seqpat); }(prediction)};
        if(fs::exists(supplPath))
        {
          if(auto exprContents{read_to_string(supplPath, binary)})
          {
            auto& expressions{exprContents.value()};
            normalize_line_endings(expressions);

            for(const auto [index, text] : std::views::split(expressions, '\n') | std::views::enumerate)
            {
              std::string_view pattern{text};
              if(pattern.empty())
                continue;

              const auto rgx{seqpat_regex(pattern, supplPath, static_cast<std::size_t>(index) + 1)};
              contents.working    = std::regex_replace(contents.working.value(),    rgx, std::string{});
              contents.prediction = std::regex_replace(contents.prediction.value(), rgx, std::string{});
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
