#pragma once

#include "EdgeTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_edge_false_positives final : public false_positive_regular_test
  {
  public:
    using false_positive_regular_test::false_positive_regular_test;    

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
