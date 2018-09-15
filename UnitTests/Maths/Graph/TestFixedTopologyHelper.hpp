#pragma once

#include "GraphTestingUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    template<class Checker>
    class undirected_fixed_topology_checker : public checker_wrapper<Checker>
    {
    public:
      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");

        check_2_4<Graph>();
      }
      
      template<class Graph>
      void check_2_4()
      {
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{make_2_4<Graph>()};
          check_2_4(g);
        }
        else
        {
          const Graph g{make_2_4<Graph>()};
          check_2_4(g);
        }
      }
    private:
      template<class Graph>
      constexpr static Graph make_2_4()
      {
        using edge = typename Graph::edge_init_type;
        // 10
        // /\
        // \/___5___
        //  x---1---x
        //   ---2---
        //
        Graph g{{edge{1,5}, edge{0,10}, edge{1,2}, edge{1,1}, edge{0, 10}}, {edge{0,2}, edge{0,5}, edge{0, 1}}};

        g.sort_edges(g.cbegin_edges(0), g.cend_edges(0), [](const auto& e1, const auto&e2){
            return e1.weight() < e2.weight();
          });

        g.set_edge_weight(g.cbegin_edges(0) + 3, 11);

        g.mutate_edge_weight(g.cbegin_edges(0) + 3, [](auto& w) { w -= 2; });

        using NodeWeight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<NodeWeight>)            
        {
          g.node_weight(g.cbegin_node_weights(), 2);
          g.mutate_node_weight(g.cbegin_node_weights() +1, [](auto& w) { w -= 3; });
        }
        
        return g;
      }

      template<class Graph>
      void check_2_4(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        if constexpr(std::is_empty_v<NodeWeight>)            
        {
          this->template check_graph(g, {{edge{1,1}, edge{1,2}, edge{1,5}, edge{0,9}, edge{0, 9}}, {edge{0, 1}, edge{0,2}, edge{0,5}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
        }
        else
        {
          this->template check_graph(g, {{edge{1,1}, edge{1,2}, edge{1,5}, edge{0,9}, edge{0, 9}}, {edge{0, 1}, edge{0,2}, edge{0,5}}}, {NodeWeight{2}, NodeWeight{-3}}, LINE(""));
        }
      }
    };

    template<class Checker>
    class directed_fixed_topology_checker : public checker_wrapper<Checker>
    {
    public:
      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");

        check_3_10<Graph>();
      }
      
      template<class Graph>
      void check_3_10()
      {
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{make_3_10<Graph>()};
          check_3_10(g);
        }
        else
        {
          const Graph g{make_3_10<Graph>()};
          check_3_10(g);
        }
      }
    private:
      template<class Graph>
      constexpr static Graph make_3_10()
      {
        using edge = typename Graph::edge_init_type;

        //  -2        6         -3  14
        // />\       />\       />\ />\
        // \ /       \ /       \ / \ /
        //    --1>---              
        //  X --0>--- X ---8>--- X
        //    --3>---   ---<7--- 
        //    --<9---

        Graph g{{edge{1,1}, edge{1,0}, edge{0, -2}, edge{1,3}}, {edge{2,8}, edge{1,6}, edge{0,9}}, {edge{2,-3}, edge{1,7}, edge{2,14}}};

        g.sort_edges(g.cbegin_edges(0)+1, g.cbegin_edges(0) + 3, [](const auto& e1, const auto& e2) {
            return e1.weight() > e2.weight();
          });

        g.sort_edges(g.cbegin_edges(1) + 1, g.cbegin_edges(2) + 2, [](const auto& e1, const auto& e2) {
            return e1.weight() > e2.weight();
          });

        g.set_edge_weight(g.cbegin_edges(2) + 2, 21);

        g.mutate_edge_weight(g.cbegin_edges(2) + 2, [](auto& w){ w *= 2; });

        using NodeWeight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<NodeWeight>)            
        {
          g.node_weight(g.cbegin_node_weights()+1, 2);
          g.mutate_node_weight(g.cbegin_node_weights() +2, [](auto& w) { w -= 3; });
        }

        return g;
      }

      template<class Graph>
      void check_3_10(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        if constexpr(std::is_empty_v<NodeWeight>)            
        {
          this->template check_graph(g,
            {
              {edge{1,1}, edge{1,0}, edge{0, -2}, edge{1,3}},
              {edge{2,8}, edge{0,9}, edge{1,6}},
              {edge{1, 7}, edge{2, -3}, edge{2, 42}}
            }, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        }
        else
        {
          this->template check_graph(g,
            {
              {edge{1,1}, edge{1,0}, edge{0, -2}, edge{1,3}},
              {edge{2,8}, edge{0,9}, edge{1,6}},
              {edge{1, 7}, edge{2, -3}, edge{2, 42}}
            }, {NodeWeight{}, NodeWeight{2}, NodeWeight{-3}}, LINE(""));
        }
      }
    };

    template<class Checker>
    class e_undirected_fixed_topology_checker : public checker_wrapper<Checker>
    {
    public:
      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");

        check_2_2<Graph>();
      }
      
      template<class Graph>
      void check_2_2()
      {
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{make_2_2<Graph>()};
          check_2_2(g);
        }
        else
        {
          const Graph g{make_2_2<Graph>()};
          check_2_2(g);
        }
      }
    private:
      template<class Graph>
      constexpr static Graph make_2_2()
      {
        using edge = typename Graph::edge_init_type;
        //     3  4
        //     /\/\
        //    / /\ \
        //    \/ / /
        //  x    x
        Graph g{{}, {edge{1,2,3}, edge{1,3,4}, edge{1,0,3}, edge{1,1,4}}};

        g.set_edge_weight(g.cbegin_edges(1) + 3, -4);
        g.mutate_edge_weight(g.cbegin_edges(1) + 3, [](auto& w) { w += 2; });

        using NodeWeight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<NodeWeight>)            
        {
          g.node_weight(g.cbegin_node_weights(), 2);
          g.mutate_node_weight(g.cbegin_node_weights() +1, [](auto& w) { w -= 3; });
        }

        return g;
      }

      template<class Graph>
      void check_2_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        if constexpr(std::is_empty_v<NodeWeight>)            
        {
          this->template check_graph(g, {{}, {edge{1,2,3}, edge{1,3,-2}, edge{1,0,3}, edge{1,1,-2}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
        }
        else
        {
          this->template check_graph(g, {{}, {edge{1,2,3}, edge{1,3,-2}, edge{1,0,3}, edge{1,1,-2}}}, {NodeWeight{2}, NodeWeight{-3}}, LINE(""));          
        }
      }
    };

    template<class Checker>
    class e_directed_fixed_topology_checker : public checker_wrapper<Checker>
    {
    public:
      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");

        check_2_2<Graph>();
      }
      
      template<class Graph>
      void check_2_2()
      {
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{make_2_2<Graph>()};
          check_2_2(g);
        }
        else
        {
          const Graph g{make_2_2<Graph>()};
          check_2_2(g);
        }
      }
    private:
      
      template<bool B> using inverted_constant = maths::inverted_constant<B>;
      
      template<class Graph>
      constexpr static Graph make_2_2()
      {
        using edge = typename Graph::edge_init_type;
        //     3  4
        //     /<\/>\
        //    /  /\  \
        //    \ / /  /
        //        x      x
        Graph g{{edge{0,inverted_constant<true>{},2,3}, edge{0,0,3,4}, edge{0,inverted_constant<true>{},0,3}, edge{0,0,1,4}}, {}};

        g.set_edge_weight(g.cbegin_edges(0) + 3, 8);
        g.mutate_edge_weight(g.cbegin_edges(0) + 3, [](auto& w) { w -= 2; });

        using NodeWeight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<NodeWeight>)            
        {
          g.node_weight(g.cbegin_node_weights(), 2);
          g.mutate_node_weight(g.cbegin_node_weights() + 1, [](auto& w) { w -= 3; });
        }

        return g;
      }

      template<class Graph>
      void check_2_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        if constexpr(std::is_empty_v<NodeWeight>)            
        {
          this->template check_graph(g, {{edge{0,inverted_constant<true>{},2,3}, edge{0,0,3,6}, edge{0,inverted_constant<true>{},0,3}, edge{0,0,1,6}}, {}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
        }
        else
        {
          this->template check_graph(g, {{edge{0,inverted_constant<true>{},2,3}, edge{0,0,3,6}, edge{0,inverted_constant<true>{},0,3}, edge{0,0,1,6}}, {}}, {NodeWeight{2}, NodeWeight{-3}}, LINE(""));
        }
      }
    };
  }
}
