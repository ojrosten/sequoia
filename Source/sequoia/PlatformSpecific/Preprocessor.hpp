////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Preprocessor logic for dealing with different platforms
 */

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/PlatformSpecific/PlatformDiscriminators.hpp"

#include <execution>
#include <vector>

namespace sequoia
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

  /** Whether the standard library supplies the parallel algorithms.

      Ask the library, not the compiler. libstdc++ has them, on oneAPI TBB; libc++ does not. Which
      of those a build gets is a property of the *library*, and a clang build may have either -
      libc++ on macOS, libstdc++ on Linux - so `__clang__` answers a different question and answers
      it wrongly for half its cases. `__cpp_lib_parallel_algorithm` (P0024R2) asks the one that
      matters.

      Keeping this as a single constant is what stops `par` and its consumers disagreeing. They did:
      choosing the stand-in on `__clang__` while `accelerate` branched on `with_clang_v` meant
      clang-targeting-Windows, which defines both `__clang__` and `_MSC_VER`, took the
      `std::for_each` branch and handed it an `int` as an execution policy - roadmap item 122. With
      one discriminator that class of mismatch cannot be written.
   */

  #if defined(__cpp_lib_parallel_algorithm)
    inline constexpr bool has_parallel_algorithms_v{true};

    namespace execution
    {
      inline constexpr auto par{std::execution::par};
    }
  #else
    inline constexpr bool has_parallel_algorithms_v{false};

    /** A stand-in, so that a call taking an execution policy still compiles where there are no
        parallel algorithms. Nothing may dereference it; `has_parallel_algorithms_v` gates its use.
     */

    namespace execution
    {
      inline constexpr int par{0};
    }
  #endif

  #if defined(_WIN32)
    using platform_constant = windows_type;
  #elif defined(__APPLE__)
    using platform_constant = macos_type;
  #elif defined(__linux__)
    using platform_constant = linux_type;
  #else
    using platform_constant = other_os_type;
  #endif

  inline constexpr bool with_msvc_v{std::is_same_v<compiler_constant, msvc_type>};
  inline constexpr bool with_clang_v{std::is_same_v<compiler_constant, clang_type>};
  inline constexpr bool with_gcc_v{std::is_same_v<compiler_constant, gcc_type>};

  inline constexpr bool with_windows_v{std::is_same_v<platform_constant, windows_type>};
  inline constexpr bool with_macos_v{std::is_same_v<platform_constant, macos_type>};
  inline constexpr bool with_linux_v{std::is_same_v<platform_constant, linux_type>};

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
