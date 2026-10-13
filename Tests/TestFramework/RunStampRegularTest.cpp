////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "RunStampRegularTest.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    run_stamp stamp()
    {
      using namespace std::chrono;
      return {.start{sys_days{year{2026} / 10 / 13} + 9h + 15min + 4s + 123ms}};
    }
  }

  [[nodiscard]]
  std::filesystem::path run_stamp_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void run_stamp_regular_test::run_tests()
  {
    test_semantics();
    test_round_trip();
    test_end_of_stream();
    test_written_format();
    test_refusals();
    test_refusals_of_files();
  }

  void run_stamp_regular_test::test_semantics()
  {
    check_semantics("A default stamp and a later one", run_stamp{}, stamp());
  }

  void run_stamp_regular_test::test_round_trip()
  {
    auto readBackFromWritten{
      [](const run_stamp& written) -> std::optional<run_stamp> {
        std::stringstream s{};
        s << written;

        run_stamp readBack{stamp()};
        if(s >> readBack)
          return readBack;

        return std::nullopt;
      }
    };

    check(equality,
          "operator>> overwrites a later stamp with a default stamp which operator<< wrote",
          readBackFromWritten(run_stamp{}),
          std::optional{run_stamp{}});

    check(equality,
          "A stamp round-trips through operator<< and operator>>",
          readBackFromWritten(stamp()),
          std::optional{stamp()});

    using namespace std::chrono;
    const run_stamp
      firstStart{.start{sys_days{year{0} / 1 / 1}}},
      lastStart {.start{sys_days{year{32767} / 12 / 31} + days{1} - milliseconds{1}}};

    check(equality,
          "A stamp with the earliest start time the format holds round-trips through operator<< and operator>>",
          readBackFromWritten(firstStart),
          std::optional{firstStart});

    check(equality,
          "A stamp with the latest start time the format holds round-trips through operator<< and operator>>",
          readBackFromWritten(lastStart),
          std::optional{lastStart});
  }

  void run_stamp_regular_test::test_end_of_stream()
  {
    std::stringstream s{};
    run_stamp readInto{stamp()};
    s >> readInto;

    check("A read at the end of a stream fails", s.fail());
    check(equality, "A read at the end of a stream leaves the stamp as it was", readInto, stamp());

    auto readWithoutFinalLineBreak{
      [](const run_stamp& written) -> std::optional<run_stamp> {
        const auto text{to_string(written)};
        std::stringstream unterminated{text.substr(0, text.size() - 1)};

        run_stamp readBack{};
        if(unterminated >> readBack)
          return readBack;

        return std::nullopt;
      }
    };

    check(equality,
          "A stamp with no final line break reads as one with the line break",
          readWithoutFinalLineBreak(stamp()),
          std::optional{stamp()});

    std::stringstream followed{to_string(stamp()) + "next line\n"};
    run_stamp readBack{};
    const bool readSucceeded{static_cast<bool>(followed >> readBack)};
    std::string rest{};
    std::getline(followed, rest);

    check("A read of a stamp followed by another line succeeds", readSucceeded);
    check(equality, "A stamp followed by another line reads as the stamp", readBack, stamp());
    check(equality, "operator>> leaves the line after a stamp unread", rest, std::string{"next line"});
  }

  void run_stamp_regular_test::test_written_format()
  {
    write_to_file(working_materials() / "Stamp.txt", to_string(stamp()), std::ios_base::binary);

    check(equivalence, "The text of a stamp", working_materials() / "Stamp.txt", predictive_materials() / "Stamp.txt");
  }

  void run_stamp_regular_test::test_refusals()
  {
    auto readStamp{
      [](std::string text) {
        return [text{std::move(text)}]() {
          std::stringstream s{text};
          run_stamp readInto{};
          s >> readInto;
        };
      }
    };

    check_exception_thrown<std::runtime_error>(
      "A first line which does not give the start time",
      readStamp("begun 2026-10-13T09:15:04.123Z\n"));

    run_stamp readInto{stamp()};
    try
    {
      std::stringstream s{"begun 2026-10-14T09:15:04.123Z\n"};
      s >> readInto;
    }
    catch(const std::runtime_error&)
    {
    }

    check(equality, "A refused read leaves the stamp as it was", readInto, stamp());
  }

  void run_stamp_regular_test::test_refusals_of_files()
  {
    check_exception_thrown<std::runtime_error>(
      "A file which does not exist",
      [this]() { return read_run_stamp(working_materials() / "Missing.txt"); });

    check_exception_thrown<std::runtime_error>(
      "An empty file",
      [this]() { return read_run_stamp(working_materials() / "Empty.txt"); });

    check_exception_thrown<std::runtime_error>(
      "A file with a line after the stamp",
      [this]() { return read_run_stamp(working_materials() / "Followed.txt"); });
  }
}
