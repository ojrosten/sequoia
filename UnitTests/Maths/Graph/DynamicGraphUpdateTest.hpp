////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DynamicGraphTestingUtilities.hpp"

#include "DynamicGraphTraversals.hpp"
#include "ConcurrencyModels.hpp"

#include <functional>

namespace sequoia
{
  namespace unit_testing
  {
    class test_graph_update : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;
    private:
      void run_tests();
    };

    template<class G>
    std::tuple<std::size_t, std::size_t, std::size_t> nth_connection_indices(const G& graph, const std::size_t node, const std::size_t localEdgeIndex)
    {
      auto begin = graph.cbegin_edges(node);
      auto end   = graph.cend_edges(node);

      if(localEdgeIndex >= static_cast<std::size_t>(distance(begin, end)))
        throw std::out_of_range("graph_utilities::nth_connection_indices - localEdgeIndex out of range");

      auto& edge = *(begin + localEdgeIndex);
      const std::size_t targ = edge.target_node();
      std::size_t nthConnection{0};

      for(auto citer = begin; citer != begin + localEdgeIndex; ++citer)
      {
        auto& currentEdge = (*citer);
        if(targ == currentEdge.target_node()) ++nthConnection;
      }

      return std::make_tuple(node, targ, nthConnection);
    }

    //=======================================================================================//
    // Class which will be used to provide graph update functions

    
    template <class G>
    class GraphUpdaterFunctors
    {
    public:
      GraphUpdaterFunctors(G& graph) : m_Graph(graph) {}
      
      void firstNodeTraversal(const std::size_t index)
      {
        auto iter = m_Graph.cbegin_node_weights() + index;
        const auto newWeight = (2 + m_NodeTraversalIndex) * *iter;
        m_Graph.node_weight(iter, newWeight);

        ++m_NodeTraversalIndex;
      }

      void secondNodeTraversal(const std::size_t index)
      {
        --m_NodeTraversalIndex;
        auto iter = m_Graph.cbegin_node_weights() + index;
        const auto newWeight =  *iter / (5 - m_NodeTraversalIndex);
        m_Graph.node_weight(iter, newWeight);
      }

      template<class Iter>
      void firstEdgeTraversal(Iter citer)
      {
        const auto newWeight = 10 + m_EdgeTraversalIndex + citer->weight();
        m_Graph.set_edge_weight(citer, newWeight);

        ++m_EdgeTraversalIndex;
      }

      template<class Iter>
      void secondEdgeTraversal(Iter citer)
      {
        --m_EdgeTraversalIndex;
        const auto newWeight = citer->weight() - 14 + m_EdgeTraversalIndex;
        m_Graph.set_edge_weight(citer, newWeight);
      }
    private:
      G& m_Graph;
      
      std::size_t m_NodeTraversalIndex{},
                  m_EdgeTraversalIndex{};
    };

    //=======================================================================================//
    // Cyclic graph created and updated using all algorithms
    
    template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
      template <class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class test_update
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using graph_t =
        typename graph_operations
        <
          GraphFlavour,      
          EdgeWeight,
          NodeWeight,      
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_type;

      using ei_t = typename graph_t::edge_init_type;

      using flavour = maths::graph_flavour;
      using UndirectedType = std::bool_constant<maths::undirected(GraphFlavour)>;
      using MutualType = std::bool_constant<mutual_info(GraphFlavour)>;

      using checker<unit_test_logger<test_mode::standard>>::check_equality;

      void check_setup(const graph_t& graph);
      
