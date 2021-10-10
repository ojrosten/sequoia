////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FlipperFreeTest.hpp"
#include "generatedProject/Unstable/Flipper.hpp"

namespace sequoia::testing
{
	namespace fs = std::filesystem;
	using namespace unstable;

	[[nodiscard]]
	std::string_view flipper_free_test::source_file() const noexcept
	{
		return __FILE__;
	}

	void flipper_free_test::run_tests()
	{
		check_equality(LINE(""), flipper{}.x, true);
		check(LINE(""), !fs::exists(temp_test_summaries_path(test_repository().parent_path() / "output")));
	}
}
