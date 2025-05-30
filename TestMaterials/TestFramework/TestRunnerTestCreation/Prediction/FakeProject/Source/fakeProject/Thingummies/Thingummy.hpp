////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <compare>

namespace stuff
{
    template<class T>
    class thingummy
    {
    public:
        [[nodiscard]]
        friend auto operator<=>(const thingummy&, const thingummy&) noexcept = default;
    };
}
