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

#include <concepts>
#include <format>
#include <fstream>
#include <istream>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr std::string_view execution_duration_label{"execution duration"},
                               runner_overhead_label   {"runner overhead"};

    [[nodiscard]]
    std::string start_entry(execution_record::start_type start)
    {
      return std::format("started {:%FT%TZ}", start);
    }

    [[nodiscard]]
    std::string duration_entry(std::string_view label, std::chrono::microseconds duration)
    {
      return std::format("{} {}us", label, duration.count());
    }

    [[nodiscard]]
    std::string attempts_entry(std::size_t attempts)
    {
      return std::format("performance check attempts {}", attempts);
    }

    [[nodiscard]]
    std::string lines_of(const execution_completion& completion)
    {
      auto attemptsLine{[](std::size_t attempts) { return std::format("{}\n", attempts_entry(attempts)); }};

      const auto attemptsLines{
          completion.performance_check_attempts
        | std::views::transform(attemptsLine)
        | std::views::join
        | std::ranges::to<std::string>()
      };

      return std::format("{}\n{}\n{}",
                         duration_entry(execution_duration_label, completion.execution_duration),
                         duration_entry(runner_overhead_label,    completion.runner_overhead),
                         attemptsLines);
    }

    // Each recognizer builds a value from the numbers in an entry. It returns
    // the value only if the entry's writer reprints the entry exactly from
    // that value.
    constexpr auto start_in{
      [](std::string_view entry) -> std::optional<execution_record::start_type> {
        // TO DO: std::chrono::parse reads the start, once libc++ provides it
        using namespace std::chrono;
        constexpr std::size_t fieldsOfStart{7};
        if(const auto numbers{extract_numbers_from<unsigned, fieldsOfStart>(entry)})
        {
          const auto [y, mo, d, h, mi, s, ms]{*numbers};
          const execution_record::start_type midnight{sys_days{year{static_cast<int>(y)} / month{mo} / day{d}}};
          const auto start{midnight + hours{h} + minutes{mi} + seconds{s} + milliseconds{ms}};

          if(start_entry(start) == entry)
            return start;
        }

        return std::nullopt;
      }
    };

    [[nodiscard]]
    constexpr auto duration_recognizer(std::string_view label)
    {
      return [label](std::string_view entry) -> std::optional<std::chrono::microseconds> {
        using rep = std::chrono::microseconds::rep;
        if(const auto numbers{extract_numbers_from<std::make_unsigned_t<rep>, 1>(entry)})
        {
          const auto [count]{*numbers};
          if(std::in_range<rep>(count))
          {
            const std::chrono::microseconds duration{static_cast<rep>(count)};
            if(duration_entry(label, duration) == entry)
              return duration;
          }
        }

        return std::nullopt;
      };
    }

    constexpr auto execution_duration_in{duration_recognizer(execution_duration_label)},
                   runner_overhead_in   {duration_recognizer(runner_overhead_label)};

    constexpr auto attempts_in{
      [](std::string_view entry) -> std::optional<std::size_t> {
        if(const auto numbers{extract_numbers_from<std::size_t, 1>(entry)})
        {
          const auto [attempts]{*numbers};
          if(attempts_entry(attempts) == entry)
            return attempts;
        }

        return std::nullopt;
      }
    };

    /** Reads the next line of `s`, and returns the value which `recognize`
        finds in that line.

        \throws std::runtime_error if `s` has no next line, or `recognize`
                finds no value in that line
     */
    template<std::invocable<std::string_view> Recognizer>
    [[nodiscard]]
    auto read_entry(std::istream& s, std::string_view description, Recognizer recognize)
    {
      std::string line{};
      if(!std::getline(s, line))
        throw std::runtime_error{std::format("Expected {} but there are no more lines", description)};

      if(const auto value{recognize(line)})
        return value.value();

      throw std::runtime_error{std::format("Expected {} but found '{}'", description, line)};
    }

    [[nodiscard]]
    bool more_to_read(std::istream& s)
    {
      return s.good() && (s.peek() != std::char_traits<char>::eof());
    }

    /** Reads the numbers of attempts in the rest of `s`. */
    [[nodiscard]]
    std::vector<std::size_t> read_attempts(std::istream& s)
    {
      std::vector<std::size_t> attempts{};
      while(more_to_read(s))
      {
        attempts.push_back(read_entry(s, "a number of attempts", attempts_in));
      }

      return attempts;
    }

    [[nodiscard]]
    std::optional<execution_completion> read_completion(std::istream& s)
    {
      if(!more_to_read(s))
        return std::nullopt;

      return execution_completion{
        .execution_duration{read_entry(s, "an execution duration", execution_duration_in)},
        .runner_overhead{read_entry(s, "a runner overhead", runner_overhead_in)},
        .performance_check_attempts{read_attempts(s)}
      };
    }
  }

  [[nodiscard]]
  std::string to_string(const execution_record& record)
  {
    const auto completionLines{record.completion ? lines_of(record.completion.value()) : std::string{}};
    return std::format("{}\n{}", start_entry(record.start), completionLines);
  }

  std::ostream& operator<<(std::ostream& s, const execution_record& record)
  {
    return s << to_string(record);
  }

  std::istream& operator>>(std::istream& s, execution_record& record)
  {
    if(!peek_for_more(s))
      return s;

    record = execution_record{.start{read_entry(s, "a start", start_in)}, .completion{read_completion(s)}};
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
