////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia
{
  namespace testing
  {
    class test_edges final : public regular_test
    {
    public:
      using regular_test::regular_test;

      [[nodiscard]]
      std::filesystem::path source_file() const;

      void run_tests();
    private:

      void test_plain_partial_edge();
      void test_partial_edge_indep_weight();
      void test_partial_edge_shared_weight();
      void test_partial_edge_conversions();

      void test_partial_edge_meta_data();
      void test_partial_edge_indep_weight_meta_data();
      void test_partial_edge_shared_weight_meta_data();
      void test_partial_edge_meta_data_conversions();

      void test_plain_embedded_partial_edge();
      void test_embedded_partial_edge_indep_weight();
      void test_embedded_partial_edge_shared_weight();
      void test_embedded_partial_edge_conversions();

      void test_embedded_partial_edge_meta_data();
      void test_embedded_partial_edge_indep_weight_meta_data();
      void test_embedded_partial_edge_shared_weight_meta_data();
      void test_embedded_partial_edge_meta_data_conversions();
    };
  }
}
