////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FailureInfoTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path failure_info_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void failure_info_test::check_exceptions()
  {
    auto readFailureInfo{
      [](std::string text) {
        return [text{std::move(text)}]() {
          std::stringstream s{text};
          failure_info info{};
          s >> info;
        };
      }
    };

    check_exception_thrown<std::runtime_error>("A record not beginning with its check index",
                                               readFailureInfo("foo"));
    check_exception_thrown<std::runtime_error>("A check index which is not a number",
                                               readFailureInfo("check: foo\n"));
    check_exception_thrown<std::runtime_error>("A record with no length",
                                               readFailureInfo("check: 1\n"));
    check_exception_thrown<std::runtime_error>("A length which is not a number",
                                               readFailureInfo("check: 1\nlength: x\n"));
    check_exception_thrown<std::runtime_error>("A message shorter than its length",
                                               readFailureInfo("check: 1\nlength: 5\nabc\n"));
    check_exception_thrown<std::runtime_error>("A message longer than its length",
                                               readFailureInfo("check: 1\nlength: 3\nabcd\n"));

    auto analyseMalformed{[this]() { return instability_analysis(working_materials() / "Malformed", 2); }};
    check_exception_thrown<std::runtime_error>("A malformed file of failures, named by the instability analysis",
                                               analyseMalformed);
  }

  void failure_info_test::check_failure_info()
  {
    using namespace std::string_literals;

    failure_info x{}, y{1, "foo"};
    check(equivalence, "", x, std::pair{0, ""s});
    check(equivalence, "", y, std::pair{1, "foo"s});

    check_semantics("", x, y, std::weak_ordering::less);

  }

  void failure_info_test::check_round_trip()
  {
    const failure_output written{
      {0, ""},
      {1, "foo"},
      {2, "foo\nbar"},
      {3, "\n  foo\n\nbar\n"},
      {4, "a\n$\nb"},
      {5, "check: 6\nlength: 0\n"}
    };

    std::stringstream s{};
    s << written;

    failure_output readBack{{9, "Replaced by what is read"}};
    s >> readBack;

    check(equality, "operator>> reads back what operator<< writes, in place of what it held", readBack, written);

    const failure_info tenth{10, "foo"};
    std::stringstream infoStream{}, outputStream{};
    infoStream   << std::hex << tenth;
    outputStream << std::hex << failure_output{tenth};

    failure_info infoReadBack{};
    infoStream >> infoReadBack;
    check(equality, "A hexadecimal stream does not change a failure_info's text", infoReadBack, tenth);

    failure_output outputReadBack{};
    outputStream >> outputReadBack;
    check(equality, "A hexadecimal stream does not change a failure_output's text", outputReadBack, failure_output{tenth});
  }

  void failure_info_test::check_end_of_stream()
  {
    std::stringstream s{};
    failure_info info{1, "foo"};
    s >> info;

    check("A read at the end of a stream fails", s.fail());
    check(equality, "A read at the end of a stream leaves the failure as it was", info, failure_info{1, "foo"});
  }

  void failure_info_test::check_instability_analysis()
  {
    write_to_file(working_materials() / "StableAnalysis.txt",
                  instability_analysis(working_materials() / "Stable", 2),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which agree, beside a file which is not a .txt file",
      working_materials() / "StableAnalysis.txt",
      predictive_materials() / "StableAnalysis.txt"
    );
  }

  void failure_info_test::check_written_format()
  {
    std::stringstream s{};
    s << failure_output{{0, ""}, {1, "foo\n$\nbar"}};
    write_to_file(working_materials() / "Written.txt", s.str(), std::ios_base::binary);

    check(
      equivalence,
      "The format operator<< writes",
      working_materials() / "Written.txt",
      predictive_materials() / "Written.txt"
    );
  }

  void failure_info_test::run_tests()
  {
    check_exceptions();
    check_failure_info();
    check_round_trip();
    check_end_of_stream();
    check_instability_analysis();
    check_written_format();
  }
}
