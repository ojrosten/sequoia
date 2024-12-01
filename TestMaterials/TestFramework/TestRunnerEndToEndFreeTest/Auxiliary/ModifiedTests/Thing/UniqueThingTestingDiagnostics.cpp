////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UniqueThingTestingDiagnostics.hpp"

namespace generatedProject::testing
{
	[[nodiscard]]
	std::filesystem::path unique_thing_false_negative_test::source_file() const
	{
		return std::source_location::current().file_name();
	}

	void unique_thing_false_negative_test::run_tests()
	{
		auto x = []() { return stuff::unique_thing{0.0}; };
		auto y = []() { return stuff::unique_thing{1.0}; };

		check(equivalence, "Useful Description", x(), 1.0);
		check(equality, "Useful Description", x(), y());
	}
}
