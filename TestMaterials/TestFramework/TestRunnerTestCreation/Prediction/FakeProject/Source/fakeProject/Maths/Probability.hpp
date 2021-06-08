////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <compare>

namespace maths
{
    class probability
    {
    public:
        [[nodiscard]]
        friend bool operator==(const probability&, const probability&) noexcept = default;

        [[nodiscard]]
        friend auto operator<=>(const probability&, const probability&) noexcept = default;
    };
}
