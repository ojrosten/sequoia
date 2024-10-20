////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "fakeProject/Stuff/NoTemplate.hpp"
#include "myLib/Utils.hpp"

namespace sequoia::testing
{
	template<> struct value_checker<stuff::oldschool>
	{
		using type = stuff::oldschool;

		template<test_mode Mode>
		static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
		{
			// e.g. check_equality("Description", logger, actual.method(), prediction.method());
		}
	};

	template<> struct equivalence_checker<stuff::oldschool>
	{
		using type = stuff::oldschool;

		template<test_mode Mode>
		static void check(test_logger<Mode>& logger, const type& actual, const double prediction)
		{
			// e.g. check_equality("Description", logger, actual.method(), prediction);
		}
	};
}
