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
	std::string_view probability_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void probability_test::run_tests()
	{
		maths::probability x{0.5}, y{1.0};
		check(equivalence, LINE(""), x, 0.5);
		check(equivalence, LINE(""), y, 1.0);
		check_semantics(LINE(""), x, y, std::weak_ordering::less);
	}
}
