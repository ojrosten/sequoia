////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "fakeProject/Utilities/Thing/UniqueThing.hpp"

namespace stuff
{
	[[nodiscard]]
	double unique_thing::get() const noexcept
	{
		return m_Thing;
	}
}
