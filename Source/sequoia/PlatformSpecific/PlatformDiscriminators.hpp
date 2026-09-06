////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Types to discriminate different compilers and operating systems

    The two are independent axes and must not stand in for one another: clang builds for Windows,
    and an executable's `.exe` suffix is a property of the platform rather than of the compiler.
 */

#include <type_traits>

namespace sequoia
{
  enum class compiler_flavour { clang, gcc, msvc, other };

  template<compiler_flavour F>
  using compiler_flavour_constant = std::integral_constant<compiler_flavour, F>;

  using clang_type          = compiler_flavour_constant<compiler_flavour::clang>;
  using gcc_type            = compiler_flavour_constant<compiler_flavour::gcc>;
  using msvc_type           = compiler_flavour_constant<compiler_flavour::msvc>;
  using other_compiler_type = compiler_flavour_constant<compiler_flavour::other>;

  // `gnu_linux`, not `linux`, because GCC predefines `linux` as `1` in its GNU
  // dialects - which is what CMake selects unless CXX_EXTENSIONS is off - so an
  // enumerator of that name expands to a numeric constant on the very platform it
  // names. Undefining the macro here would keep the prettier spelling and hand the
  // same trap to every client writing `operating_system::linux`.
  enum class operating_system { windows, macos, gnu_linux, other };

  template<operating_system S>
  using operating_system_constant = std::integral_constant<operating_system, S>;

  using windows_type      = operating_system_constant<operating_system::windows>;
  using macos_type        = operating_system_constant<operating_system::macos>;
  using linux_type        = operating_system_constant<operating_system::gnu_linux>;
  using other_os_type     = operating_system_constant<operating_system::other>;
}