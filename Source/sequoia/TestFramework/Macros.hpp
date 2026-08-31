////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The test framework's macros, and nothing else.

    A named module exports declarations, never a `#define`, so this header is
    included in the global module fragment of anything that needs it. That is
    what the global module fragment is for, and it is why keeping a macro is not
    a retreat from a modules migration.

    STATIC_CHECK earns its keep by being a macro. A function taking the
    condition as a template argument sees a `bool`, so the compiler reports
    "static assertion failed" and nothing else; the macro puts the *expression*
    inside the static_assert, and the compiler then echoes it with every
    templated type substituted - which is the whole value when the types are
    complex.
 */

#define STATIC_CHECK(...) (check("", [&](){ static_assert(__VA_ARGS__); return true; }()))
