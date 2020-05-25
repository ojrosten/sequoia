////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IteratorTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view iterator_false_positive_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void iterator_false_positive_test::run_tests()
  {
    // TO DO
  }
}
