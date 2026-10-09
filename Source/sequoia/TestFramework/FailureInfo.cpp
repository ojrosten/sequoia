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
    /** \brief The failures of one run of a test, as recorded and with each
               message projected.
     */
    struct run_failures
    {
      failure_output recorded{}, projected{};
    };

    [[nodiscard]]
    failure_output with_projected_messages(const failure_output& output, const message_projection& projection)
    {
      auto projectedInfo{
        [&projection](const failure_info& info) { return failure_info{info.check_index, projection(info.message)}; }
      };

      return output | std::views::transform(projectedInfo) | std::ranges::to<failure_output>();
    }

    /** \brief Reports the outcomes of repeated runs of a test, if they differ
               once each message is projected.

        `failuresFromFiles` must be sorted stably by their projected failures,
        so that each outcome is shown as the earliest of its runs recorded it.
     */
    [[nodiscard]]
    std::string analyse_output(const fs::path& filename, const std::vector<run_failures>& failuresFromFiles)
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
        if(first->projected != current->projected)
        {
          freqs += to_percent(std::ranges::distance(current, first)) += "%,";

          const auto mismatchIndex{
            static_cast<std::size_t>(
              std::ranges::distance(current->projected.begin(),
                                    std::ranges::mismatch(current->projected, first->projected).in1)
            )
          };

          if(mismatchIndex == first->recorded.size())
          {
            throw std::logic_error{"Unable to identify instability"};
          }

          const auto& newOutcomeMessage{first->recorded[mismatchIndex].message};
          if(mismatchIndex == current->recorded.size())
          {
            if(current->recorded.empty())
            {
              messages.append("--No failures--\n\nvs.\n\n").append(indent(newOutcomeMessage, tab));
            }
            else
            {
              const auto commonMessage{
                [current](){
                  std::string mess{};
                  for(const auto& info : current->recorded)
                  {
                    mess.append(indent(info.message, tab)).append("\n");
                  }

                  return mess;
                }()
              };

              messages.append(messages.empty() ? commonMessage : "\n");

              messages.append(std::format("vs.\n\n{}{}", commonMessage, indent(newOutcomeMessage, tab)));
            }
          }
          else
          {
            if(current == initial)
              messages.append(indent(current->recorded[mismatchIndex].message, tab));

            messages.append("\nvs.\n\n").append(indent(newOutcomeMessage, tab));
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
  std::string instability_analysis(const fs::path& root, const std::size_t trials, const message_projection& projection)
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

    auto readFailureOutput{
      [&projection](const fs::path& file) -> run_failures {
        if(std::ifstream ifile{file, std::ios_base::binary})
        {
          try
          {
            failure_output output{};
            ifile >> output;
            auto projected{with_projected_messages(output, projection)};
            return {.recorded{std::move(output)}, .projected{std::move(projected)}};
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
      [&files, trials, &readFailureOutput](std::size_t first) {
        auto testFiles{std::span{files}.subspan(first, trials)};
        auto failuresFromFiles{
          testFiles | std::views::transform(readFailureOutput) | std::ranges::to<std::vector>()
        };

        std::ranges::stable_sort(failuresFromFiles, {}, &run_failures::projected);
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
