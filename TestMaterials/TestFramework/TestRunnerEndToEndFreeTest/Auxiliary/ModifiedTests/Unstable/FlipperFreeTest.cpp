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
	std::filesystem::path flipper_free_test::source_file() const noexcept
	{
		return std::source_location::current().file_name();
	}

	void flipper_free_test::run_tests()
	{
		check(equality, report_line(""), flipper{}.x, true);
		check(report_line(""), !fs::exists(output_paths::instability_analysis(project_root()) / "FlipperFreeTest_cpp"));
	}
}
