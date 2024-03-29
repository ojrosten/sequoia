////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "fakeProject/Maybe/Maybe.hpp"

namespace sequoia::testing
{
	template<class T>
	struct value_checker<other::functional::maybe<T>>
	{
		using type = other::functional::maybe<T>;

		template<test_mode Mode>
		static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
		{
			// e.g. check_equality("Description", logger, actual.method(), prediction.method());
		}
	};

	template<class T>
	struct equivalence_checker<other::functional::maybe<T>>
	{
		using type = other::functional::maybe<T>;

		template<test_mode Mode>
		static void check(test_logger<Mode>& logger, const type& actual, const std::optional<T>& prediction)
		{
			// e.g. check_equality("Description", logger, actual.method(), prediction);
		}
	};
}
