////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"
#include "generatedProject/Utilities/Thing/UniqueThing.hpp"
#include "../../Stuff/FooTestingUtilities.hpp"

namespace sequoia::testing
{
	template<> struct value_checker<stuff::unique_thing>
	{
		using type = stuff::unique_thing;

		template<test_mode Mode>
		static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
		{
			check_equality("Description", logger, actual.get(), prediction.get());
		}
	};

	template<> struct equivalence_checker<stuff::unique_thing>
	{
		using type = stuff::unique_thing;

		template<test_mode Mode>
		static void check(test_logger<Mode>& logger, const type& actual, const double& prediction)
		{
			check_equality("Description", logger, actual.get(), prediction);
		}
	};
}
