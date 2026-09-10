////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The handful of macros sequoia defines, and nothing else.

    Macros do not cross a module boundary: a named module exports declarations,
    never `#define`s. Keeping them in a header of their own lets a module
    include exactly this in its global module fragment, without dragging in the
    declarations that would then belong to two modules at once.
 */

#if defined(_MSC_VER)
  #define SEQUOIA_MSVC_EMPTY_BASE_HACK __declspec(empty_bases)
  #define SEQUOIA_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
  #define SEQUOIA_MSVC_EMPTY_BASE_HACK
  #define SEQUOIA_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if defined(_MSC_VER) && !defined(__clang__)
  #define NAMESPACE_SEQUOIA_AS_BITMASK inline namespace sequoia_bitmask
#else
  #define NAMESPACE_SEQUOIA_AS_BITMASK namespace sequoia
#endif

/** Suppresses a gcc warning over a span of code, and expands to nothing elsewhere.

    gcc only: clang lacks some of these warning names, and MSVC warns C4068 on a pragma it does not
    recognise. Each use should say at the site why the diagnostic is wrong.
 */

#if defined(__GNUG__) && !defined(__clang__)
  #define SEQUOIA_DO_PRAGMA(x) _Pragma(#x)
  #define SEQUOIA_GCC_SUPPRESS_BEGIN(warning) \
    _Pragma("GCC diagnostic push")            \
    SEQUOIA_DO_PRAGMA(GCC diagnostic ignored warning)
  #define SEQUOIA_GCC_SUPPRESS_END _Pragma("GCC diagnostic pop")
#else
  #define SEQUOIA_GCC_SUPPRESS_BEGIN(warning)
  #define SEQUOIA_GCC_SUPPRESS_END
#endif
