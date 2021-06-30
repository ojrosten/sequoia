#pragma once

#include "EdgeTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_edge_false_positives final : public regular_false_positive_test
  {
  public:
    using regular_false_positive_test::regular_false_positive_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
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
