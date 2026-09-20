////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <compare>

namespace other::functional
{
	template<class T>
	class maybe
	{
	public:
		[[nodiscard]]
		friend bool operator==(const maybe&, const maybe&) noexcept = default;

		[[nodiscard]]
		friend auto operator<=>(const maybe&, const maybe&) noexcept = default;
	};
}
