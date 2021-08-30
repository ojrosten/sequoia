////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <ostream>
#include <string>

namespace sequoia::testing
{
  void report(std::ostream& stream, std::string_view prefix, std::string_view message);

  void set_top_copyright(std::string& text, std::string_view copyright);
}