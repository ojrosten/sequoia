////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/FailureInfo.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;
  
  namespace
  {
    [[nodiscard]]
    std::string analyse_output(const fs::path& filename, const std::vector<failure_output>& failuresFromFiles)
    {
      if(failuresFromFiles.size() <= 1) return "";

      using namespace std::string_literals;

      auto to_percent{
        [&failuresFromFiles](auto num){
          return std::format("{:3.1f}", 100 * static_cast<double>(num) / failuresFromFiles.size());
        }
      };

      auto first{failuresFromFiles.begin()},
           last{failuresFromFiles.end()},
           current{first};

      const auto initial{first};

      std::string freqs{"["};
      std::string messages{};
      while(++first != last)
      {
        if(*first != *current)
        {
          freqs += to_percent(std::ranges::distance(current, first)) += "%,";
          auto[i,j]{std::ranges::mismatch(*current, *first)};
          if(j == first->end())
          {
            throw std::logic_error{"Unable to identify instability"};
          }
          else if(i == current->end())
          {
            if(current->begin() == current->end())
            {
              messages.append("--No failures--\n\nvs.\n\n").append(j->message);
            }
            else
            {
              const auto commonMessage{
                [current](){
                  std::string mess{};
                  for(auto c{current->begin()}; c != current->end(); ++c)
                  {
                    mess.append(c->message).append("\n");
                  }

                  return mess;
                }()
              };

              messages.append(messages.empty() ? commonMessage : "\n");

              messages.append(std::format("vs.\n\n{}{}", commonMessage, j->message));
            }
          }
          else
          {
            if(current == initial)
              messages.append(i->message);

            messages.append("\nvs.\n\n").append(j->message);
          }

          current = first;
        }
      }

      if(current != initial)
      {
        freqs += to_percent(std::ranges::distance(current, last)) += "%]\n\n"s;

        return std::format("\nInstability detected in file \"{}\"\nOutcome frequencies:\n{}{}\n{}",
                           filename.string(),
                           freqs,
                           messages,
                           instability_footer());
      }

      return "";
    }

    [[nodiscard]]
    fs::path source_from_instability_analysis(const fs::path& dir)
    {
      auto parent{dir.parent_path()};
      if(!parent.empty())
      {
        std::string str{(--parent.end())->string()};
        if(auto pos{str.find_last_of('_')}; pos != std::string::npos)
        {
          str[pos] = '.';
        }

        return str;
      }

      return "";
    }
  }

  [[nodiscard]]
  std::string to_string(const failure_info& info)
  {
    return std::format("check: {}\nlength: {}\n{}\n", info.check_index, info.message.size(), info.message);
  }

  std::ostream& operator<<(std::ostream& s, const failure_info& info)
  {
    return s << to_string(info);
  }

  std::istream& operator>>(std::istream& s, failure_info& info)
  {
    if(!peek_for_more(s))
      return s;

    auto toCheckIndex{[](const std::string& text) { return parse_integer<std::size_t>(text, "a check index"); }};
    auto toLength    {[](const std::string& text) { return parse_integer<std::size_t>(text, "a length"); }};

    info = failure_info{
      .check_index{extract_field(s, "check: ", toCheckIndex)},
      .message{extract_text(s, extract_field(s, "length: ", toLength))}
    };

    return s;
  }

  [[nodiscard]]
  std::string to_string(const failure_output& output)
  {
    auto infoText{[](const failure_info& info) { return to_string(info); }};

    return
        output
      | std::views::transform(infoText)
      | std::views::join
      | std::ranges::to<std::string>();
  }

  std::ostream& operator<<(std::ostream& s, const failure_output& output)
  {
    return s << to_string(output);
  }

  std::istream& operator>>(std::istream& s, failure_output& output)
  {
    using iter_t = std::istream_iterator<failure_info>;
    output = std::ranges::subrange{iter_t{s}, iter_t{}} | std::ranges::to<failure_output>();
    return s;
  }

  [[nodiscard]]
  std::string instability_analysis(const fs::path& root, const std::size_t trials)
  {
    if(trials <= 1) return "";

    const auto files{
      [&root](){
        auto isOutputFile{
          [](const fs::directory_entry& entry) {
            return entry.is_regular_file() && (entry.path().extension() == ".txt");
          }
        };

        auto pathOf{[](const fs::directory_entry& entry) { return entry.path(); }};

        auto outputFiles{
            fs::recursive_directory_iterator{root}
          | std::views::filter(isOutputFile)
          | std::views::transform(pathOf)
          | std::ranges::to<std::vector>()
        };

        std::ranges::sort(outputFiles);
        return outputFiles;
      }()
    };

    if(files.size() % trials)
      throw std::runtime_error{"Instability analysis: incorrect number of output files"};

    auto readAndIndentFailureOutput{
      [](const fs::path& file) {
        auto indented{
          [](const failure_info& info) {
            return failure_info{info.check_index, indent(info.message, tab)};
          }
        };

        if(std::ifstream ifile{file, std::ios_base::binary})
        {
          try
          {
            failure_output output{};
            ifile >> output;
            return output | std::views::transform(indented) | std::ranges::to<failure_output>();
          }
          catch(const std::exception& e)
          {
            throw std::runtime_error{
              std::format("Unable to read the failures in {}: {}", file.generic_string(), e.what())
            };
          }
        }
        else
        {
          throw std::runtime_error{report_failed_read(file)};
        }
      }
    };

    auto analyseTestFrom{
      [&files, trials, &readAndIndentFailureOutput](std::size_t first) {
        auto testFiles{std::span{files}.subspan(first, trials)};
        auto failuresFromFiles{
          testFiles | std::views::transform(readAndIndentFailureOutput) | std::ranges::to<std::vector>()
        };

        std::ranges::sort(failuresFromFiles);
        return analyse_output(source_from_instability_analysis(testFiles.front().parent_path()), failuresFromFiles);
      }
    };

    // TO DO: files | std::views::chunk(trials), in place of the stride and the subspan, once libc++ has chunk (P2442)
    const auto message{
        std::views::iota(std::size_t{}, files.size())
      | std::views::stride(trials)
      | std::views::transform(analyseTestFrom)
      | std::views::join
      | std::ranges::to<std::string>()
    };

    return !message.empty() ? message : "\nNo instabilities detected\n";
  }
}
