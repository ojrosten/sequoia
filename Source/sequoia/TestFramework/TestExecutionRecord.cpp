////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file
    \brief Definitions for TestExecutionRecord.hpp
 */

#include "sequoia/TestFramework/TestExecutionRecord.hpp"

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
    constexpr std::string_view execution_duration_label {"execution duration"},
                               runner_overhead_label    {"runner overhead"},
                               attempts_label           {"performance check attempts"},
                               entries_of_attempts_label{"entries of performance check attempts"};

    enum class finished_state : bool { no, yes };

    [[nodiscard]]
    std::string start_entry(test_execution_record::start_type start)
    {
      return std::format("started {:%FT%TZ}", start);
    }

    [[nodiscard]]
    std::string duration_entry(std::string_view label, std::chrono::microseconds duration)
    {
      return std::format("{} {}us", label, duration.count());
    }

    [[nodiscard]]
    std::string finished_entry(finished_state finished)
    {
      return std::format("finished {}", finished == finished_state::yes ? "yes" : "no");
    }

    [[nodiscard]]
    std::string count_entry(std::string_view label, std::size_t count)
    {
      return std::format("{} {}", label, count);
    }

    // Each recognizer builds a value from the numbers in an entry. It returns
    // the value only if the entry's writer reprints the entry exactly from
    // that value.
    constexpr auto start_in{
      [](std::string_view entry) -> std::optional<test_execution_record::start_type> {
        // TO DO: std::chrono::parse reads the start, once libc++ provides it
        using namespace std::chrono;
        constexpr std::size_t fieldsOfStart{7};
        if(const auto numbers{extract_numbers_from<unsigned, fieldsOfStart>(entry)})
        {
          const auto [y, mo, d, h, mi, s, ms]{*numbers};
          const test_execution_record::start_type midnight{sys_days{year{static_cast<int>(y)} / month{mo} / day{d}}};
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

    [[nodiscard]]
    constexpr auto count_recognizer(std::string_view label)
    {
      return [label](std::string_view entry) -> std::optional<std::size_t> {
        if(const auto numbers{extract_numbers_from<std::size_t, 1>(entry)})
        {
          const auto [count]{*numbers};
          if(count_entry(label, count) == entry)
            return count;
        }

        return std::nullopt;
      };
    }

    constexpr auto attempts_in           {count_recognizer(attempts_label)},
                   entries_of_attempts_in{count_recognizer(entries_of_attempts_label)};

    constexpr auto finished_in{
      [](std::string_view entry) -> std::optional<finished_state> {
        for(const auto finished : {finished_state::no, finished_state::yes})
        {
          if(finished_entry(finished) == entry)
            return finished;
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

    /** Reads the number of entries of attempts, then that many entries. */
    [[nodiscard]]
    std::vector<std::size_t> read_attempts(std::istream& s)
    {
      const auto entries{read_entry(s, "a number of entries of attempts", entries_of_attempts_in)};

      std::vector<std::size_t> attempts{};
      for([[maybe_unused]] const auto entry : std::views::iota(std::size_t{}, entries))
      {
        attempts.push_back(read_entry(s, "a number of attempts", attempts_in));
      }

      return attempts;
    }

    [[nodiscard]]
    std::optional<test_execution_completion> read_completion(std::istream& s, finished_state finished)
    {
      if(finished == finished_state::no)
        return std::nullopt;

      if(test_execution_completion completion{}; s >> completion)
        return completion;

      throw std::runtime_error{"Expected an execution duration but there are no more lines"};
    }

    /** Reads the `T` which `file` holds. `description` names a `T` in the
        messages of what this throws.

        \throws std::runtime_error if `file` cannot be opened, holds no `T`,
                holds anything after its `T`, or reading a `T` from it throws
     */
    template<class T>
    [[nodiscard]]
    T read_from(const fs::path& file, std::string_view description)
    {
      if(std::ifstream ifile{file})
      {
        try
        {
          if(T value{}; ifile >> value)
          {
            if(std::string line{}; more_to_read(ifile) && std::getline(ifile, line))
              throw std::runtime_error{std::format("Expected the end of the file but found '{}'", line)};

            return value;
          }
        }
        catch(const std::runtime_error& e)
        {
          throw std::runtime_error{
            std::format("Unable to read the {} in {}: {}", description, file.generic_string(), e.what())
          };
        }

        throw std::runtime_error{std::format("No {} in {}", description, file.generic_string())};
      }

      throw std::runtime_error{report_failed_read(file)};
    }
  }

  [[nodiscard]]
  std::string to_string(const test_execution_completion& completion)
  {
    auto attemptsLine{[](std::size_t attempts) { return std::format("{}\n", count_entry(attempts_label, attempts)); }};

    const auto attemptsLines{
        completion.performance_check_attempts
      | std::views::transform(attemptsLine)
      | std::views::join
      | std::ranges::to<std::string>()
    };

    return std::format("{}\n{}\n{}\n{}",
                       duration_entry(execution_duration_label, completion.execution_duration),
                       duration_entry(runner_overhead_label,    completion.runner_overhead),
                       count_entry(entries_of_attempts_label,   completion.performance_check_attempts.size()),
                       attemptsLines);
  }

  std::ostream& operator<<(std::ostream& s, const test_execution_completion& completion)
  {
    return s << to_string(completion);
  }

  std::istream& operator>>(std::istream& s, test_execution_completion& completion)
  {
    if(!peek_for_more(s))
      return s;

    completion = test_execution_completion{
      .execution_duration{read_entry(s, "an execution duration", execution_duration_in)},
      .runner_overhead{read_entry(s, "a runner overhead", runner_overhead_in)},
      .performance_check_attempts{read_attempts(s)}
    };

    return s;
  }

  [[nodiscard]]
  std::string to_string(const test_execution_record& record)
  {
    const auto finished{record.completion ? finished_state::yes : finished_state::no};
    const auto completionLines{record.completion ? to_string(record.completion.value()) : std::string{}};
    return std::format("{}\n{}\n{}", start_entry(record.start), finished_entry(finished), completionLines);
  }

  std::ostream& operator<<(std::ostream& s, const test_execution_record& record)
  {
    return s << to_string(record);
  }

  std::istream& operator>>(std::istream& s, test_execution_record& record)
  {
    if(!peek_for_more(s))
      return s;

    const auto start{read_entry(s, "a start time", start_in)};
    const auto finished{read_entry(s, "whether the test finished", finished_in)};
    record = test_execution_record{.start{start}, .completion{read_completion(s, finished)}};
    return s;
  }

  [[nodiscard]]
  test_execution_record read_test_execution_record(const fs::path& file)
  {
    return read_from<test_execution_record>(file, "test execution record");
  }

  [[nodiscard]]
  std::string to_string(const run_stamp& stamp)
  {
    return std::format("{}\n", start_entry(stamp.start));
  }

  std::ostream& operator<<(std::ostream& s, const run_stamp& stamp)
  {
    return s << to_string(stamp);
  }

  std::istream& operator>>(std::istream& s, run_stamp& stamp)
  {
    if(!peek_for_more(s))
      return s;

    stamp = run_stamp{.start{read_entry(s, "a start time", start_in)}};
    return s;
  }

  [[nodiscard]]
  run_stamp read_run_stamp(const fs::path& file)
  {
    return read_from<run_stamp>(file, "run stamp");
  }
}
