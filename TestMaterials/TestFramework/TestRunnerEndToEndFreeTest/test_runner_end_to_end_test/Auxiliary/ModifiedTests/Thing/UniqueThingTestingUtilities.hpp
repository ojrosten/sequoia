////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"
#include "generatedProject/Utilities/Thing/UniqueThing.hpp"

namespace sequoia::testing
{
	template<> struct value_tester<stuff::unique_thing>
	{
		using type = stuff::unique_thing;

		template<test_mode Mode>
		static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
		{
			check(equality, "Description", logger, actual.get(), prediction.get());
		}
		
		template<test_mode Mode>
		static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const double& prediction)
		{
			check(equality, "Description", logger, actual.get(), prediction);
		}
	};

	template<>
	struct serializer<stuff::unique_thing>
	{
		[[nodiscard]]
		static std::string make(const stuff::unique_thing& thing) { return std::format("{}", thing.get()); }
	};
}
