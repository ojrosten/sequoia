////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <compare>

class human
{
public:
    [[nodiscard]]
    friend bool operator==(const human&, const human&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const human&, const human&) noexcept = default;
};