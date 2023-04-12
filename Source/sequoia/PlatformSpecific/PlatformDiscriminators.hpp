////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Types to descriminate different compilers
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
}