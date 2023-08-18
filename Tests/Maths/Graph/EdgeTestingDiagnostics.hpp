#pragma once

/*! \file */

#include "EdgeTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_edge_false_positives final : public regular_false_positive_test
  {
  public:
    using regular_false_positive_test::regular_false_positive_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_plain_partial_edge();
    void test_partial_edge_indep_weight();
    void test_partial_edge_shared_weight();
    void test_partial_edge_meta_data();
    void test_partial_edge_indep_weight_meta_data();
    void test_partial_edge_shared_weight_meta_data();

    void test_plain_embedded_partial_edge();
    void test_embedded_partial_edge_indep_weight();
    void test_embedded_partial_edge_shared_weight();
    void test_embedded_partial_edge_meta_data();
  };

}
