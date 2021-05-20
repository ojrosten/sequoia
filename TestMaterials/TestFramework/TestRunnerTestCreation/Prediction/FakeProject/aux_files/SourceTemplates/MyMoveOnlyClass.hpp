////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

namespace
{
	template<?> class ?type
	{
	public:
		?type(const ?type&)     = delete;
		?type(?type&&) noexcept = default;

		?type& operator=(const ?type&)     = delete;
		?type& operator=(?type&&) noexcept = default;

		[[nodiscard]]
		friend bool operator==(const ?type&, const ?type&) noexcept = default;

		[[nodiscard]]
		friend bool operator!=(const ?type&, const ?type&) noexcept = default;
	};
}
