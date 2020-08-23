////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

namespace sequoia
{
  namespace testing
  {    
    class test_edges final : public regular_test
    {
    public:
      using regular_test::regular_test;

      [[nodiscard]]
      std::string_view source_file() const noexcept final;
    private:      
      struct null_weight{};
      
      void run_tests() final;

      void test_plain_partial_edge();
      void test_partial_edge_indep_weight();
      void test_partial_edge_shared_weight();

      void test_plain_embedded_partial_edge();
      void test_embedded_partial_edge_indep_weight();
      void test_embedded_partial_edge_shared_weight();
      
      void test_plain_edge();     
      void test_weighted_edge();

      void test_plain_embedded_edge();
      void test_embedded_edge_indep_weight();
      void test_embedded_edge_shared_weight();
    };
  }
}
