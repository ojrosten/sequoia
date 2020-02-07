////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  class monotonic_sequence_false_positive_test final : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;    

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:    
    void run_tests() final;
  };
}
