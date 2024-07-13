////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/?TestCore.hpp"

namespace sequoia::testing
{
	class ?forename_?surname final : public ?_test
	{
	public:
		using ?_test::?_test;

		[[nodiscard]]
		std::filesystem::path source_file() const;

		void run_tests();
	};
}
