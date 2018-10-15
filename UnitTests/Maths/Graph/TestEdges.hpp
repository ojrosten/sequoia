#pragma once

#include "UnitTestUtils.hpp"

#include "TestEdgeHelper.hpp"

#include "Utilities.hpp"

#include <initializer_list>

namespace sequoia
{
  namespace unit_testing
  {
    class test_edges : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      void run_tests();

      void test_partial_edge();      
      void test_plain_edge();
      void test_weighted_edge();
      void test_weighted_edge_lVal(const double val);
      void test_embedded_partial_edge();
      void test_embedded_edge();
      
      
      void test_edge_conversions();

      template<class E, class=std::enable_if_t<!std::is_empty_v<typename E::weight_type>>>
      bool check_edge(const std::size_t target, const typename E::weight_type& weight, const E& edge, const std::string& message="")
      {
        bool passed{true};
        if(!check_equality<std::size_t>(target, edge.target_node(), "Target node incorrect " + message)) passed = false;
        if(!check_equality(weight, edge.weight(), "Weight incorrect " + message))                        passed = false;        

        return passed;
      }

      template<class E, class=std::enable_if_t<std::is_empty_v<typename E::weight_type>>>
      bool check_edge(const std::size_t target, const E& edge, const std::string& message="")
      {
        return check_equality<std::size_t>(target, edge.target_node(), "Target node incorrect " + message);
      }

      template<class E, class=std::enable_if_t<!std::is_empty_v<typename E::weight_type>>>
      bool check_edge(const std::size_t host, const std::size_t target, const bool inverted, const typename E::weight_type& weight, const E& edge, const std::string& message="")
      {
        bool passed{check_edge(target, weight, edge, message)};
        if(!check_equality(host, edge.host_node(), "Host node incorrect " + message)) passed = false;
        if(check(edge.inverted() == inverted, "Inverted flag incorrect " + message)) passed = false;
        
        return passed;
      }

      template<class E, class=std::enable_if_t<std::is_empty_v<typename E::weight_type>>>
      bool check_edge(const std::size_t host, const std::size_t target, const bool inverted, const E& edge, const std::string& message="")
      {
        bool passed{check_edge(target, edge, message)};
        if(!check_equality(host, edge.host_node(), "Host node incorrect " + message)) passed = false;
        if(check(edge.inverted() == inverted, "Inverted flag incorrect " + message)) passed = false;
        
        return passed;
      }

      template<class E, class=std::enable_if_t<!std::is_empty_v<typename E::weight_type>>>
      bool check_embedded_edge(const std::size_t target, const std::size_t targetPosIndex, const typename E::weight_type& weight, const E& edge, const std::string& message="")
      {
        bool passed{check_edge(target, weight, edge, message)};
        if(!check_equality(targetPosIndex, edge.complementary_index(), "Complementary index incorrect " + message)) passed = false;
        
        return passed;
      }

      template<class E, class=std::enable_if_t<std::is_empty_v<typename E::weight_type>>>
      bool check_embedded_edge(const std::size_t target, const std::size_t targetPosIndex, const E& edge, const std::string& message="")
      {
        bool passed{check_edge(target, edge, message)};
        if(!check_equality(targetPosIndex, edge.complementary_index(), "Complementary index incorrect " + message)) passed = false;
        
        return passed;
      }

      template<class E, class=std::enable_if_t<!std::is_empty_v<typename E::weight_type>>>
      bool check_embedded_edge(const std::size_t host, const std::size_t target, const std::size_t targetPosIndex, const typename E::weight_type& weight, const E& edge, const std::string& message="")
      {
        bool passed{check_embedded_edge(target, targetPosIndex, weight, edge, message)};
        if(!check_equality(host, edge.host_node(), "Host node incorrect " + message)) passed = false;
        
        return passed;
      }

      template<class E, class=std::enable_if_t<std::is_empty_v<typename E::weight_type>>>
      bool check_embedded_edge(const std::size_t host, const std::size_t target, const std::size_t targetPosIndex, const E& edge, const std::string& message="")
      {
        bool passed{check_embedded_edge(target, targetPosIndex, edge, message)};
        if(!check_equality(host, edge.host_node(), "Host node incorrect " + message)) passed = false;
        
        return passed;
      }
    };
  }
}
