////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProbabilityTestingDiagnostics.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path probability_false_positive_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void probability_false_positive_test::run_tests()
	{
		maths::probability x{0.5}, y{1.0};
		check(equivalence, report_line(""), x, 0.6);
		check(equality, report_line(""), x, y);
	}
}