      void execute_operations() override;
      /*{
        graph_t graph;

        graph.add_node(std::size_t{5});
        graph.add_node(std::size_t{2});
        graph.add_node(std::size_t{10});
        graph.add_node(std::size_t{4});

        graph.join(0, 1, std::size_t{1});
        graph.join(1, 3, std::size_t{3});
        graph.join(3, 2, std::size_t{4});
        graph.join(0, 2, std::size_t{7});
        graph.join(2, 0, std::size_t{2});

        //       7
        //(0)5=======10(2)
        //   |   2   |
        //  1|       |4
        //   |       |
        //(1)2-------4(3)
        //       3

        check_edge_weights<false>(graph, initial_reversed_edge_weights(), LINE(""));

        GraphUpdaterFunctors<graph_t> updater(graph);

        //=============================== DFS ===============================//
        auto firstNodeFn = [&updater](const std::size_t index){ updater.firstNodeTraversal(index); };
        maths::depth_first_search(graph, false, 0, firstNodeFn);

        check_equality<std::size_t>(10, *graph.cbegin_node_weights(), LINE("Node 0 traversed first, with weight set to (2+0)*5"));
        check_equality<std::size_t>(6,  *(graph.cbegin_node_weights() + 1), LINE("Node 1 traversed second, with weight set to (2+1)*2"));   
        check_equality<std::size_t>(16, *(graph.cbegin_node_weights() + 3), LINE("Node 3 traversed third, with weight set to (2+2)*4"));
        check_equality<std::size_t>(50, *(graph.cbegin_node_weights() + 2), LINE("Node 2 traversed fourth, with weight set to (2+3)*10"));

        auto secondNodeFn = [&updater](const std::size_t index){ updater.secondNodeTraversal(index); };
        maths::depth_first_search(graph, false, 0, maths::null_functor(), secondNodeFn);
        check_equality<std::size_t>(5, *graph.cbegin_node_weights(), LINE("Node 0 traversed first, weight reset to 5"));
        check_equality<std::size_t>(2, *(graph.cbegin_node_weights() + 1), LINE("Node 1 traversed second, weight reset to 2"));
        check_equality<std::size_t>(4, *(graph.cbegin_node_weights() + 3), LINE("Node 3 traversed third, weight reset to 4"));
        check_equality<std::size_t>(10,*(graph.cbegin_node_weights() + 2), LINE("Node 2 traversed fourth, weight reset to 10"));

        // auto will pick up criter for dfs and citer for other searches
        auto firstEdgeFn = [&updater](auto citer) { updater.firstEdgeTraversal(citer); };
        maths::depth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), firstEdgeFn);

        //  Undirected           Directed        0-1;1-3;3-2;0-2;2-0;
        //      18                  17
        //(0)5=======10(2)   (0)5=======10(2)
        //   |  12   |          |   16   |
        // 13|       |18      12|        |17
        //   |       |          |        |
        //(1)2-------4(3)    (1)2--------4(3)
        //      16                  15

        check_edge_weights<false>(graph, dfu_first_traversal_answers(), LINE(""));

        // auto will pick up criter for dfs and citer for other searches
        auto secondEdgeFn = [&updater](auto citer) { updater.secondEdgeTraversal(citer); };
        dfu_second_edge_traversal(UndirectedType(), graph, secondEdgeFn);

        //  Undirected           Directed        0-1;1-3;3-2;0-2;2-0;
        //       5                  7
        //(0)5=======10(2)   (0)5=======10(2)
        //   |   0   |          |   2    |
        //  3|       |4        1|        |4
        //   |       |          |        |
        //(1)2-------4(3)    (1)2--------4(3)
        //       5                  3
         
        check_edge_weights<false>(graph, dfu_second_traversal_answers(), LINE(""));

        //=============================== BFS ===============================//
        maths::breadth_first_search(graph, false, 0, firstNodeFn);

        check_equality<std::size_t>(10, *graph.cbegin_node_weights(), LINE("Node 0 traversed first, with weight set to (2+0)*5"));
        check_equality<std::size_t>(6,  *(graph.cbegin_node_weights() + 1), LINE("Node 1 traversed second, with weight set to (2+1)*2"));   
        check_equality<std::size_t>(40, *(graph.cbegin_node_weights() + 2), LINE("Node 2 traversed third, with weight set to (2+2)*10"));
        check_equality<std::size_t>(20, *(graph.cbegin_node_weights() + 3), LINE("Node 3 traversed fourth, with weight set to (2+3)*4"));

        maths::breadth_first_search(graph, false, 0, maths::null_functor(), secondNodeFn);

        check_equality<std::size_t>(5, *graph.cbegin_node_weights(), LINE("Node 0 traversed first, weight reset to 5"));
        check_equality<std::size_t>(2, *(graph.cbegin_node_weights() + 1), LINE("Node 1 traversed second, weight reset to 2"));   
        check_equality<std::size_t>(10,*(graph.cbegin_node_weights() + 2), LINE("Node 2 traversed third, weight reset to 10"));
        check_equality<std::size_t>(4, *(graph.cbegin_node_weights() + 3), LINE("Node 3 traversed fourth, weight reset to 4"));

        maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), firstEdgeFn);

        //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
        //      16                  18
        //(0)5=======10(2)   (0)5=======10(2)
        //   |  12   |          |   15   |
        // 13|       |18      11|        |18
        //   |       |          |        |
        //(1)2-------4(3)    (1)2--------4(3)
        //      18                  15
 
        check_edge_weights<true>(graph, bfu_first_traversal_answers(), LINE(""));

        bfu_second_edge_traversal(UndirectedType(), graph, secondEdgeFn);

        //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
        //       5                   7
        //(0)5=======10(2)   (0)5=======10(2)
        //   |   0   |          |    2   |
        //  3|       |4        1|        |4
        //   |       |          |        |
        //(1)2-------4(3)    (1)2--------4(3)
        //       5                  3

        check_edge_weights<true>(graph, bfu_second_traversal_answers(), LINE(""));
        
        //=============================== PRS ===============================//
        maths::priority_search(graph, false, 0, firstNodeFn);

        check_equality<std::size_t>(10, *graph.cbegin_node_weights(), LINE("Node 0 traversed first, with weight set to (2+0)*5"));        
        check_equality<std::size_t>(30, *(graph.cbegin_node_weights() + 2), LINE("Node 2 traversed second, with weight set to (2+1)*10"));
        undirected(GraphFlavour) ? check_equality<std::size_t>(16,  *(graph.cbegin_node_weights() + 3), LINE("Node 3 traversed third, with weight set to (2+2)*4"))
          : check_equality<std::size_t>(8,  *(graph.cbegin_node_weights() + 1), LINE("Node 1 traversed third, with weight set to (2+2)*2"));
        undirected(GraphFlavour) ? check_equality<std::size_t>(10, *(graph.cbegin_node_weights() + 1), LINE("Node 1 travsered fourth, with weight set to (2+3)*2"))
          : check_equality<std::size_t>(20, *(graph.cbegin_node_weights() + 3), LINE("Node 3 travsered fourth, with weight set to (2+3)*4"));

        //  Undirected            Directed        0-1;1-3;3-2;0-2;2-0;
        //       5                   7
        //(0)10=======30(2)  (0)10=======30(2)
        //    |   0   |          |    2   |
        //   3|       |4        1|        |4
        //    |       |          |        |
        //(1)10-------16(3)   (1)8--------20(3)
        //       5                  3

        maths::priority_search(graph, false, 0, maths::null_functor(), secondNodeFn);

        //    Undirected          Directed        0-1;1-3;3-2;0-2;2-0;
        //        5                   7
        //(0)10=======15(2)  (0)10========15(2)
        //    |   0   |          |    2   |
        //   3|       |4        1|        |4
        //    |       |          |        |
        //(1) 2-------4(3)   (1) 2--------4(3)
        //        5                   3

        check_equality<std::size_t>(5, *graph.cbegin_node_weights(), LINE("Node 0 traversed first, with weight set to 10*(5-3)"));        
        check_equality<std::size_t>(10, *(graph.cbegin_node_weights() + 2), LINE("Node 2 traversed second, with weight set to 30*(5-2)"));
        undirected(GraphFlavour) ? check_equality<std::size_t>(4, *(graph.cbegin_node_weights() + 3), LINE("Node 3 traversed third, with weight set to 16/(5-1)"))
          : check_equality<std::size_t>(2, *(graph.cbegin_node_weights() + 1), LINE("Node 1 traversed third, with weight set to 8/(5-1)"));
        undirected(GraphFlavour) ? check_equality<std::size_t>(2, *(graph.cbegin_node_weights() + 1), LINE("Node 1 travsered fourth, with weight set to 10/5"))
          : check_equality<std::size_t>(4, *(graph.cbegin_node_weights() + 3), LINE("Node 3 travsered fourth, with weight set to 20/5"));


        maths::priority_search(graph, false, 0, maths::null_functor(), maths::null_functor(), firstEdgeFn);

        //    Undirected          Directed        0-1;1-3;3-2;0-2;2-0;
        //       16                  18
        //(0)10=======15(2)  (0)10========15(2)
        //    |  12   |          |   14   |
        //  13|       |17      11|        |18
        //    |       |          |        |
        //(1) 2-------4(3)   (1) 2--------4(3)
        //        19                 16

        check_edge_weights<true>(graph, psu_first_traversal_answers(), LINE(""));

        ps_second_edge_traversal(UndirectedType(), graph, secondEdgeFn);

        //    Undirected          Directed        0-1;1-3;3-2;0-2;2-0;
        //        6                  7
        //(0)10=======15(2)  (0)10========15(2)
        //    |   1   |          |   2    |
        //   0|       |5        1|        |4
        //    |       |          |        |
        //(1) 2-------4(3)   (1) 2--------4(3)
        //        5                  3
        
        check_edge_weights<true>(graph, psu_second_traversal_answers(), LINE(""));
      }
      */

