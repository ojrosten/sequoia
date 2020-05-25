////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

namespace sequoia::testing
{
  class test_static_queue_false_positives final : public false_positive_regular_test
  {
  public:
    using false_positive_regular_test::false_positive_regular_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:    
    void run_tests() final;

    void check_depth_0(); 
    void check_depth_1();   
  };
}
