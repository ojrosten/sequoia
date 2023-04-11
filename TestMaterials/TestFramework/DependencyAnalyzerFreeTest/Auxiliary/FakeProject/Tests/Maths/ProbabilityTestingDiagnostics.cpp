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
	std::string_view probability_false_positive_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void probability_false_positive_test::run_tests()
	{
		maths::probability x{0.5}, y{1.0};
		check_equivalence(report_line(""), x, 0.6);
		check_equality(report_line(""), x, y);
	}
}
