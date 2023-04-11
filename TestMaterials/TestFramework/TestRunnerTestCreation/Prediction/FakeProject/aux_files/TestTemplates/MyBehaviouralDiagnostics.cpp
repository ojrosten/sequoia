////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2022.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "?BehaviouralDiagnostics.hpp"
#include "?Header.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::string_view ?forename_false_positive_?surname::source_file() const noexcept
	{
		return __FILE__;
	}

	void ?forename_false_positive_?surname::run_tests()
	{
		// e.g. check(equality, report_line("Useful description"), some_function(), 42);
	}

	[[nodiscard]]
	std::string_view ?forename_false_negative_?surname::source_file() const noexcept
	{
		return __FILE__;
	}

	void ?forename_false_negative_?surname::run_tests()
	{
		// e.g. check(equality, report_line("Useful description"), some_function(), 42);
	}
}
