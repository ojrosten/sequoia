////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

// `import std` exports declarations, not macros, so _ITERATOR_DEBUG_LEVEL has
// to come from the header that owns it. <yvals.h> is the MSVC STL's config
// header: it derives the macro from _HAS_ITERATOR_DEBUGGING and _SECURE_SCL,
// and pulls in nothing but CRT configuration, so it cannot collide with the
// declarations `import std` supplies. Deriving the level ourselves would risk
// silently disagreeing with the STL, and the allocation tests branch on it.
#if defined(_MSC_VER)
  #include <yvals.h>
#endif

export module sequoia.platform_specific:Preprocessor;

import std;

import :PlatformDiscriminators;

/** \file Preprocessor logic for dealing with different platforms */

export namespace sequoia
{

  #if defined(_MSC_VER)
    using compiler_constant = msvc_type;

    [[nodiscard]]
    constexpr int iterator_debug_level() noexcept
    {
      return _ITERATOR_DEBUG_LEVEL;
    }
  #else
    #if defined(__clang__)
      using compiler_constant = clang_type;
    #elif defined(__GNUG__)
      using compiler_constant = gcc_type;
    #else
      using compiler_constant = other_compiler_type;
    #endif

    int iterator_debug_level() noexcept;
  #endif

  #if defined(__clang__)
    namespace execution
    {
      inline constexpr int par{0};
    }
  #else
    namespace execution
    {
      inline constexpr auto par{std::execution::par};
    }
  #endif

  inline constexpr bool with_msvc_v{std::is_same_v<compiler_constant, msvc_type>};
  inline constexpr bool with_clang_v{std::is_same_v<compiler_constant, clang_type>};
  inline constexpr bool with_gcc_v{std::is_same_v<compiler_constant, gcc_type>};

  [[nodiscard]]
  inline std::string compiler_name()
  {
    if constexpr(with_clang_v)
      return "clang";
    else if(with_gcc_v)
      return "gcc";
    else if(with_msvc_v)
      return "msvc";

    return "";
  }
}
