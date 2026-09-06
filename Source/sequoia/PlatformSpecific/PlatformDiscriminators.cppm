////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.platform_specific:PlatformDiscriminators;

import std;

/** \file
    \brief Types to discriminate different compilers and operating systems

    The two are independent axes and must not stand in for one another: clang builds for Windows,
    and an executable's `.exe` suffix is a property of the platform rather than of the compiler.
 */

export namespace sequoia
{
  enum class compiler_flavour { clang, gcc, msvc, other };

  template<compiler_flavour F>
  using compiler_flavour_constant = std::integral_constant<compiler_flavour, F>;

  using clang_type          = compiler_flavour_constant<compiler_flavour::clang>;
  using gcc_type            = compiler_flavour_constant<compiler_flavour::gcc>;
  using msvc_type           = compiler_flavour_constant<compiler_flavour::msvc>;
  using other_compiler_type = compiler_flavour_constant<compiler_flavour::other>;

  enum class operating_system { windows, macos, linux, other };

  template<operating_system S>
  using operating_system_constant = std::integral_constant<operating_system, S>;

  using windows_type      = operating_system_constant<operating_system::windows>;
  using macos_type        = operating_system_constant<operating_system::macos>;
  using linux_type        = operating_system_constant<operating_system::linux>;
  using other_os_type     = operating_system_constant<operating_system::other>;
}
