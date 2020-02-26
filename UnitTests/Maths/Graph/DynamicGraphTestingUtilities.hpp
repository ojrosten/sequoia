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

#include <variant>

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
    class Logger,
    class... Extenders
  >
  class basic_graph_operations : protected graph_checker<Logger, Extenders...>
  {
  public:
    using graph_type = typename graph_type_generator<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>::graph_type;   

    using checker_type = graph_checker<Logger, Extenders...>;

    basic_graph_operations() = default;
    basic_graph_operations(const basic_graph_operations&) = delete;

    basic_graph_operations& operator=(const basic_graph_operations&) = delete;
    basic_graph_operations& operator=(basic_graph_operations&&)      = delete;
    
    log_summary run()
    {
      execute_operations();

      return this->summary("", log_summary::duration{});
    }

    log_summary recover_summary() const { return this->summary("", log_summary::duration{}); }
      
  protected:
    basic_graph_operations(basic_graph_operations&&) = default;
    ~basic_graph_operations() = default;
    
    virtual void execute_operations() = 0;
  };

  template
  <
    maths::graph_flavour GraphFlavour,      
    class EdgeWeight,
    class NodeWeight,      
    class EdgeWeightPooling,
    class NodeWeightPooling,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits,
    test_mode Mode
  >
  using regular_graph_operations
  = basic_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, unit_test_logger<Mode>, regular_extender<unit_test_logger<Mode>>>;

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
  using graph_operations = regular_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, test_mode::standard>;

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
  using false_positive_graph_operations = regular_graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits, test_mode::false_positive>;
  
  
  template <class EdgeWeight, class NodeWeight>
  class graph_test_helper
  {
  public:
    explicit graph_test_helper(concurrency_mode mode)
    {
      if(mode == concurrency_mode::deep)
      {
        m_Summary = std::vector<std::future<log_summary>>{};
      }
    }
      
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
    using summary = std::variant<log_summary, std::vector<std::future<log_summary>>>;
    summary m_Summary;

    template<class Test>
    class sentry
    {
    public:
      sentry(Test& unitTest, summary& s) : m_UnitTest{unitTest},  m_Summary{s} {}

      ~sentry()
      {
        std::visit(
            sequoia::variant_visitor{
              [&test{m_UnitTest}](log_summary& s){
                test.merge(s);   
                s.clear();
              },
              [&test{m_UnitTest}](std::vector<std::future<log_summary>>& s){
                try
                {
                  for(auto&& f : s)
                  {
                    test.merge(f.get());
                  }
                }
                catch(const std::exception& e)
                {
                  test.report_async_exception(combine_messages(test.name(), e.what(), "\n"));
                }
                catch(...)
                {
                  test.report_async_exception(combine_messages(test.name(), "Unknown exception", "\n"));
                }

                s.clear();
              }
            }
            , m_Summary);
      }
    private:
      Test& m_UnitTest;
      summary& m_Summary;
    };

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

      using testuu = TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled<EdgeWeight>, unpooled<NodeWeight>, EdgeStorageTraits, NodeStorageTraits>;
      using testud = TemplateTestClass<GraphType, EdgeWeight, NodeWeight, unpooled<EdgeWeight>, data_pool<NodeWeight>, EdgeStorageTraits, NodeStorageTraits>;  
      using testdu = TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool<EdgeWeight>, unpooled<NodeWeight>, EdgeStorageTraits, NodeStorageTraits>;
      using testdd = TemplateTestClass<GraphType, EdgeWeight, NodeWeight, data_pool<EdgeWeight>, data_pool<NodeWeight>, EdgeStorageTraits, NodeStorageTraits>;

      run_graph_test(testuu{});
 
      if constexpr(!std::is_empty_v<EdgeWeight> && !std::is_empty_v<NodeWeight>)
      {
        if constexpr(!minimal_graph_tests())
        {     
          run_graph_test(testud{});
          run_graph_test(testdu{});
          run_graph_test(testdd{});
        }
      }
      else if constexpr(!std::is_empty_v<EdgeWeight>)
      {
        run_graph_test(testdu{});
      }
      else if constexpr(!std::is_empty_v<NodeWeight>)
      {
        run_graph_test(testud{});
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
    void run_graph_test(Test&& test)
    {
      try
      {
        std::visit(
            variant_visitor{
              [test{std::move(test)}](log_summary& s) mutable { s += test.run(); },
              [test{std::move(test)}] (std::vector<std::future<log_summary>>& s) mutable {
                s.emplace_back(std::async([test{std::move(test)}]() mutable { return test.run(); }));
              }
            }
            , m_Summary);
      }
      catch(...)
      {
        if(std::holds_alternative<log_summary>(m_Summary))
        {
          auto& s{std::get<log_summary>(m_Summary)};
          s += test.recover_summary();
        }

        throw;
      }
    }
  };
}
