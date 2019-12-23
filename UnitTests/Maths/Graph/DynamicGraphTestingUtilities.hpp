////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestPreprocessor.hpp"

#include "GraphTestingUtilities.hpp"
#include "NodeStorageTestingUtilities.hpp"
#include "PartitionedDataTestingUtilities.hpp"

#include "DynamicGraph.hpp"

namespace sequoia::unit_testing
{
  // Details Checkers
  
  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct detailed_equality_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_detailed_equality_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct detailed_equality_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_detailed_equality_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  // Equivalence Checkers
  
  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct equivalence_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_equivalence_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  // Weak Equivalence

  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct weak_equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
    : impl::graph_weak_equivalence_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {};

  // Edge Storage Traits
  
  struct independent_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct independent_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  struct shared_weight_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  struct shared_weight_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  struct custom_allocator_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::partitioned_sequence<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = custom_partitioned_sequence_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
  };

  struct custom_allocator_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = custom_bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
  };

  template<class NodeWeight, bool=std::is_empty_v<NodeWeight>>
  struct custom_node_weight_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    constexpr static bool has_allocator{true};
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, true, true, true>>;
  };

  template<class NodeWeight>
  struct custom_node_weight_storage_traits<NodeWeight, true>
  {
    constexpr static bool has_allocator{};
  };

  // Meta

  [[nodiscard]]
  constexpr bool embedded(const maths::graph_flavour graphFlavour) noexcept
  {
    using gf = maths::graph_flavour;
    return (graphFlavour == gf::directed_embedded) || (graphFlavour == gf::undirected_embedded);
  }

  template
  <
    maths::graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightStorage,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    bool=embedded(GraphFlavour)
  >
  struct graph_type_generator
  {
    using graph_type = maths::graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightStorage, EdgeStorageTraits, NodeWeightStorageTraits>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,     
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, true>
  {
    using graph_type = maths::embedded_graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
  };

  // Unit test utilities
  
  template
  <
    maths::graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    class Logger=unit_test_logger<test_mode::standard>
  >
  class graph_operations : protected graph_checker<Logger>
  {
  public:
    using graph_type = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;   
    
    using graph_checker<Logger>::check_equality;
    using graph_checker<Logger>::check_regular_semantics;
    
    log_summary run()
    {
      execute_operations();

      return this->summary("");
    }

    log_summary get_summary() const { return this->summary(""); }
      
  protected:
    virtual void execute_operations() = 0;
  };

  
  template <class EdgeWeight, class NodeWeight>
  class graph_test_helper
  {
  public:
    graph_test_helper() = default;
      
    template
    <
      template
      <
        maths::graph_flavour,
        class...
      >
      class TemplateTestClass,
      class Test
    >
    void run_tests(Test& unitTest)
    {        
      using flavour = maths::graph_flavour;
      sentry<Test> s{unitTest, m_Summary};
      
      run_graph_flavour_tests<flavour::undirected, TemplateTestClass>();
      if constexpr(!minimal_graph_tests())
      {
        run_graph_flavour_tests<flavour::undirected_embedded, TemplateTestClass>();
        run_graph_flavour_tests<flavour::directed, TemplateTestClass>();
        run_graph_flavour_tests<flavour::directed_embedded, TemplateTestClass>();

        graph_storage_tests<flavour::directed_embedded, independent_contiguous_edge_storage_traits, maths::node_weight_storage_traits<NodeWeight>, TemplateTestClass>();
        graph_storage_tests<flavour::directed_embedded, independent_bucketed_edge_storage_traits, maths::node_weight_storage_traits<NodeWeight>, TemplateTestClass>();
      }
    }
      
    template
    <
      class EdgeStorageTraits,
      template <class, bool> class NodeStorageTraits,
      template
      <
        maths::graph_flavour,
        class...
      >
      class TemplateTestClass,
      class Test
    >
    void run_storage_tests(Test& unitTest)
    {        
      using flavour = maths::graph_flavour;
      sentry<Test> s{unitTest, m_Summary};

      using NodeStorage = NodeStorageTraits<NodeWeight, std::is_empty_v<NodeWeight>>;
      
      graph_storage_tests<flavour::undirected, EdgeStorageTraits, NodeStorage, TemplateTestClass>();
      if constexpr (!minimal_graph_tests())
      {
        graph_storage_tests<flavour::undirected_embedded, EdgeStorageTraits, NodeStorage, TemplateTestClass>();
        graph_storage_tests<flavour::directed,            EdgeStorageTraits, NodeStorage, TemplateTestClass>();
        graph_storage_tests<flavour::directed_embedded,   EdgeStorageTraits, NodeStorage, TemplateTestClass>();
      }
    }
      
    template
    <
      maths::graph_flavour GraphFlavour,
      template
      <
        maths::graph_flavour,
        class...
      >
      class TemplateTestClass,
      class Test
    >
    void run_individual_test(Test& unitTest)
    {
      sentry<Test> s{unitTest, m_Summary};
      run_graph_flavour_tests<GraphFlavour, TemplateTestClass>();
    }
      
  private:
    log_summary m_Summary{};

    template<class Test>
    class sentry
    {
    public:
      sentry(Test& unitTest, log_summary& summary) : m_UnitTest{unitTest},  m_Summary{summary} {}
      ~sentry()
      {
        m_UnitTest.merge(m_Summary);   
        m_Summary.clear();
      }
    private:
      Test& m_UnitTest;
      log_summary& m_Summary;
    };

    template<class Test>
    void finish(Test& unitTest)
    {
      unitTest.merge(m_Summary);   
      m_Summary.clear();
    }

    template
    <
      maths::graph_flavour GraphType,
      class EdgeStorageTraits,
      class NodeStorageTraits,
      template
      <
        maths::graph_flavour,
        class...
      >
      class TemplateTestClass
    >
    void graph_storage_tests()
    {
      using namespace data_sharing;

      TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled<EdgeWeight>, unpooled<NodeWeight>, EdgeStorageTraits, NodeStorageTraits> test;
      run_graph_test(test);
 
      if constexpr(!std::is_empty_v<EdgeWeight> && !std::is_empty_v<NodeWeight>)
      {
        if constexpr(!minimal_graph_tests())
        {
          TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled<EdgeWeight>, data_pool<NodeWeight>, EdgeStorageTraits, NodeStorageTraits> test1;  
          TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool<EdgeWeight>, unpooled<NodeWeight>, EdgeStorageTraits, NodeStorageTraits> test2;
          TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool<EdgeWeight>, data_pool<NodeWeight>, EdgeStorageTraits, NodeStorageTraits> test3;

          run_graph_test(test1);
          run_graph_test(test2);
          run_graph_test(test3);
        }
      }
      else if constexpr(!std::is_empty_v<EdgeWeight>)
      {
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool<EdgeWeight>, unpooled<NodeWeight>, EdgeStorageTraits, NodeStorageTraits> test;
        run_graph_test(test);
      }
      else if constexpr(!std::is_empty_v<NodeWeight>)
      {
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled<EdgeWeight>, data_pool<NodeWeight>, EdgeStorageTraits, NodeStorageTraits> test;
        run_graph_test(test);
      }
    }
      
    template
    <
      maths::graph_flavour GraphType,
      template
      <
        maths::graph_flavour,
        class...
      >
      class TemplateTestClass
    >
    void run_graph_flavour_tests()
    {
      using namespace data_structures;
        
      graph_storage_tests<GraphType, maths::contiguous_edge_storage_traits, maths::node_weight_storage_traits<NodeWeight>, TemplateTestClass>();
      if constexpr (!minimal_graph_tests())
      {
        graph_storage_tests<GraphType, maths::bucketed_edge_storage_traits, maths::node_weight_storage_traits<NodeWeight>, TemplateTestClass>();
      }
    }

    template<class Test>
    void run_graph_test(Test& test)
    {
      try
      {
        m_Summary += test.run();
      }
      catch(...)
      {
        m_Summary += test.get_summary();
        throw;
      }
    }
  };
}
