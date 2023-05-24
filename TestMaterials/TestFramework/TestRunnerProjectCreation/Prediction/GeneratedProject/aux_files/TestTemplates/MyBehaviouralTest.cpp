////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "?BehaviouralTest.hpp"
#include "?Header.hpp"

namespace sequoia::testing
{
	[[nodiscard]]
	std::filesystem::path ?forename_?surname::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void ?forename_?surname::run_tests()
	{
		// e.g. check(equality, report_line("Useful description"), some_function(), 42);
	}
}
