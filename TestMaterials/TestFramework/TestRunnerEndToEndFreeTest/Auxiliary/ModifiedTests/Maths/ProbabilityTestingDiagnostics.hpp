////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProbabilityTestingUtilities.hpp"

namespace sequoia::testing
{
	class probability_false_positive_test final : public regular_false_positive_test
	{
	public:
		using regular_false_positive_test::regular_false_positive_test;

		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
}
