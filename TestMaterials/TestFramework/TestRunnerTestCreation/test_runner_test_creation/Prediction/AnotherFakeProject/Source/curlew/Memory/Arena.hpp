////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include <compare>

class arena
{
public:
    arena(const arena&)     = delete;
    arena(arena&&) noexcept = default;

    arena& operator=(const arena&)     = delete;
    arena& operator=(arena&&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const arena&, const arena&) noexcept = default;
};
