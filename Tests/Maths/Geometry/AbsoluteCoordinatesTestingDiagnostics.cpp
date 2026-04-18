////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsoluteCoordinatesTestingDiagnostics.hpp"


namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path absolute_coordinates_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_coordinates_false_negative_test::run_tests()
  {
  }

  void absolute_coordinates_false_negative_test::test_absolute()
  {
  }
}
