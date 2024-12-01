////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UniqueThingTestingUtilities.hpp"

namespace generatedProject::testing
{
	using namespace sequoia::testing;

	class unique_thing_false_negative_test final : public move_only_false_negative_test
	{
	public:
		using move_only_false_negative_test::move_only_false_negative_test;

		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
}
