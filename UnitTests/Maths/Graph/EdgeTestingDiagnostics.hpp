#pragma once

#include "EdgeTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  class test_edge_false_positives : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;
    
    void run_tests() override;
    
  private:
    void test_partial_edge();
    void test_embedded_partial_edge();
    void test_edge();
    void test_embedded_edge();
  };

  class test_edge_false_negatives : public false_negative_test
  {
  public:
    using false_negative_test::false_negative_test;
    
    void run_tests() override;

  private:
    void test_partial_edge();
    void test_embedded_partial_edge();
    void test_edge();
    void test_embedded_edge();
  };

}
