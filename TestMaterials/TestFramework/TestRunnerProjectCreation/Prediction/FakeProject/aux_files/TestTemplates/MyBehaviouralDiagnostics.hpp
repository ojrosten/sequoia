////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/?TestCore.hpp"

namespace sequoia::testing
{
	class ?forename_false_positive_?surname final : public ?_false_positive_test
	{
	public:
		using ?_false_positive_test::?_false_positive_test;

	private:
		[[nodiscard]]
		std::string_view source_file() const noexcept final;

		void run_tests() final;
	};
	
	class ?forename_false_negative_?surname final : public ?_false_negative_test
	{
	public:
		using ?_false_negative_test::?_false_negative_test;

	private:
		[[nodiscard]]
		std::string_view source_file() const noexcept final;

		void run_tests() final;
	};
}
