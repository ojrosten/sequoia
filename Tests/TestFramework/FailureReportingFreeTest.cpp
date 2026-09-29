////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FailureReportingFreeTest.hpp"
#include "sequoia/TestFramework/FailureReporting.hpp"

#include <cstdlib>
#include <stdexcept>

namespace sequoia::testing
{
  using namespace std::string_literals;

  /// In a named namespace, since toolchains spell an anonymous namespace differently
  struct derived_logic_error : std::logic_error
  {
    using std::logic_error::logic_error;
  };

  namespace
  {
    [[noreturn]]
    void first_handler() noexcept
    {
      std::abort();
    }

    /// Its body differs from `first_handler`'s, so that a linker folding identical functions cannot merge the two
    [[noreturn]]
    void second_handler() noexcept
    {
      std::_Exit(EXIT_FAILURE);
    }
  }

  [[nodiscard]]
  std::filesystem::path failure_reporting_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void failure_reporting_free_test::run_tests()
  {
    test_describe_exception();
    test_scoped_terminate_handler();
  }

  void failure_reporting_free_test::test_describe_exception()
  {
    check(equality, "No exception", describe_exception(nullptr), "none"s);

    check(equality,
          "A standard exception",
          describe_exception(std::make_exception_ptr(std::runtime_error{"Oops"})),
          "std::runtime_error: Oops"s);

    check(equality,
          "An exception derived from a standard one, named by its own type",
          describe_exception(std::make_exception_ptr(derived_logic_error{"Bad"})),
          "sequoia::testing::derived_logic_error: Bad"s);

    check(equality,
          "An exception not derived from std::exception",
          describe_exception(std::make_exception_ptr(42)),
          "an exception not derived from std::exception"s);
  }

  void failure_reporting_free_test::test_scoped_terminate_handler()
  {
    const auto outermost{std::get_terminate()};

    {
      const scoped_terminate_handler first{first_handler};
      check("The first handler is installed", std::get_terminate() == first_handler);

      {
        const scoped_terminate_handler second{second_handler};
        check("The second handler is installed", std::get_terminate() == second_handler);
      }

      check("The first handler is restored", std::get_terminate() == first_handler);
    }

    check("The outermost handler is restored", std::get_terminate() == outermost);
  }

  [[nodiscard]]
  std::filesystem::path failure_reporting_in_parallel_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void failure_reporting_in_parallel_free_test::run_tests()
  {
    check("Termination is reported on the thread running this test", std::get_terminate() == report_termination);
  }
}
