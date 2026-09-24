////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "?TestFile.hpp"
#include "?Header.hpp"

namespace ?::testing
{
	[[nodiscard]]
	std::filesystem::path ?test_name::source_file()
	{
		return std::source_location::current().file_name();
	}

	void ?test_name::run_tests()
	{
		// e.g. check(equality, "Useful description", some_function(), 42);
	}
}
