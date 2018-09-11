#pragma once

#include "UnitTestUtils.hpp"

#include "NodeStorage.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class test_node_storage : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      using unit_test::check_equality;
      
      template<class WeightProxy>
      class node_storage_tester : public maths::graph_impl::node_storage<WeightProxy>
      {
      public:
        using maths::graph_impl::node_storage<WeightProxy>::node_storage;
        using maths::graph_impl::node_storage<WeightProxy>::reserve;
        using maths::graph_impl::node_storage<WeightProxy>::capacity;
        using maths::graph_impl::node_storage<WeightProxy>::shrink_to_fit;
        using maths::graph_impl::node_storage<WeightProxy>::add_node;
        using maths::graph_impl::node_storage<WeightProxy>::insert_node;
        using maths::graph_impl::node_storage<WeightProxy>::erase_node;
        using maths::graph_impl::node_storage<WeightProxy>::erase_nodes;
        using maths::graph_impl::node_storage<WeightProxy>::clear;
      };

      template<class WeightProxy, std::size_t N>
      class static_node_storage_tester : public maths::graph_impl::static_node_storage<WeightProxy, N>
      {
      public:
        using maths::graph_impl::static_node_storage<WeightProxy, N>::static_node_storage;
      };

      template<class Storage>
      bool check_storage(const Storage& storage, const std::vector<typename Storage::weight_type>& expected, std::string message)
      {
        bool passed{true};
        if(!message.empty()) message.insert(0, " "); 
        if(check_equality(expected.size(), storage.size(), "Storage of wrong size" + message))
        {
          auto citer = storage.cbegin_node_weights();
          auto ceiter = expected.cbegin();
          auto criter = storage.crend_node_weights();
          for(; citer != storage.cend_node_weights(); ++citer, ++ceiter)
          {
            --criter;
            if(!check_equality(*citer, *criter, "Disagreement between forward and reverse iterators" + message))
              passed = false;

            if(!check_equality(*ceiter, *citer, "Weight incorrect at position " + std::to_string(std::distance(expected.cbegin(), ceiter)) + message))
              passed = false;
          }
        }
        else
        {
          passed = false;
        }

        if(!check_equality(distance(storage.cbegin_node_weights(), storage.cend_node_weights()), distance(storage.crbegin_node_weights(), storage.crend_node_weights()), "Disagreement between forward and reverse iterator distances" + message))
        {
          passed = false;
        }

        return passed;
      }
      
      void run_tests();

      struct null_weight{};

      void test_dynamic_node_storage();
      void test_static_node_storage();
    };
  }
}
