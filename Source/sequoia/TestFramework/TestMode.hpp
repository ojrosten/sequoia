////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

namespace sequoia::testing
{
  /*! \brief Specifies whether tests are run as standard tests or in false postive/negative mode.

      \anchor test_mode_enum
   */
  enum class test_mode { standard, false_negative, false_positive };
}