////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorPolarCoordinatesTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path vector_polar_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_polar_coordinates_test::run_tests()
  {
  }
}