      template<class UpdateFunctor>
      void dfu_second_edge_traversal(std::true_type, graph_t& graph, UpdateFunctor fn)
      {
        maths::depth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), maths::null_functor(), fn);
      }

      template<class UpdateFunctor>
      void dfu_second_edge_traversal(std::false_type, graph_t& graph, UpdateFunctor fn)
      {
        maths::depth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), fn);
      }

      template<class UpdateFunctor>
      void bfu_second_edge_traversal(std::true_type, graph_t& graph, UpdateFunctor fn)
      {
        maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), maths::null_functor(), fn);
      }

      template<class UpdateFunctor>
      void bfu_second_edge_traversal(std::false_type, graph_t& graph, UpdateFunctor fn)
      {
        maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), fn);
      }
      
      std::vector<std::vector<std::size_t>> dfu_first_traversal_answers()
      {
        if constexpr (maths::undirected(GraphFlavour))
        {
          return {
            std::vector<std::size_t>{12, 18, 13},
            std::vector<std::size_t>{16, 13},
            std::vector<std::size_t>{12, 18, 18},
            std::vector<std::size_t>{18, 16}
          };
        }
        else if constexpr (GraphFlavour == flavour::directed_embedded)
        {
          return {
            std::vector<std::size_t>{16, 17, 12},
            std::vector<std::size_t>{15, 12},
            std::vector<std::size_t>{16, 17, 17},
            std::vector<std::size_t>{17, 15}
          };
        }
        else
        {
          return {
            std::vector<std::size_t>{17, 12},
            std::vector<std::size_t>{15},
            std::vector<std::size_t>{16},
            std::vector<std::size_t>{17}
          };
        }
      }

      std::vector<std::vector<std::size_t>> dfu_second_traversal_answers()
      {
        if constexpr (maths::undirected(GraphFlavour))
        {
         return {
           std::vector<std::size_t>{0, 5, 3},
           std::vector<std::size_t>{5, 3},
           std::vector<std::size_t>{0, 5, 4},
           std::vector<std::size_t>{4, 5}
         };
        }
        else if constexpr (GraphFlavour == flavour::directed_embedded)
        {
          return {
            std::vector<std::size_t>{2, 7, 1},
            std::vector<std::size_t>{3, 1},
            std::vector<std::size_t>{2, 7, 4},
            std::vector<std::size_t>{4, 3}
          };
        }
        else
        {
          return {
            std::vector<std::size_t>{7, 1},
            std::vector<std::size_t>{3},
            std::vector<std::size_t>{2},
            std::vector<std::size_t>{4}
          };
        }
      }

      std::vector<std::vector<std::size_t>> bfu_first_traversal_answers()
      {
        if constexpr (maths::undirected(GraphFlavour))
        {
          return {
            std::vector<std::size_t>{13, 16, 12},
            std::vector<std::size_t>{13, 18},
            std::vector<std::size_t>{18, 16, 12},
            std::vector<std::size_t>{18, 18}};
        }
        else if constexpr (GraphFlavour == flavour::directed_embedded)
        {
          return {
            std::vector<std::size_t>{11, 18, 15},
            std::vector<std::size_t>{11, 15},
            std::vector<std::size_t>{18, 18, 15},
            std::vector<std::size_t>{15, 18}
          };
        }
        else
        {
          return {
            std::vector<std::size_t>{11, 18},
            std::vector<std::size_t>{15},
            std::vector<std::size_t>{15},
            std::vector<std::size_t>{18}
          };
        }
      }

      std::vector<std::vector<std::size_t>> bfu_second_traversal_answers()
      {
        if constexpr (maths::undirected(GraphFlavour))
        {
          return{
            std::vector<std::size_t>{3, 5, 0},
            std::vector<std::size_t>{3, 5},
            std::vector<std::size_t>{4, 5, 0},
            std::vector<std::size_t>{5, 4}
          };
        }
        else if constexpr (GraphFlavour == flavour::directed_embedded)
        {
          return {
            std::vector<std::size_t>{1, 7, 2},
            std::vector<std::size_t>{1, 3},
            std::vector<std::size_t>{4, 7, 2},
            std::vector<std::size_t>{3, 4}
          };
        }
        else
        {
          return {
            std::vector<std::size_t>{1, 7},
            std::vector<std::size_t>{3},
            std::vector<std::size_t>{2},
            std::vector<std::size_t>{4}
          };
        }    
      }

      std::vector<std::vector<std::size_t>> psu_first_traversal_answers()
      {
        if constexpr (maths::undirected(GraphFlavour))
        {
          return {
            std::vector<std::size_t>{13, 16, 12},
            std::vector<std::size_t>{13, 19},
            std::vector<std::size_t>{17, 16, 12},
            std::vector<std::size_t>{19, 17}
          };
        }
        else if constexpr (GraphFlavour == flavour::directed_embedded)
        {
          return {
            std::vector<std::size_t>{11, 18, 14},
            std::vector<std::size_t>{11, 16},
            std::vector<std::size_t>{18, 18, 14},
            std::vector<std::size_t>{16, 18}
          };
        }
        else
        {
          return {
            std::vector<std::size_t>{11, 18},
            std::vector<std::size_t>{16},
            std::vector<std::size_t>{14},
            std::vector<std::size_t>{18}
          };
        }        
      }

      template<class UpdateFunctor>
      void ps_second_edge_traversal(std::true_type, graph_t& graph, UpdateFunctor fn)
      {
        maths::priority_search(graph, false, 0, maths::null_functor(), maths::null_functor(), maths::null_functor(), fn);
      }

      template<class UpdateFunctor>
      void ps_second_edge_traversal(std::false_type, graph_t& graph, UpdateFunctor fn)
      {
        maths::priority_search(graph, false, 0, maths::null_functor(), maths::null_functor(), fn);
      }

      std::vector<std::vector<std::size_t>> psu_second_traversal_answers()
      {
        if constexpr (maths::undirected(GraphFlavour))
        {
          return {
            std::vector<std::size_t>{0, 6, 1},
            std::vector<std::size_t>{0, 5},
            std::vector<std::size_t>{5, 6, 1},
            std::vector<std::size_t>{5, 5}};
        }
        else if constexpr (GraphFlavour == flavour::directed_embedded)
        {
          return {
            std::vector<std::size_t>{1, 7, 2},
            std::vector<std::size_t>{1, 3},
            std::vector<std::size_t>{4, 7, 2},
            std::vector<std::size_t>{3, 4}
          };
        }
        else
        {
          return {
            std::vector<std::size_t>{1, 7},
            std::vector<std::size_t>{3},
            std::vector<std::size_t>{2},
            std::vector<std::size_t>{4}
          };
        }
      }
      
      template<bool Forward>
      void check_edge_weights(const graph_t& graph, const std::vector<std::vector<std::size_t>>& answers, const std::string& failureMessage = "")
      {
          for(auto cans_iter=answers.cbegin(); cans_iter != answers.cend(); ++cans_iter)
          {
              const std::size_t nodeIndex{static_cast<std::size_t>(std::distance(answers.cbegin(), cans_iter))};
              auto credge_begin = maths::graph_impl::iterator_getter<Forward>::begin(graph, nodeIndex);
              auto credge_end = maths::graph_impl::iterator_getter<Forward>::end(graph, nodeIndex);
              const std::size_t num_edges{static_cast<std::size_t>(distance(credge_begin, credge_end))};

              std::stringstream message;
              message << "Node " << nodeIndex << " has " << cans_iter->size() << " edges";
              if(!failureMessage.empty()) message << " " << failureMessage;
              this->template check_equality<std::size_t>(cans_iter->size(), num_edges, message.str());
              if(cans_iter->size() == num_edges)
              {
                  for(auto credge_iter = credge_begin; credge_iter != credge_end; ++credge_iter)
                  {
                      auto edge_index = distance(credge_begin, credge_iter);
                      std::stringstream message;
                      auto val = (*cans_iter)[edge_index];
                      const std::string dir{Forward ? " forward" : " reversed"};
                      message << "Node " << nodeIndex << dir << " edge " << edge_index << " has weight " << val;
                      if(!failureMessage.empty()) message << " " << failureMessage;
                      this->template check_equality<std::size_t>(val, credge_iter->weight(), message.str());
                  }
              }
          }
      }
    };
    
    //=======================================================================================//
    // Graph (tree) created and searched; at each vertex,
    // and edge, an update of the weights is performed

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class NodeWeight,      
      template <class> class EdgeWeightPooling,
      template <class> class NodeWeightPooling,
      template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
      template <class, template<class> class, bool> class NodeWeightStorageTraits
    >
    class test_bf_update
      : public graph_operations<GraphFlavour, EdgeWeight, NodeWeight, EdgeWeightPooling, NodeWeightPooling, EdgeStorageTraits, NodeWeightStorageTraits>
    {
    private:
      using graph_t =
        typename graph_operations
        <
          GraphFlavour,      
          EdgeWeight,
          NodeWeight,      
          EdgeWeightPooling,
          NodeWeightPooling,
          EdgeStorageTraits,
          NodeWeightStorageTraits
        >::graph_type;

      using ei_t = typename graph_t::edge_init_type;
      using ew_t = std::vector<double>;
      
      using checker<unit_test_logger<test_mode::standard>>::check_equality;
      using checker<unit_test_logger<test_mode::standard>>::check_exception_thrown;
      using checker<unit_test_logger<test_mode::standard>>::check_range;
      
      void execute_operations() override;

      void test_second_edge_traversal_update(graph_t& graph);
    };
  }
}
