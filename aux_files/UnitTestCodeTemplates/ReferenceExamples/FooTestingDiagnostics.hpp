////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FooTestingUtilities.hpp"

namespace sequoia::testing
{
  class foo_false_positive_test final : public false_positive_regular_test
  {
  public:
    using false_positive_regular_test::false_positive_regular_test;    

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:    
    void run_tests() final;
  };
}
