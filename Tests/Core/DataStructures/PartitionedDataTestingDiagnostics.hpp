////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedDataTestingUtilities.hpp"

namespace sequoia::testing
{
  class partitioned_data_false_positive_test final : public false_positive_regular_test
  {
  public:
    using false_positive_regular_test::false_positive_regular_test;    

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:    
    void run_tests() final;

    template<class T> void test_set();

    template<class PartitionedData> void test();
  };
}
