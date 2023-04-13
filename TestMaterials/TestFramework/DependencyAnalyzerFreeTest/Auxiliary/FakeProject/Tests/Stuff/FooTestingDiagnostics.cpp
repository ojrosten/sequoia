////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FooTestingDiagnostics.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path foo_false_positive_test::source_file() const noexcept
	{
		return std::source_location::current().file_name();
	}

	void foo_false_positive_test::run_tests()
	{
		// For example:

		// auto x = []() { return bar::baz::foo<T>{args}; };
		// auto y = []() { return bar::baz::foo<T>{different args}; };
		// check_equivalence(report_line("Useful Description"), x(), something inequivalent - ordinarily this would fail);
		// check_equality(report_line("Useful Description"), x(), y());
	}
}
