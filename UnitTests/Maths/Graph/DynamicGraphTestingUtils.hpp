#pragma once

#include "GraphTestingUtils.hpp"
#include "DynamicGraph.hpp"

namespace sequoia::unit_testing
{ 
  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template<maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits
  >
  struct details_checker<maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {
    using type = maths::graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;

    template<class Logger>
    static void check(Logger& logger, const type& graph, const type& prediction, std::string_view description)
    {
      using connectivity_t = typename type::connectivity_type;

      check_equality(logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction), description);

      // TO DO: nodes
    }    
  };

  template
  <
    maths::directed_flavour Directedness,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template<maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits
  >
  struct details_checker<maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>>
  {
    using type = maths::embedded_graph<Directedness, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;

    template<class Logger>
    static void check(Logger& logger, const type& graph, const type& prediction, std::string_view description)
    {
      using connectivity_t = typename type::connectivity_type;

      check_equality(logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction), description);

      // TO DO: nodes
    }    
  };
  

  
  
  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct independent_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::contiguous_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::contiguous_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct independent_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
  };

  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct shared_weight_contiguous_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::contiguous_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::contiguous_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };

  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template <class> class EdgeWeightPooling>
  struct shared_weight_bucketed_edge_storage_traits
  {
    template <class T, class Sharing, class Traits> using storage_type = data_structures::bucketed_storage<T, Sharing, Traits>;
    template <class T, class Sharing> using traits_type = data_structures::bucketed_storage_traits<T, Sharing>;

    constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
  };  

  template
  <
    maths::graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightStorage,
    template<maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits,
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
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template<maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits
  >
  struct graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, true>
  {
    using graph_type = maths::embedded_graph<maths::to_directedness(GraphFlavour), EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>;
  };

  template
  <
    maths::graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template<maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template<class, template<class> class, bool> class NodeWeightStorageTraits,
    class Logger=unit_test_logger<test_mode::standard>
  >
  class graph_operations : protected graph_checker<Logger>
  {
  public:
    using graph_type = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;      
      
    log_summary run(const std::string& helpername)
    {        
      this->failure_message_prefix(demangle<graph_type>());

      execute_operations();

      return this->summary("");
    }

    log_summary get_summary() const { return this->summary(""); }
    
    const std::string& prefix() const noexcept { return this->failure_message_prefix(); }
      
  protected:
    virtual void execute_operations() = 0;
  };

  
  template <class EdgeWeight, class NodeWeight>
  class graph_test_helper
  {
  public:
    graph_test_helper(std::string_view name="") : m_Name{name} {}
      
    template
    <
      template
      <
        maths::graph_flavour,
        class,
        class,
        template <class> class,
        template <class> class,
        template <maths::graph_flavour, class, template<class> class> class,
        template <class, template<class> class, bool> class
      >
      class TemplateTestClass,
      class Test
    >
    void run_tests(Test& unitTest)
    {        
      using flavour = maths::graph_flavour;
      sentry<Test> s{unitTest, m_Summary};
      
      run_graph_flavour_tests<flavour::undirected, TemplateTestClass>();
      run_graph_flavour_tests<flavour::undirected_embedded, TemplateTestClass>();
      run_graph_flavour_tests<flavour::directed, TemplateTestClass>();
      run_graph_flavour_tests<flavour::directed_embedded, TemplateTestClass>();

      graph_storage_tests_impl<flavour::directed_embedded, independent_contiguous_edge_storage_traits, TemplateTestClass>();
      graph_storage_tests_impl<flavour::directed_embedded, independent_bucketed_edge_storage_traits, TemplateTestClass>();
    }
      
    template
    <
      template <maths::graph_flavour, class, template<class> class> class EdgeStorage,
      template
      <
        maths::graph_flavour,
        class,
        class,
        template <class> class,
        template <class> class,
        template <maths::graph_flavour, class, template<class> class> class,
        template <class, template<class> class, bool> class
      >
      class TemplateTestClass,
      class Test
    >
    void run_storage_tests(Test& unitTest)
    {        
      using flavour = maths::graph_flavour;
      sentry<Test> s{unitTest, m_Summary};
      
      graph_storage_tests_impl<flavour::undirected,          EdgeStorage, TemplateTestClass>();
      graph_storage_tests_impl<flavour::undirected_embedded, EdgeStorage, TemplateTestClass>();
      graph_storage_tests_impl<flavour::directed,            EdgeStorage, TemplateTestClass>();
      graph_storage_tests_impl<flavour::directed_embedded,   EdgeStorage, TemplateTestClass>();          
    }
      
    template
    <
      maths::graph_flavour GraphFlavour,
      template
      <
        maths::graph_flavour,
        class,
        class,
        template <class> class,
        template <class> class,
        template <maths::graph_flavour, class, template<class> class> class,
        template <class, template<class> class, bool> class
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
    std::string m_Name;
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
      template <maths::graph_flavour, class, template<class> class> class EdgeStorage,
      template
      <
        maths::graph_flavour,
        class,
        class,
        template <class> class,
        template <class> class,
        template <maths::graph_flavour, class, template<class> class> class,
        template <class, template<class> class, bool> class
      >
      class TemplateTestClass
    >
    void graph_storage_tests_impl()
    {
      using namespace data_sharing;

      {
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled, unpooled, EdgeStorage, maths::node_weight_storage_traits> test;
        run_graph_test(test);
      }
 
      if constexpr(!std::is_empty_v<EdgeWeight> && !std::is_empty_v<NodeWeight>)
      {
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled, data_pool, EdgeStorage, maths::node_weight_storage_traits> test1;
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool, unpooled, EdgeStorage, maths::node_weight_storage_traits> test2;
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool, data_pool, EdgeStorage, maths::node_weight_storage_traits> test3;
      
        run_graph_test(test1);
        run_graph_test(test2);
        run_graph_test(test3);
      }
      else if constexpr(!std::is_empty_v<EdgeWeight>)
      {
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool, unpooled, EdgeStorage, maths::node_weight_storage_traits> test;
        run_graph_test(test);
      }
      else if constexpr(!std::is_empty_v<NodeWeight>)
      {
        TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled, data_pool, EdgeStorage, maths::node_weight_storage_traits> test;
        run_graph_test(test);
      }
    }
      
    template
    <
      maths::graph_flavour GraphType,
      template
      <
        maths::graph_flavour,
        class,
        class,
        template <class> class,
        template <class> class,
        template <maths::graph_flavour, class, template<class> class> class,
        template <class, template<class> class, bool> class
      >
      class TemplateTestClass
    >
    void run_graph_flavour_tests()
    {
      using namespace data_structures;
        
      graph_storage_tests_impl<GraphType, maths::contiguous_edge_storage_traits, TemplateTestClass>();
      graph_storage_tests_impl<GraphType, maths::bucketed_edge_storage_traits, TemplateTestClass>();
    }

    template<class Test>
    void run_graph_test(Test& test)
    {
      try
      {
        m_Summary += test.run(m_Name);
      }
      catch(...)
      {
        m_Summary += test.get_summary();
        m_Summary.name(test.prefix());
        throw;
      }
    }
  };
}
