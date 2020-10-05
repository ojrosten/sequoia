////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MaybeTestingUtilities.hpp"

namespace sequoia::testing
{
  class maybe_false_positive_test final : public regular_false_positive_test
  {
  public:
    using regular_false_positive_test::regular_false_positive_test;    

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:    
    void run_tests() final;
  };
}
