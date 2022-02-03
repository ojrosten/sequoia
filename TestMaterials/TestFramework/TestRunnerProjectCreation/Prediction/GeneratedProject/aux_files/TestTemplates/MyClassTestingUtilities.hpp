////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/?TestCore.hpp"
#include "?Class.hpp"

namespace sequoia::testing
{
	template<?> struct value_tester<::?_class>
	{
		using type = ::?_class;

		template<test_mode Mode>
		static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
		{
			// e.g. check_equality("Description", logger, actual.method(), prediction.method());
		}
		
		template<test_mode Mode>
		static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, ?args)
		{
			// e.g. check_equality("Description", logger, actual.method(), ?predictions);
		}
	};
}
