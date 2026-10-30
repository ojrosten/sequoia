////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module sequoia.test_framework;

import std;

namespace sequoia::testing
{
  [[nodiscard]]
  std::string instability_projection(std::string_view message)
  {
    return text_with_zeroed_measurements(message);
  }
}
