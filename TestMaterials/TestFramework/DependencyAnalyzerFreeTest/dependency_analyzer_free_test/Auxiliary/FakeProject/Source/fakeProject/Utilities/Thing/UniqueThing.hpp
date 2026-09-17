////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <compare>

namespace stuff
{
	class unique_thing
	{
		double m_Thing{};
	public:
		explicit unique_thing(double t) : m_Thing{t}
		{}
	
		unique_thing(const unique_thing&)     = delete;
		unique_thing(unique_thing&&) noexcept = default;

		unique_thing& operator=(const unique_thing&)     = delete;
		unique_thing& operator=(unique_thing&&) noexcept = default;
		
		[[nodiscard]]
		double get() const noexcept;

		[[nodiscard]]
		friend bool operator==(const unique_thing&, const unique_thing&) noexcept = default;

		[[nodiscard]]
		friend auto operator<=>(const unique_thing&, const unique_thing&) noexcept = default;
	};
}
