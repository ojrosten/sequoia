////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/PlatformSpecific/Macros.hpp"

import std;
import sequoia.maths.geometry;

/** \file
    \brief Validator fixtures shared by the compile-time and run-time validator tests.

    Both are deliberately partial: they exist to separate the disjuncts of
    `validator_for`, which the production validators cannot do because they
    satisfy every one of them.
 */

namespace sequoia::testing
{
  /** \brief Accepts a single value and no array.

      This separates `validator_for`'s two disjuncts, and - being unprivileged -
      shows that `defines_identity_validator_v` is a declaration rather than an
      inference: it checks nothing, exactly as `identity_validator` does, and
      still answers false.
   */
  struct single_value_validator
  {
    template<maths::bounds Bounds>
    [[nodiscard]] constexpr double operator()(Bounds, double val) const noexcept { return val; }
  };

  /** \brief Callable, but on nothing the framework will ever offer it. */
  struct unusable_validator
  {
    [[nodiscard]] constexpr int operator()(int i) const noexcept { return i; }
  };
}
