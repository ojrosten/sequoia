////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FailureInfoTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path failure_info_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void failure_info_test::check_exceptions()
  {
    check_exception_thrown<std::runtime_error>(report_line("Stream does not start with expected pattern"), [](){
      std::stringstream s{};
      s << "foo";

      failure_info x{};
      s >> x;
    });

    check_exception_thrown<std::runtime_error>(report_line("Stream does not contain index"), [](){
      std::stringstream s{};
      s << "$Check: foo";

      failure_info x{};
      s >> x;
    });

    check_exception_thrown<std::runtime_error>(report_line("Stream does not contain message"), [](){
      std::stringstream s{};
      s << "$Check: 1\n";

      failure_info x{};
      s >> x;
    });
  }

  void failure_info_test::check_failure_info()
  {
    failure_info x{}, y{1, "foo"};
    check(equivalence, report_line(""), x, 0, "");
    check(equivalence, report_line(""), y, 1, "foo");

    check_semantics(report_line(""), x, y, std::weak_ordering::less);

    failure_info z{42, "..$.."};
    check_semantics(report_line("Intermediate $"), x, z, std::weak_ordering::less);
  }

  void failure_info_test::run_tests()
  {
    check_exceptions();
    check_failure_info();
  }
}
