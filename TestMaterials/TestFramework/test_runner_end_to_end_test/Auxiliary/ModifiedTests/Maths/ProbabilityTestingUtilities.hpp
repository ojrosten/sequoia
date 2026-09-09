////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "generatedProject/Maths/Probability.hpp"

namespace sequoia::testing
{
	template<> struct value_tester<maths::probability>
	{
		using type = maths::probability;

		template<test_mode Mode>
		static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
		{
			check(equality, "", logger, actual.raw_value(), prediction.raw_value());
		}
		
		template<test_mode Mode>
		static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const double& prediction)
		{
			check(equality, "Description", logger, actual.raw_value(), prediction);
		}
	};
}
