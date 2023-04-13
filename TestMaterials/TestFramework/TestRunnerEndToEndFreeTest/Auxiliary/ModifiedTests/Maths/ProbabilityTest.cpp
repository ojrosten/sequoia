////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProbabilityTest.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path probability_test::source_file() const noexcept
	{
		return std::source_location::current().file_name();
	}

	void probability_test::run_tests()
	{
		maths::probability x{0.5}, y{1.0};
		check(equivalence, report_line(""), x, 0.5);
		check(equivalence, report_line(""), y, 1.0);
		check_semantics(report_line(""), x, y, std::weak_ordering::less);
	}
}
