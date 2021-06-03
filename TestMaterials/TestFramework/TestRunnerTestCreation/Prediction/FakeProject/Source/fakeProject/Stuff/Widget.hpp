////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

namespace stuff
{
    class widget
    {
    public:
        [[nodiscard]]
        friend bool operator==(const widget&, const widget&) noexcept = default;

        [[nodiscard]]
        friend bool operator!=(const widget&, const widget&) noexcept = default;
    };
}
