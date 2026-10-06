////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include <compare>

namespace stuff
{
    class gadget
    {
    public:
        gadget(const gadget&)     = delete;
        gadget(gadget&&) noexcept = default;

        gadget& operator=(const gadget&)     = delete;
        gadget& operator=(gadget&&) noexcept = default;

        [[nodiscard]]
        friend auto operator<=>(const gadget&, const gadget&) noexcept = default;
    };
}
