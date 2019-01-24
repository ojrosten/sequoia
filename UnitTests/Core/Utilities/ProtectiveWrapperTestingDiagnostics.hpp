////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProtectiveWrapperTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  class protective_wrapper_false_positive_test : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;    
    
  private:    
    void run_tests() override;

    void test_basic_type();

    void test_container_type();

    void test_aggregate_type();
  };

}
