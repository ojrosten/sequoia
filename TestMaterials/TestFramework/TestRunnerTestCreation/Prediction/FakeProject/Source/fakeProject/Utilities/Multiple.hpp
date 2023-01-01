////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <compare>

template<class... T>
class multiple
{
public:
    multiple(const multiple&)     = delete;
    multiple(multiple&&) noexcept = default;

    multiple& operator=(const multiple&)     = delete;
    multiple& operator=(multiple&&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const multiple&, const multiple&) noexcept = default;
};
