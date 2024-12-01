////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "fakeProject/Maths/Probability.hpp"
#include "fakeProject/Maths/Helper.hpp"
#include "foo/Utilities/Helper.hpp"

namespace maths
{
	[[nodiscard]]
	double probability::raw_value() const noexcept { return m_Prob; }
}
