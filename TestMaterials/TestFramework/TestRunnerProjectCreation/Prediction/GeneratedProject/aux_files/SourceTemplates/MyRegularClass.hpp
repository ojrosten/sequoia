////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <compare>

namespace
?{
	template<?>
	class ?type
	{
	public:
		[[nodiscard]]
		friend bool operator==(const ?type&, const ?type&) noexcept = default;

		[[nodiscard]]
		friend auto operator<=>(const ?type&, const ?type&) noexcept = default;
	};
?}
