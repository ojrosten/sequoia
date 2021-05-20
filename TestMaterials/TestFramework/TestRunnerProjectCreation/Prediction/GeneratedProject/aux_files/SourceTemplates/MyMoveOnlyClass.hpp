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
	class ?class
	{
	public:
		?class(const ?class&)     = delete;
		?class(?class&&) noexcept = default;

		?class& operator=(const ?class&)     = delete;
		?class& operator=(?class&&) noexcept = default;

		[[nodiscard]]
		friend bool operator==(const ?class&, const ?class&) noexcept = default;

		[[nodiscard]]
		friend bool operator!=(const ?class&, const ?class&) noexcept = default;
	};
}
