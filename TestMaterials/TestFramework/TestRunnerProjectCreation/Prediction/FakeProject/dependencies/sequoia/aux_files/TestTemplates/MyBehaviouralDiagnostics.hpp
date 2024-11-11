////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/?TestCore.hpp"

namespace ?::testing
{
	using namespace sequoia::testing;class ?forename_false_negative_?surname final : public ?_false_negative_test
	{
	public:
		using ?_false_negative_test::?_false_negative_test;

		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
	
	class ?forename_false_negative_?surname final : public ?_false_negative_test
	{
	public:
		using ?_false_negative_test::?_false_negative_test;

		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
}
