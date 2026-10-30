////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module sequoia.test_framework;

import std;

import sequoia.streaming;
import sequoia.text_processing;

/** \file
    \brief Definitions for TestExecutionRecord.cppm
 */

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr std::string_view label_for_execution_duration {"execution duration"},
                               label_for_runner_overhead    {"runner overhead"},
                               label_for_attempts           {"performance check attempts"},
                               label_for_entries_of_attempts{"entries of performance check attempts"};

    enum class test_execution_status : bool { started, finished };

    [[nodiscard]]
    std::string entry_for_start_time(test_execution_record::start_time_type startTime)
    {
      return std::format("started {:%FT%TZ}", startTime);
    }

    [[nodiscard]]
    std::string entry_for_duration(std::string_view label, std::chrono::microseconds duration)
    {
      return std::format("{} {}us", label, duration.count());
    }

    [[nodiscard]]
    std::string entry_for_status(test_execution_status status)
    {
      return std::format("finished {}", status == test_execution_status::finished ? "yes" : "no");
    }

    [[nodiscard]]
    std::string entry_for_count(std::string_view label, std::size_t count)
    {
      return std::format("{} {}", label, count);
    }

    // Each extractor finds the value an entry holds, and returns it only if
    // the entry's writer reprints the entry exactly from that value.
    constexpr auto extractor_for_start_time{
      [](std::string_view entry) -> std::optional<test_execution_record::start_time_type> {
        // TO DO: std::chrono::parse reads the start time, once libc++ provides it
        using namespace std::chrono;
        constexpr std::size_t fieldsOfStart{7};
        if(const auto numbers{extract_numbers_from<unsigned, fieldsOfStart>(entry)})
        {
          const auto [y, mo, d, h, mi, s, ms]{*numbers};
          const test_execution_record::start_time_type midnight{
            sys_days{year{static_cast<int>(y)} / month{mo} / day{d}}
          };
          const auto startTime{midnight + hours{h} + minutes{mi} + seconds{s} + milliseconds{ms}};

          if(entry_for_start_time(startTime) == entry)
            return startTime;
        }

        return std::nullopt;
      }
    };

    [[nodiscard]]
    constexpr auto extractor_for_duration(std::string_view label)
    {
      return [label](std::string_view entry) -> std::optional<std::chrono::microseconds> {
        using rep = std::chrono::microseconds::rep;
        if(const auto numbers{extract_numbers_from<std::make_unsigned_t<rep>, 1>(entry)})
        {
          const auto [count]{*numbers};
          if(std::in_range<rep>(count))
          {
            const std::chrono::microseconds duration{static_cast<rep>(count)};
            if(entry_for_duration(label, duration) == entry)
              return duration;
          }
        }

        return std::nullopt;
      };
    }

    constexpr auto extractor_for_execution_duration{extractor_for_duration(label_for_execution_duration)},
                   extractor_for_runner_overhead   {extractor_for_duration(label_for_runner_overhead)};

    template<std::integral Count>
    [[nodiscard]]
    constexpr auto extractor_for_count(std::string_view label)
    {
      return [label](std::string_view entry) -> std::optional<Count> {
        if(const auto numbers{extract_numbers_from<std::size_t, 1>(entry)})
        {
          const auto [count]{*numbers};
          if(std::in_range<Count>(count) && (entry_for_count(label, count) == entry))
            return static_cast<Count>(count);
        }

        return std::nullopt;
      };
    }

    constexpr auto extractor_for_attempts{extractor_for_count<std::size_t>(label_for_attempts)};

    // The number of entries is a std::ptrdiff_t since std::ranges::generate_n
    // takes its count as an iterator's difference type
    constexpr auto extractor_for_entries_of_attempts{
      extractor_for_count<std::ptrdiff_t>(label_for_entries_of_attempts)
    };

    constexpr auto extractor_for_status{
      [](std::string_view entry) -> std::optional<test_execution_status> {
        for(const auto status : {test_execution_status::started, test_execution_status::finished})
        {
          if(entry_for_status(status) == entry)
            return status;
        }

        return std::nullopt;
      }
    };

    /** Reads the next line of `s`, and returns the value which `extract`
        finds in that line.

        \throws std::runtime_error if `s` has no next line, or `extract`
                finds no value in that line
     */
    template<std::invocable<std::string_view> Extractor>
    [[nodiscard]]
    auto read_entry(std::istream& s, std::string_view description, Extractor extract)
    {
      std::string line{};
      if(!std::getline(s, line))
        throw std::runtime_error{std::format("Expected {} but there are no more lines", description)};

      if(const auto value{extract(line)})
        return value.value();

      throw std::runtime_error{std::format("Expected {} but found '{}'", description, line)};
    }

    /** Reads the number of entries of attempts, then that many entries. */
    [[nodiscard]]
    std::vector<std::size_t> read_attempt_counts(std::istream& s)
    {
      const auto entries{read_entry(s, "a number of entries of attempts", extractor_for_entries_of_attempts)};
      auto nextNumberOfAttempts{[&s]() { return read_entry(s, "a number of attempts", extractor_for_attempts); }};

      // generate_n rather than a view, since each read consumes a line of `s`
      std::vector<std::size_t> attempts{};
      std::ranges::generate_n(std::back_inserter(attempts), entries, nextNumberOfAttempts);
      return attempts;
    }

    [[nodiscard]]
    test_execution_completion read_completion_entries(std::istream& s)
    {
      return {
        .execution_duration{read_entry(s, "an execution duration", extractor_for_execution_duration)},
        .runner_overhead{read_entry(s, "a runner overhead", extractor_for_runner_overhead)},
        .performance_check_attempts{read_attempt_counts(s)}
      };
    }

    [[nodiscard]]
    std::optional<test_execution_completion> read_optional_completion(std::istream& s, test_execution_status status)
    {
      if(status == test_execution_status::started)
        return std::nullopt;

      return read_completion_entries(s);
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
            if(std::string line{}; peek_for_more(ifile) && std::getline(ifile, line))
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
    auto attemptsLine{
      [](std::size_t attempts) { return std::format("{}\n", entry_for_count(label_for_attempts, attempts)); }
    };

    const auto attemptsLines{
        completion.performance_check_attempts
      | std::views::transform(attemptsLine)
      | std::views::join
      | std::ranges::to<std::string>()
    };

    return std::format("{}\n{}\n{}\n{}",
                       entry_for_duration(label_for_execution_duration, completion.execution_duration),
                       entry_for_duration(label_for_runner_overhead,    completion.runner_overhead),
                       entry_for_count(label_for_entries_of_attempts,   completion.performance_check_attempts.size()),
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

    completion = read_completion_entries(s);

    return s;
  }

  [[nodiscard]]
  std::string to_string(const test_execution_record& record)
  {
    const auto status{record.completion ? test_execution_status::finished : test_execution_status::started};
    const auto completionLines{record.completion ? to_string(record.completion.value()) : std::string{}};
    return std::format("{}\n{}\n{}",
                       entry_for_start_time(record.start_time),
                       entry_for_status(status),
                       completionLines);
  }

  std::ostream& operator<<(std::ostream& s, const test_execution_record& record)
  {
    return s << to_string(record);
  }

  std::istream& operator>>(std::istream& s, test_execution_record& record)
  {
    if(!peek_for_more(s))
      return s;

    const auto startTime{read_entry(s, "a start time", extractor_for_start_time)};
    const auto status{read_entry(s, "whether the test finished", extractor_for_status)};
    record = test_execution_record{.start_time{startTime}, .completion{read_optional_completion(s, status)}};
    return s;
  }

  [[nodiscard]]
  std::vector<test_execution_record> read_test_execution_records(const fs::path& file)
  {
    if(std::ifstream ifile{file})
    {
      try
      {
        return std::views::istream<test_execution_record>(ifile) | std::ranges::to<std::vector>();
      }
      catch(const std::runtime_error& e)
      {
        throw std::runtime_error{
          std::format("Unable to read the test execution records in {}: {}", file.generic_string(), e.what())
        };
      }
    }

    throw std::runtime_error{report_failed_read(file)};
  }

  [[nodiscard]]
  std::string to_string(const run_stamp& stamp)
  {
    return std::format("{}\n", entry_for_start_time(stamp.start_time));
  }

  std::ostream& operator<<(std::ostream& s, const run_stamp& stamp)
  {
    return s << to_string(stamp);
  }

  std::istream& operator>>(std::istream& s, run_stamp& stamp)
  {
    if(!peek_for_more(s))
      return s;

    stamp = run_stamp{.start_time{read_entry(s, "a start time", extractor_for_start_time)}};
    return s;
  }

  [[nodiscard]]
  run_stamp read_run_stamp(const fs::path& file)
  {
    return read_from<run_stamp>(file, "run stamp");
  }
}
