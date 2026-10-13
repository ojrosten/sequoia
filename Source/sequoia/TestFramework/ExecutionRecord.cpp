////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file
    \brief Definitions for ExecutionRecord.hpp
 */

#include "sequoia/TestFramework/ExecutionRecord.hpp"

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Numbers.hpp"

#include <format>
#include <fstream>
#include <istream>
#include <ostream>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr std::string_view start_key             {"started "},
                               execution_duration_key{"execution duration "},
                               runner_overhead_key   {"runner overhead "},
                               attempts_key          {"performance check attempts "};

    constexpr std::string_view microseconds_unit{"us"};

    [[nodiscard]]
    std::string entry(std::string_view key, std::string_view value)
    {
      return std::format("{}{}\n", key, value);
    }

    [[nodiscard]]
    std::string text_of(execution_record::start_type start)
    {
      return std::format("{:%FT%TZ}", start);
    }

    [[nodiscard]]
    std::string text_of(std::chrono::microseconds duration)
    {
      return std::format("{}{}", duration.count(), microseconds_unit);
    }

    [[nodiscard]]
    execution_record::start_type to_start(const std::string& text)
    {
      // TO DO: std::chrono::parse reads the start, once libc++ provides it
      using namespace std::chrono;
      constexpr std::size_t fieldsOfStart{7};
      if(const auto fields{extract_numbers_from<unsigned, fieldsOfStart>(text)})
      {
        const auto [y, mo, d, h, mi, s, ms]{*fields};
        const year_month_day date{year{static_cast<int>(y)}, month{mo}, day{d}};

        // The ranges are checked before the arithmetic: a large field would
        // overflow it where a duration's representation is an int
        if(date.ok() && (h < 24) && (mi < 60) && (s < 60) && (ms < 1000))
        {
          const auto start{sys_days{date} + hours{h} + minutes{mi} + seconds{s} + milliseconds{ms}};
          if(text_of(start) == text)
            return start;
        }
      }

      throw std::runtime_error{std::format("'{}' is not a start", text)};
    }

    [[nodiscard]]
    std::chrono::microseconds to_duration(const std::string& text)
    {
      if(text.ends_with(microseconds_unit))
      {
        const std::string_view count{std::string_view{text}.substr(0, text.size() - microseconds_unit.size())};
        const std::chrono::microseconds duration{
          parse_integer<std::chrono::microseconds::rep>(count, "a number of microseconds")
        };

        if(text_of(duration) == text)
        {
          if(duration < std::chrono::microseconds::zero())
            throw std::runtime_error{std::format("'{}' is a negative duration", text)};

          return duration;
        }
      }

      throw std::runtime_error{std::format("'{}' is not a duration in microseconds", text)};
    }

    [[nodiscard]]
    std::size_t to_attempts(const std::string& text)
    {
      if(const auto attempts{parse_integer<std::size_t>(text, "a number of attempts")}; std::to_string(attempts) == text)
        return attempts;

      throw std::runtime_error{std::format("'{}' is not a number of attempts", text)};
    }

    [[nodiscard]]
    std::string text_of(const execution_completion& completion)
    {
      auto attemptsEntry{[](std::size_t attempts) { return entry(attempts_key, std::to_string(attempts)); }};

      const auto attemptsEntries{
          completion.performance_check_attempts
        | std::views::transform(attemptsEntry)
        | std::views::join
        | std::ranges::to<std::string>()
      };

      return std::format("{}{}{}",
                         entry(execution_duration_key, text_of(completion.execution_duration)),
                         entry(runner_overhead_key,    text_of(completion.runner_overhead)),
                         attemptsEntries);
    }

    [[nodiscard]]
    bool more_to_read(std::istream& s)
    {
      return s.good() && (s.peek() != std::char_traits<char>::eof());
    }

    /** Reads entries of attempts until the end of `s`. */
    [[nodiscard]]
    std::vector<std::size_t> read_attempts(std::istream& s)
    {
      std::vector<std::size_t> attempts{};
      while(more_to_read(s))
      {
        attempts.push_back(extract_field(s, attempts_key, to_attempts));
      }

      return attempts;
    }

    [[nodiscard]]
    std::optional<execution_completion> read_completion(std::istream& s)
    {
      if(!more_to_read(s))
        return std::nullopt;

      return execution_completion{
        .execution_duration{extract_field(s, execution_duration_key, to_duration)},
        .runner_overhead{extract_field(s, runner_overhead_key, to_duration)},
        .performance_check_attempts{read_attempts(s)}
      };
    }
  }

  [[nodiscard]]
  std::string to_string(const execution_record& record)
  {
    const auto completionText{record.completion ? text_of(record.completion.value()) : std::string{}};
    return entry(start_key, text_of(record.start)) + completionText;
  }

  std::ostream& operator<<(std::ostream& s, const execution_record& record)
  {
    return s << to_string(record);
  }

  std::istream& operator>>(std::istream& s, execution_record& record)
  {
    if(!peek_for_more(s))
      return s;

    record = execution_record{.start{extract_field(s, start_key, to_start)}, .completion{read_completion(s)}};
    return s;
  }

  [[nodiscard]]
  execution_record read_execution_record(const fs::path& file)
  {
    if(std::ifstream ifile{file})
    {
      try
      {
        if(execution_record record{}; ifile >> record)
          return record;
      }
      catch(const std::runtime_error& e)
      {
        throw std::runtime_error{
          std::format("Unable to read the execution record in {}: {}", file.generic_string(), e.what())
        };
      }

      throw std::runtime_error{std::format("No execution record in {}", file.generic_string())};
    }

    throw std::runtime_error{report_failed_read(file)};
  }
}
