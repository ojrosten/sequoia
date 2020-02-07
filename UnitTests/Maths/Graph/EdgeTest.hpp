////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia
{
  namespace unit_testing
  {    
    class test_edges final : public unit_test
    {
    public:
      using unit_test::unit_test;

      [[nodiscard]]
      std::string_view source_file_name() const noexcept final;
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
