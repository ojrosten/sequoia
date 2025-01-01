////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <compare>

class cloud
{
public:
    cloud(const cloud&)     = delete;
    cloud(cloud&&) noexcept = default;

    cloud& operator=(const cloud&)     = delete;
    cloud& operator=(cloud&&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const cloud&, const cloud&) noexcept = default;
};
