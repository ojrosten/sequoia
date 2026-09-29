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

  namespace
  {
    constexpr std::string_view blanks{" \t\r"};

    [[nodiscard]]
    std::string include_directive(std::string_view includePath)
    {
      return std::format("#include \"{}\"", includePath);
    }

    [[nodiscard]]
    std::string registration_of(std::string_view test)
    {
      return std::format("runner.register_test<{}>();", test);
    }

    [[nodiscard]]
    std::string_view without_leading_blanks(std::string_view line)
    {
      return line.substr(std::ranges::min(line.find_first_not_of(blanks), line.size()));
    }

    /** \brief Removes from `file` each line which `shouldRemove` accepts, and says whether there was one. */
    template<std::predicate<std::string_view> ShouldRemove>
    bool remove_lines(const fs::path& file, ShouldRemove shouldRemove)
    {
      const auto contents{read_to_string(file, std::ios_base::in)};
      if(!contents)
        throw std::runtime_error{report_failed_read(file)};

      auto isKept{
        [&shouldRemove](auto&& line) { return !shouldRemove(std::string_view{std::ranges::begin(line), std::ranges::end(line)}); }
      };

      // Only whole lines go, each with its newline, so a change in length is a removal.
      const auto edited{
          contents.value()
        | std::views::split('\n')
        | std::views::filter(isKept)
        | std::views::join_with('\n')
        | std::ranges::to<std::string>()
      };

      if(edited.size() == contents->size())
        return false;

      write_to_file(file, edited, std::ios_base::out);
      return true;
    }

    /** \brief The entries of a list in a CMakeLists.txt, and where in its text they lie. */
    struct cmake_list
    {
      std::string::size_type   entries_start{}, entries_end{};
      std::size_t              entry_indent{};
      std::vector<std::string> entries{};
    };

    /** \brief The first list in `text` which `patternOpen` opens and `patternClose` closes, if there is one.

        The entries begin at the end of the list's first line, and each is on a line of its own.
     */
    [[nodiscard]]
    std::optional<cmake_list> find_cmake_list(std::string_view text, std::string_view patternOpen, std::string_view patternClose)
    {
      constexpr auto npos{std::string::npos};

      const auto startPos{text.find(patternOpen)};
      if(startPos == npos)
        return std::nullopt;

      const auto endPos{text.find(patternClose, startPos + patternOpen.size())};
      if(endPos == npos)
        return std::nullopt;

      std::vector<std::string> entries{};
      auto newlinePos{npos}, next{startPos + patternOpen.size()};
      while((newlinePos = text.find('\n', next)) < endPos)
      {
        next = std::ranges::min(text.find('\n', newlinePos + 1), endPos);
        const auto entryStart{text.find_first_not_of(' ', newlinePos + 1)};

        entries.emplace_back(text.substr(entryStart, next - entryStart));
      }

      const auto entryIndent{
        [patternOpen]() {
          if(const auto pos{patternOpen.find('(')}; pos < std::string_view::npos)
            return pos + 1;

          return patternOpen.size();
        }()
      };

      return cmake_list{.entries_start{std::ranges::min(text.find('\n', startPos + patternOpen.size()), endPos)},
                        .entries_end{endPos},
                        .entry_indent{entryIndent},
                        .entries{std::move(entries)}};
    }

    void write_cmake_list(std::string& text, const cmake_list& list)
    {
      auto entryLine{
        [&list](const std::string& entry) { return std::format("\n{}{}", std::string(list.entry_indent, ' '), entry); }
      };

      const auto entryLines{
          list.entries
        | std::views::transform(entryLine)
        | std::views::join
        | std::ranges::to<std::string>()
      };

      text.replace(list.entries_start, list.entries_end - list.entries_start, entryLines);
    }

    [[nodiscard]]
    std::string cmake_entry(std::string_view cmakeEntryPrefix, const fs::path& file, const fs::path& hostDir)
    {
      return std::format("{}{}", cmakeEntryPrefix, file.lexically_relative(hostDir).generic_string());
    }
  }

  void add_include(const fs::path& file, std::string_view includePath)
  {
    auto inserter{
      [&includePath](std::string& text) {

        std::string_view include{"#include"};
        std::vector<std::string> entries{std::format("{}\n", include_directive(includePath))};

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

  bool remove_include(const fs::path& file, std::string_view includePath)
  {
    return remove_lines(file, [directive{include_directive(includePath)}](std::string_view line) { return line == directive; });
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

    std::string_view contentsView      {contentsStr};
    const auto       executionIndentEnd{contentsView.find_first_not_of(blanks, executionLineStart)};
    std::string_view executionIndent   {contentsView.substr(executionLineStart, executionIndentEnd - executionLineStart)};

    auto isRegistered{
      [contentsView](std::string_view test) {
        auto startsWithRegistration{
          [registration{registration_of(test)}](auto&& line) {
            return without_leading_blanks({std::ranges::begin(line), std::ranges::end(line)}).starts_with(registration);
          }
        };

        return std::ranges::any_of(contentsView | std::views::split('\n'), startsWithRegistration);
      }
    };

    auto registrationLineOf{
      [executionIndent](std::string_view test) {
        return std::format("{}{}\n", executionIndent, registration_of(test));
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

  bool remove_test_registrations(const fs::path& file, const std::vector<std::string>& tests)
  {
    auto isRegistration{
      [&tests](std::string_view line) {
        auto registers{
          [content{without_leading_blanks(line)}](const std::string& test) { return content.starts_with(registration_of(test)); }
        };

        return std::ranges::any_of(tests, registers);
      }
    };

    return remove_lines(file, isRegistration);
  }

  void add_to_cmake(const fs::path& cmakeLists,
                    const fs::path& hostDir,
                    const fs::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrefix)
  {
    auto addEntry{
      [&](std::string& text) {
        auto list{find_cmake_list(text, patternOpen, patternClose)};
        if(!list)
          throw std::runtime_error{
            std::format("Unable to find appropriate place to add source file to {}", cmakeLists.generic_string())
          };

        list->entries.push_back(cmake_entry(cmakeEntryPrefix, file, hostDir));
        std::ranges::sort(list->entries);
        write_cmake_list(text, *list);
      }
    };

    read_modify_write(cmakeLists, addEntry);
  }

  bool remove_from_cmake(const fs::path& cmakeLists,
                         const fs::path& hostDir,
                         const fs::path& file,
                         std::string_view patternOpen,
                         std::string_view patternClose,
                         std::string_view cmakeEntryPrefix)
  {
    auto contents{read_to_string(cmakeLists, std::ios_base::in)};
    if(!contents)
      throw std::runtime_error{report_failed_read(cmakeLists)};

    auto list{find_cmake_list(*contents, patternOpen, patternClose)};
    if(!list || (std::erase(list->entries, cmake_entry(cmakeEntryPrefix, file, hostDir)) == 0))
      return false;

    write_cmake_list(*contents, *list);
    write_to_file(cmakeLists, *contents, std::ios_base::out);
    return true;
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
