////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

namespace 
{
	template<?>
	class ?_class
	{
	public:
		?_class(const ?_class&) = delete;
		?_class(?_class&&)      = delete;

		?_class& operator=(const ?_class&) = delete;
		?_class& operator=(?_class&&)      = delete;

		[[nodiscard]]
		friend bool operator==(const ?class&, const ?class&) = default;

		[[nodiscard]]
		friend bool operator!=(const ?class&, const ?class&) = default;
	};
}
