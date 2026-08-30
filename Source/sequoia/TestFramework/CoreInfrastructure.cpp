////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "sequoia/TestFramework/CoreInfrastructure.hpp"

#include <format>

namespace sequoia::testing::impl
{
  [[nodiscard]] std::string serialize(bool x)               { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(char x)               { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(long long x)          { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(unsigned long long x) { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(float x)              { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(double x)             { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(long double x)        { return std::format("{}", x); }
  [[nodiscard]] std::string serialize(std::string_view x)   { return std::format("{}", x); }
}
