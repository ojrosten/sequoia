////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtils.hpp"
#include "StaticGraphTestingUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    template<class Checker>
    class init_checker
    {
    public:
      init_checker(Checker& checker) : m_Checker{checker} {}

    protected:
      Checker& m_Checker;      
      
      ~init_checker() = default;

      template<
        class Graph,
        class Edge=typename Graph::edge_init_type,
        class NodeWeight=typename Graph::node_weight_type
      >
      void check_graph(const Graph& g, std::initializer_list<std::initializer_list<Edge>> connPrediction, std::initializer_list<NodeWeight> nodePrediction, std::string_view description)
      {
        if constexpr(std::is_empty_v<NodeWeight>)
        {
          m_Checker.template check_graph(g, connPrediction, description);
        }
        else
        {
          m_Checker.template check_graph(g, connPrediction, nodePrediction, description);
        }
      }

      template<class Graph>
      void check_0_0()
      {
        if constexpr(is_static_graph_v<Graph>)
        {
          // Should work in C++ 20;
          // the issue is std::array<T,0> doesn't work
          // for types without a default constructor...

          //constexpr Graph g{};
          //check_0_0(g);
        }
        else
        {
          const Graph g{};
          check_0_0(g); 
        }
      }

      template<class Graph>
      void check_1_0()
      {
        using NodeWeight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<NodeWeight>)
        {
          // Remove restriction in C++20
          if constexpr(!is_static_graph_v<Graph>)
                        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{},{}}, {NodeWeight{}}}; }, LINE("Mismatched nodes and edges"));
          
          if constexpr(is_static_graph_v<Graph>)
          {
            // Should work in C++ 20;
            // the issue is std::array<T,0> doesn't work
            // for types without a default constructor...

            //constexpr Graph g{nodeWeight{1}, {{}}};
            //check_1_0(g);
          }
          else
          {
            const Graph g{{{}}, {NodeWeight{}}};
            check_1_0(g); 
          }
        }        
      }

      template<class Graph>
      void check_2_0()
      {
        using NodeWeight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<NodeWeight>)
        {
          // Remove restriction in C++20
          if constexpr(!is_static_graph_v<Graph>)
                        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{}}, {NodeWeight{}, NodeWeight{}}}; }, LINE("Mismatched nodes and edges"));
          
          if constexpr(is_static_graph_v<Graph>)
          {
            // Should work in C++ 20;
            // the issue is std::array<T,0> doesn't work
            // for types without a default constructor...

            //constexpr Graph g{nodeWeight{1}, {{}}};
            //check_1_0(g);
          }
          else
          {
            const Graph g{{{},{}}, {NodeWeight{}, NodeWeight{}}};
            check_2_0(g); 
          }
        }        
      }
      
    private:
      template<class Graph>
      void check_0_0(const Graph& g)
      {        
        check_graph(g, {}, {}, LINE(""));

        //using conn_prediction_t = std::initializer_list<std::initializer_list<typename Graph::edge_init_type>>;
        //using nodes_prediction_t = std::initializer_list<typename Graph::node_weight_type>;
        //m_Checker.template check_equivalence<Graph, conn_prediction_t, nodes_prediction_t>(g, conn_prediction_t{}, nodes_prediction_t{}, LINE(""));
      }

      template<class Graph>
      void check_1_0(const Graph& g)
      {        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_2_0(const Graph& g)
      {        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{}, {}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }
    };
    
    template<class Checker>
    class undirected_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;

      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
        check_0_0<Graph>();
        check_1_0<Graph>();
        check_1_1<Graph>();
        check_1_2<Graph>();
        check_2_0<Graph>();
        check_2_1<Graph>();
        check_3_2<Graph>();
        check_3_3<Graph>();
        check_3_4<Graph>();
      }

      template<class Graph>
      void check_0_0()
      {
        // Should work in C++ 20;
        // the issue is std::array<T,0> doesn't work
        // for types without a default constructor...

        //if constexpr(is_static_graph_v<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}}; }, LINE("Initializer list too long"));
        // m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}}; }, LINE("Initializer list too long"));
        //}

        init_checker<Checker>::template check_0_0<Graph>();
      }

      template<class Graph>
      void check_1_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_1_0<Graph>();
      }

      template<class Graph>
      void check_2_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_2_0<Graph>();
      }

      template<class Graph>
      void check_1_1()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}}; }, LINE("Too few elements in initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}, edge{0}}}; }, LINE("First partial index of loop out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{1}}}; }, LINE("Second partial index of loop out of range"));
        if constexpr(is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {}}; }, LINE("Initializer list too long"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,2}, edge{0,-2}}}; }, LINE("Mismatch between weights"));
        }
        
        //  /\
        //  \/
        //   x

        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{0}, edge{0}}};
            check_1_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{0, -2}, edge{0, -2}}};
            check_1_1w(g);
          }
        }
        else
        {
          {
            const Graph g{{edge{0}, edge{0}}};
            check_1_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{0, -2}, edge{0, -2}}};
            check_1_1w(g);
          }          
        }

        using node_weight = typename Graph::node_weight_type;        
        if constexpr(!std::is_empty_v<node_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0}, edge{0}}, {}}, {node_weight{}}}; }, LINE("Mismatch between node and edge init"));

          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0}, edge{0}}}, {node_weight{}, node_weight{}}}; }, LINE("Mismatch between node and edge init"));
          
          if constexpr(is_static_graph_v<Graph>)
          {
            {
              constexpr Graph g{{{edge{0}, edge{0}}}, {node_weight{}}};
              check_1_1(g);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              constexpr Graph g{{{edge{0, -2}, edge{0, -2}}}, {node_weight{}}};
              check_1_1w(g);
            }
          }
          else
          {
            {
              const Graph g{{{edge{0}, edge{0}}}, {node_weight{}}};
              check_1_1(g);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              const Graph g{{{edge{0, -2}, edge{0, -2}}}, {node_weight{}}};
              check_1_1w(g);
            }          
          }
        }
      }

      template<class Graph>
      void check_1_2()
      {
        using edge = typename Graph::edge_init_type;

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{0}, edge{0}}}; }, LINE("Too few elements in initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{0}, edge{0}, edge{1}}}; }, LINE("Partial index out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{0}, edge{0}, edge{0}, edge{0}}}; }, LINE("Too Many elements in initializer list"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,2}, edge{0,-1}, edge{0,2}, edge{0,2}}}; }, LINE("Weight mismatch"));
        }

        //  /\
        //  \/
        //   x
        //  /\
        //  \/

        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{0}, edge{0}, edge{0}, edge{0}}};
            check_1_2(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph
              g{{edge{0,1}, edge{0,0}, edge{0,1}, edge{0,0}}},
              g2{{edge{0,-1}, edge{0,-1}, edge{0,-1}, edge{0,-1}}};

              check_1_2w(g,g2);
          }
        }
        else
        {
          {
            const Graph g{{edge{0}, edge{0}, edge{0}, edge{0}}};
            check_1_2(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph
              g{{edge{0,1}, edge{0,0}, edge{0,1}, edge{0,0}}},
              g2{{edge{0,-1}, edge{0,-1}, edge{0,-1}, edge{0,-1}}};

            check_1_2w(g,g2);
          }
        }
      }

      template<class Graph>
      void check_2_1()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {}}; }, LINE("Too few elements in both sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {edge{0}}}; }, LINE("Too few elements in first sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}, {}}; }, LINE("Too few elements in second sub-initializer list"));

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{0}}, {edge{1}}}; }, LINE("Too many elements in first sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}, {edge{1}, edge{0}}}; }, LINE("Too many elements in second sub-initializer list"));

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}, {edge{0}}}; }, LINE("Mismatched indices"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,5}}, {edge{0,-5}}}; }, LINE("Mismatched weights"));
        }
            

        // x------x
        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{1}}, {edge{0}}};
            check_2_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{1,-5}}, {edge{0,-5}}};
            check_2_1w(g);
          }
        }
        else
        {
          {
            const Graph g{{edge{1}}, {edge{0}}};
            check_2_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{1,-5}}, {edge{0,-5}}};
            check_2_1w(g);
          }
          
        }
      }

      template<class Graph>
      void check_3_2()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}, {edge{0}}, {}}; }, LINE("Too few elements in sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}, {edge{0}, edge{0}}, {edge{1}}}; }, LINE("Mismatched partial indices"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}}, {edge{0,8}, edge{2,1}}, {edge{1,8}}}; }, LINE("Mismatched weights"));
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}}, {edge{0,8}, edge{2,8}}, {edge{1,8}}}; }, LINE("Mismatched weight"));
        }
      
        // x-----x-----x
        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph
              g{{edge{1}}, {edge{0}, edge{2}}, {edge{1}}},
              g2{{edge{1}}, {edge{2}, edge{0}}, {edge{1}}};

            check_3_2(g, g2);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph
              g{{edge{1,1}}, {edge{0,1}, edge{2,8}}, {edge{1,8}}},
              g2{{edge{1,2}}, {edge{2,-2}, edge{0,2}}, {edge{1,-2}}};

              check_3_2w(g, g2);
          }
        }
        else
        {
          {
            const Graph
              g{{edge{1}}, {edge{0}, edge{2}}, {edge{1}}},
              g2{{edge{1}}, {edge{2}, edge{0}}, {edge{1}}};

            check_3_2(g, g2);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph
              g{{edge{1,1}}, {edge{0,1}, edge{2,8}}, {edge{1,8}}},
              g2{{edge{1,2}}, {edge{2,-2}, edge{0,2}}, {edge{1,-2}}};

            check_3_2w(g, g2);
          }
        }
      }

      template<class Graph>
      void check_3_3()
      {
        using edge = typename Graph::edge_init_type;
        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){Graph{{edge{1,-2}}, {edge{1,-2}, edge{0,-2}, edge{1,-2}, edge{2,-3}}, {edge{1,-2}}};}, LINE("Mismatched weights"));

          m_Checker.template check_exception_thrown<std::logic_error>([](){Graph{{edge{2,7}, edge{1,2}}, {edge{2,5}, edge{0,2}}, {edge{1,-7}, edge{0,5}}};}, LINE("Mismatched weights"));
        }
        
        using edge = typename Graph::edge_init_type;
        
        //      /\
        //      \/
        // x-----x-----x

        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph
              g{{edge{1}}, {edge{1}, edge{0}, edge{1}, edge{2}}, {edge{1}}},
              g2{{edge{1}}, {edge{2}, edge{1}, edge{1}, edge{0}}, {edge{1}}};

            check_3_3(g, g2);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph
              g{{edge{1,-2}}, {edge{1,-2}, edge{0,-2}, edge{1,-2}, edge{2,-2}}, {edge{1,-2}}},
              g2{{edge{1,-2}}, {edge{2,4}, edge{1,3}, edge{1,3}, edge{0,-2}}, {edge{1,4}}};

              check_3_3w(g, g2);
          }
        }
        else
        {
          {
            const Graph
              g{{edge{1}}, {edge{1}, edge{0}, edge{1}, edge{2}}, {edge{1}}},
              g2{{edge{1}}, {edge{2}, edge{1}, edge{1}, edge{0}}, {edge{1}}};

            check_3_3(g, g2);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph
              g{{edge{1,-2}}, {edge{1,-2}, edge{0,-2}, edge{1,-2}, edge{2,-2}}, {edge{1,-2}}},
              g2{{edge{1,-2}}, {edge{2,4}, edge{1,3}, edge{1,3}, edge{0,-2}}, {edge{1,4}}};

              check_3_3w(g, g2);
          }
        }

        //    x
        //   / \
        //  /   \
        // x-----x

        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{2}, edge{1}}, {edge{2}, edge{0}}, {edge{1}, edge{0}}};
            check_3_3_equilateral(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{2,7}, edge{1,2}}, {edge{2,0}, edge{0,2}}, {edge{1,0}, edge{0,7}}};
            check_3_3w_equilateral(g);
          }
        }
        else
        {
          {
            const Graph g{{edge{2}, edge{1}}, {edge{2}, edge{0}}, {edge{1}, edge{0}}};
            check_3_3_equilateral(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{2,7}, edge{1,2}}, {edge{2,0}, edge{0,2}}, {edge{1,0}, edge{0,7}}};
            check_3_3w_equilateral(g);
          }
        }
      }

      template<class Graph>
      void check_3_4()
      {
        using edge = typename Graph::edge_init_type;

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){Graph{{edge{2,1}, edge{1,0}, edge{1,1}}, {edge{2,1}, edge{0,0}, edge{0,0}}, {edge{1,1}, edge{0,1}}};}, LINE("Mismatched weights"));
        }

        //    x
        //   / \
        //  /   \
        // x=====x

        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{2}, edge{1}, edge{1}}, {edge{2}, edge{0}, edge{0}}, {edge{1}, edge{0}}};
            check_3_4(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{2,1}, edge{1,0}, edge{1,1}}, {edge{2,1}, edge{0,1}, edge{0,0}}, {edge{1,1}, edge{0,1}}};
            check_3_4w(g);
          }
        }
        else
        {
          {
            const Graph g{{edge{2}, edge{1}, edge{1}}, {edge{2}, edge{0}, edge{0}}, {edge{1}, edge{0}}};
            check_3_4(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{2,1}, edge{1,0}, edge{1,1}}, {edge{2,1}, edge{0,1}, edge{0,0}}, {edge{1,1}, edge{0,1}}};
            check_3_4w(g);
          }
        }
      }
    private:
      using init_checker<Checker>::m_Checker;
      using init_checker<Checker>::check_graph;      
      
      template<class Graph>
      void check_1_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0}, edge{0}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_1w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0, -2}, edge{0, -2}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0}, edge{0}, edge{0}, edge{0}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_2w(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0, 0}, edge{0, 0}, edge{0, 1}, edge{0, 1}}}, {NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{0, -1}, edge{0, -1}, edge{0, -1}, edge{0, -1}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_2_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1}}, {edge{0}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_2_1w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1,-5}}, {edge{0,-5}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1}}, {edge{0}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1}}, {edge{0}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));

        m_Checker.check_regular_semantics(g, {{edge{1}}, {edge{0}}, {edge{2}, edge{2}}}, LINE("Regular semantics"));
      }

      template<class Graph>
      void check_3_2w(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1,1}}, {edge{0,1}, edge{2,8}}, {edge{1,8}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1,2}}, {edge{0,2}, edge{2,-2}}, {edge{1,-2}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));       
      }

      template<class Graph>
      void check_3_3(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{1}}, {edge{0}, edge{1}, edge{1}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1}}, {edge{0}, edge{1}, edge{1}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_3w(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g,  {{edge{1,-2}}, {edge{0,-2}, edge{1,-2}, edge{1,-2}, edge{2,-2}}, {edge{1,-2}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1,-2}}, {edge{0,-2}, edge{1,3}, edge{1,3}, edge{2,4}}, {edge{1,4}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_3_equilateral(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{1}, edge{2}}, {edge{0}, edge{2}}, {edge{0}, edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_3w_equilateral(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{1,2}, edge{2,7}}, {edge{0,2}, edge{2,0}}, {edge{0,7}, edge{1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_4(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1}, edge{1}, edge{2}}, {edge{0}, edge{0}, edge{2}}, {edge{0}, edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_4w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        using edge_weight_type = typename edge::weight_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        if constexpr (!is_orderable_v<edge_weight_type>)
        {
          check_graph(g, {{edge{1,0}, edge{1,1}, edge{2,1}}, {edge{0,1}, edge{0,0}, edge{2,1}}, {edge{0,1}, edge{1,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        }
        else
        {
          check_graph(g, {{edge{1,0}, edge{1,1}, edge{2,1}}, {edge{0,0}, edge{0,1}, edge{2,1}}, {edge{0,1}, edge{1,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        }
      }
    };


    template<class Checker>
    class undirected_embedded_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;

      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
        check_0_0<Graph>();
        check_1_0<Graph>();
        check_1_1<Graph>();
        check_1_2<Graph>();
        check_2_0<Graph>();
        check_2_1<Graph>();
        check_3_2<Graph>();
        check_3_3<Graph>();
      }

      template<class Graph>
      void check_0_0()
      {
        // Should work in C++ 20;
        // the issue is std::array<T,0> doesn't work
        // for types without a default constructor...

        //if constexpr(is_static_graph_v<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}}; }, LINE("Initializer list too long"));
        // m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0}}}; }, LINE("Initializer list too long"));
        //}

        init_checker<Checker>::template check_0_0<Graph>();
      }

      template<class Graph>
      void check_1_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_1_0<Graph>();
      }

      template<class Graph>
      void check_2_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_2_0<Graph>();
      }
    
      template<class Graph>
      void check_1_1()
      {
        using edge = typename Graph::edge_init_type;
      
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,1}}}; }, LINE("Too few elements in initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}, edge{0,1}}}; }, LINE("First partial index of loop out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,1}, edge{1,0}}}; }, LINE("Second partial index of loop out of range"));
        if constexpr (is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {}}; }, LINE("Initializer list too long"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0}, edge{0,0}}}; }, LINE("First complementary index is self-referential"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0}, edge{0,1}}}; }, LINE("Second complementary index is self-referential"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,2}, edge{0,0}}}; }, LINE("First complementary index is out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0}, edge{0,2}}}; }, LINE("First complementary index is out of range"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,1,1}, edge{0,0,2}}}; }, LINE("Weight mismatch"));
        }

        //  /\
        //  \/
        //   x

        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{0,1}, edge{0,0}}};
            check_1_1(g);
          }
          
          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{0,1,-1}, edge{0,0,-1}}};
            check_1_1w(g);
          }
        }
        else
        {
          {
            const Graph g{{edge{0,1}, edge{0,0}}};
            check_1_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{0,1,-1}, edge{0,0,-1}}};
            check_1_1w(g);
          }
        }

        using node_weight = typename Graph::node_weight_type;

        if constexpr(!std::is_empty_v<node_weight>)
        {

          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0,1}, edge{0,0}}, {}}, {node_weight{}}}; }, LINE("Mismatch between node and edge init"));

          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0,1}, edge{0,0}}}, {node_weight{}, node_weight{}}}; }, LINE("Mismatch between node and edge init"));
          
          if constexpr(is_static_graph_v<Graph>)
          {
            {
              constexpr Graph g{{{edge{0,1}, edge{0,0}}}, {node_weight{}}};
              check_1_1(g);
            }
          
            if constexpr(!std::is_empty_v<edge_weight>)
            {
              constexpr Graph g{{{edge{0,1,-1}, edge{0,0,-1}}}, {node_weight{}}};
              check_1_1w(g);
            }
          }
          else
          {
            {
              const Graph g{{{edge{0,1}, edge{0,0}}}, {node_weight{}}};
              check_1_1(g);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              const Graph g{{{edge{0,1,-1}, edge{0,0,-1}}}, {node_weight{}}};
              check_1_1w(g);
            }
          }
        }
      }

      template<class Graph>
      void check_1_2()
      {
        using edge = typename Graph::edge_init_type;

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,1}, edge{0,0}, edge{0,0}}}; }, LINE("Too few elements in initializer lists"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,2}, edge{0,3}, edge{0,0}, edge{1,1}}}; }, LINE("Partial index out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,2}, edge{0,3}, edge{0,0}, edge{0,1}, edge{0,0}}}; }, LINE("Too Many elements in initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0}, edge{0,3}, edge{0,2}, edge{0,1}}}; }, LINE("Self-referential complementary indices"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,4}, edge{0,3}, edge{0,0}, edge{0,1}}}; }, LINE("Complementary index out of range"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,2,1}, edge{0,3,1}, edge{0,0,2}, edge{0,1,2}}}; }, LINE("Weight mismatch"));
        }
      
        //  /\
        //  \/
        //   x
        //  /\
        //  \/
      
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{{edge{0,2}, edge{0,3}, edge{0,0}, edge{0,1}}};
          check_1_2(g);
        }
        else
        { 
          const Graph g{{edge{0,2}, edge{0,3}, edge{0,0}, edge{0,1}}};
          check_1_2(g);
        }
      }

      template<class Graph>
      void check_2_1()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {}}; }, LINE("Too few elements in both sub-initializer lists"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {edge{0,0}}}; }, LINE("Too few elements in first sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0}}, {}}; }, LINE("Too few elements in second sub-initializer list"));

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,1}, edge{0,1}}, {edge{1,0}}}; }, LINE("Too many elements in first sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0}}, {edge{1,0}, edge{0,0}}}; }, LINE("Too many elements in second sub-initializer list"));

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}}, {edge{0,0}}}; }, LINE("Complementary index out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}}, {edge{0,1}}}; }, LINE("Complementary indices out of range"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0,1}, edge{0,0,0}}}; }, LINE("Weight mismatch"));
        }
      
        // x------x
        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{1,0}}, {edge{0,0}}};      
            check_2_1(g);
            
            constexpr auto o{g.order()};
            m_Checker.template check_equality<size_t>(2u, o, LINE("Check constexpr order")); 

            constexpr auto s{g.size()};
            m_Checker.template check_equality<size_t>(1u, s, LINE("Check constexpr size"));
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{1,0,-3}}, {edge{0,0,-3}}};      
            check_2_1w(g);

            constexpr auto o{g.order()};
            m_Checker.template check_equality<size_t>(2u, o, LINE("Check constexpr order")); 

            constexpr auto s{g.size()};
            m_Checker.template check_equality<size_t>(1u, s, LINE("Check constexpr size"));
          }
        }
        else
        {
          {
            const Graph g{{edge{1,0}}, {edge{0,0}}};      
            check_2_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{1,0,-3}}, {edge{0,0,-3}}};      
            check_2_1w(g);
          }
        }          
      }

      template<class Graph>
      void check_3_2()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>) m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0}}, {edge{0,0}}, {}}; }, LINE("Too few elements in sub-initializer list"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0}}, {edge{0,0}, edge{0,0}}, {edge{1,0}}}; }, LINE("Mismatched partial indices"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}}, {edge{0,0}, edge{2,0}}, {edge{1,0}}}; }, LINE("Mismatched complementary indices"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0,0}}, {edge{0,0,1}, edge{2,0,0}}, {edge{1,1,0}}}; }, LINE("Weight mismatch"));
        }
        
        // x-----x-----x
        
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph
            g{{edge{1,0}}, {edge{0,0}, edge{2,0}}, {edge{1,1}}},
            g2{{edge{1,1}}, {edge{2,0}, edge{0,0}}, {edge{1,0}}};
        
            check_3_2(g, g2);            
        }
        else
        {
          const Graph
            g{{edge{1,0}}, {edge{0,0}, edge{2,0}}, {edge{1,1}}},
            g2{{edge{1,1}}, {edge{2,0}, edge{0,0}}, {edge{1,0}}};
        
            check_3_2(g, g2);
        }
      }

      template<class Graph>
      void check_3_3()    
      {
        using edge = typename Graph::edge_init_type;
      
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1}}, {edge{1,2}, edge{0,1}, edge{1,0}, edge{2,0}}, {edge{1,3}}}; }, LINE("Mismatched complementary index"));

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,1,2}}, {edge{1,2,2}, edge{0,0,0}, edge{1,0,2}, edge{2,0,-3}}, {edge{1,3,-3}}}; }, LINE("Weight mismatch"));
        }

      
        //      /\
        //      \/
        // x-----x-----x
        
        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph
            g{{edge{1,1}}, {edge{1,2}, edge{0,0}, edge{1,0}, edge{2,0}}, {edge{1,3}}},
            g2{{edge{1,3}}, {edge{2,0}, edge{1,2}, edge{1,1}, edge{0,0}}, {edge{1,0}}};
        
            check_3_3(g, g2);
        }
        else
        {
          const Graph
            g{{edge{1,1}}, {edge{1,2}, edge{0,0}, edge{1,0}, edge{2,0}}, {edge{1,3}}},
            g2{{edge{1,3}}, {edge{2,0}, edge{1,2}, edge{1,1}, edge{0,0}}, {edge{1,0}}};
        
            check_3_3(g, g2);
        }

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{2,1}, edge{1,1}}, {edge{2,0}, edge{0,1}}, {edge{0,0}, edge{1,0}}}; }, LINE("Mismatched complementary indices"));

        //    x
        //   / \
        //  /   \
        // x-----x

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{{edge{2,1}, edge{1,1}}, {edge{2,0}, edge{0,1}}, {edge{1,0}, edge{0,0}}};
          check_3_3_equilateral(g);
        }
        else
        {
          const Graph g{{edge{2,1}, edge{1,1}}, {edge{2,0}, edge{0,1}}, {edge{1,0}, edge{0,0}}};
          check_3_3_equilateral(g);
        }
      }
      
    private:
      using init_checker<Checker>::m_Checker;
      using init_checker<Checker>::check_graph;
      
      template<class Graph>
      void check_1_1(const Graph& g)
      {
        
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{0,1}, edge{0,0}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_1w(const Graph& g)
      {
        
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{0,1, -1}, edge{0,0, -1}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_2(const Graph& g)
      {
        
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{0,2}, edge{0,3}, edge{0,0}, edge{0,1}}}, {NodeWeight{}}, LINE(""));        
      }

      template<class Graph>
      void check_2_1(const Graph& g)
      {
        
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{1,0}}, {edge{0,0}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_2_1w(const Graph& g)
      {
        
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
        
        check_graph(g, {{edge{1,0,-3}}, {edge{0,0,-3}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;
      
        check_graph(g, {{edge{1,0}}, {edge{0,0}, edge{2,0}}, {edge{1,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1,1}}, {edge{2,0}, edge{0,0}}, {edge{1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));

        m_Checker.check_regular_semantics(g, {{edge{1,0}}, {edge{0,0}}, {edge{2,1}, edge{2,0}}}, LINE("Regular semantics"));
      }

      template<class Graph>
      void check_3_3(const Graph& g, const Graph& g2)
      {       
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1,1}}, {edge{1,2}, edge{0,0}, edge{1,0}, edge{2,0}}, {edge{1,3}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1,3}}, {edge{2,0}, edge{1,2}, edge{1,1}, edge{0,0}}, {edge{1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_3_equilateral(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{2,1}, edge{1,1}}, {edge{2,0}, edge{0,1}}, {edge{1,0}, edge{0,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }
    };

    

    template<class Checker>
    class directed_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;

      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
        check_0_0<Graph>();
        check_1_0<Graph>();
        check_1_1<Graph>();
        check_1_2<Graph>();
        check_2_0<Graph>();
        check_2_1<Graph>();
        check_3_2<Graph>();
        check_4_2<Graph>();
      }
      
      template<class Graph>
      void check_0_0()
      {
        // Should work in C++ 20;
        // the issue is std::array<T,0> doesn't work
        // for types without a default constructor...

        //if constexpr(is_static_graph_v<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}}; }, LINE("Initializer list too long"));
        // m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}}; }, LINE("Initializer list too long"));
        //}

        init_checker<Checker>::template check_0_0<Graph>();
      }

      template<class Graph>
      void check_1_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_1_0<Graph>();
      }

      template<class Graph>
      void check_2_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_2_0<Graph>();
      }

      template<class Graph>
      void check_1_1()
      {
        using edge = typename Graph::edge_init_type;

        m_Checker. template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}}; }, LINE("Partial index out of range"));

        //  />\
        //  \ /
        //   x

        using edge_weight = typename edge::weight_type;
        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph g{{edge{0}}};
            check_1_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{0,10}}};
            check_1_1w(g);
          }
        }
        else
        {
          {
            const Graph g{{edge{0}}};
            check_1_1(g);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph g{{edge{0,10}}};
            check_1_1w(g);
          }
        }

        using node_weight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<node_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0}}, {}}, {node_weight{}}}; }, LINE("Mismatch between node and edge init"));

          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0}}}, {node_weight{}, node_weight{}}}; }, LINE("Mismatch between node and edge init"));
          
          if constexpr(is_static_graph_v<Graph>)
          {
            {
              constexpr Graph g{{{edge{0}}}, {node_weight{}}};
              check_1_1(g);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              constexpr Graph g{{{edge{0,10}}}, {node_weight{}}};
              check_1_1w(g);
            }
          }
          else
          {
            {
              const Graph g{{{edge{0}}}, {node_weight{}}};
              check_1_1(g);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              const Graph g{{{edge{0,10}}}, {node_weight{}}};
              check_1_1w(g);
            }
          }
        }
      }

      template<class Graph>
      void check_1_2()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}}}; }, LINE("Too few elements in initializer lists"));
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{0}, edge{0}}}; }, LINE("Too many elements in initializer lists"));
        }
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0}, edge{1}}}; }, LINE("Partial index out of range"));
        
        //  />\
        //  \ /
        //   x
        //  / \
        //  \</

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{{edge{0}, edge{0}}};
          check_1_2(g);
        }
        else
        { 
          const Graph g{{edge{0}, edge{0}}};
          check_1_2(g);
        }
      }

      template<class Graph>
      void check_2_1()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}}; }, LINE("Too few elements in initializer lists"));
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}, {edge{0}}}; }, LINE("Too many elements in initializer lists"));
        }

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{2}}, {}}; }, LINE("Partial index out of range"));

        // x-->---x
        if constexpr(is_static_graph_v<Graph>)
        {      
          constexpr Graph g{{edge{1}}, {}};      
          check_2_1(g);
        }
        else
        {
          const Graph g{{edge{1}},{}};      
          check_2_1(g);
        }          
      }

      template<class Graph>
      void check_3_2()
      {
        using edge = typename Graph::edge_init_type;

        if constexpr(is_static_graph_v<Graph>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}}; }, LINE("Only one element in initializer lists"));
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}, {edge{0}}}; }, LINE("Only two elements in initializer lists"));
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1}}, {edge{0}}, {}, {}}; }, LINE("Too many elements in initializer lists"));
        }

        // x-->--x--<--x
        // x--<--x-->--x

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph
            g{{edge{1}}, {}, {edge{1}}},
            g2{{}, {edge{2}, edge{0}}, {}};

          check_3_2(g, g2);
        }
        else
        {
          const Graph
            g{{edge{1}}, {}, {edge{1}}},
            g2{{}, {edge{2}, edge{0}}, {}};

          check_3_2(g, g2);
        }
      }

      template<class Graph>
      void check_4_2()
      {
        using edge = typename Graph::edge_init_type;

        //        />\
        //        \ /
        // x  x    x-->--x

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{{}, {}, {edge{2}, edge{3}}, {}};
          check_4_2(g);
        }
        else
        {
          const Graph g{{}, {}, {edge{2}, edge{3}}, {}};
          check_4_2(g);
        }
      }
    private:
      using init_checker<Checker>::m_Checker;
      using init_checker<Checker>::check_graph;
      
      template<class Graph>
      void check_1_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_1w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0,10}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_1_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0}, edge{0}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_2_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1}}, {}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{1}}, {}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{}, {edge{2}, edge{0}}, {}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));

        m_Checker.check_regular_semantics(g, g2, LINE("Regular semantics"));
      }

      template<class Graph>
      void check_4_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{}, {}, {edge{2}, edge{3}}, {}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }
    };


    template<class Checker>
    class directed_embedded_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;

      template<class Graph>
      void check_all()
      {
        static_assert(!is_static_graph_v<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
        check_0_0<Graph>();
        check_1_0<Graph>();
        check_1_1<Graph>();
        check_2_0<Graph>();
        check_2_1<Graph>();
        check_3_1<Graph>();
        check_3_2<Graph>();
      }
      
      template<class Graph>
      void check_0_0()
      {
        // Should work in C++ 20;
        // the issue is std::array<T,0> doesn't work
        // for types without a default constructor...

        //if constexpr(is_static_graph_v<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}}; }, LINE("Initializer list too long"));
        // m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0,0}}}; }, LINE("Initializer list too long"));
        //}

        init_checker<Checker>::template check_0_0<Graph>();
      }

      template<class Graph>
      void check_1_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_1_0<Graph>();
      }

      template<class Graph>
      void check_2_0()
      {
        // TO DO: add exception checks in C++20
        
        init_checker<Checker>::template check_2_0<Graph>();
      }

      template<class Graph>
      void check_1_1()
      {
        using maths::inversion_constant;
        using edge = typename Graph::edge_init_type;

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0,1}, edge{0,0,0}}}; }, LINE("Partial index out of range"));
        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{0,0,1}, edge{0,0,1}}}; }, LINE("Mismatched complementary indices"));

        //  />\   /<\
        //  \ /   \ /
        //   x     x

        using edge_weight = typename edge::weight_type;
        if constexpr(is_static_graph_v<Graph>)
        {
          {
            constexpr Graph
              g{{edge{0,0,1}, edge{0,0,0}}},
              g2{{edge{0,inversion_constant<true>{},1}, edge{0,inversion_constant<true>{},0}}};
            check_1_1(g, g2);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph
              g{{edge{0,0,1,9}, edge{0,0,0,9}}},
              g2{{edge{0,inversion_constant<true>{},1,-7}, edge{0,inversion_constant<true>{},0,-7}}};
            check_1_1w(g, g2);
          }
        }
        else
        {
          {
            const Graph
              g{{edge{0,0,1}, edge{0,0,0}}},
              g2{{edge{0,inversion_constant<true>{},1}, edge{0,inversion_constant<true>{},0}}};
            check_1_1(g, g2);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            const Graph
              g{{edge{0,0,1,9}, edge{0,0,0,9}}},
              g2{{edge{0,inversion_constant<true>{},1,-7}, edge{0,inversion_constant<true>{},0,-7}}};
            check_1_1w(g, g2);
          }
        }

        using node_weight = typename Graph::node_weight_type;
        if constexpr(!std::is_empty_v<node_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0,0,1}, edge{0,0,0}}, {}}, {node_weight{}}}; }, LINE("Mismatch between node and edge init"));

          m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{{edge{0,0,1}, edge{0,0,0}}}, {node_weight{}, node_weight{}}}; }, LINE("Mismatch between node and edge init"));
          
          if constexpr(is_static_graph_v<Graph>)
          {
            {
              constexpr Graph
                g{{{edge{0,0,1}, edge{0,0,0}}}, {node_weight{}}},
                g2{{{edge{0,inversion_constant<true>{},1}, edge{0,inversion_constant<true>{},0}}}, {node_weight{}}};
              check_1_1(g, g2);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              constexpr Graph
                g{{{edge{0,0,1,9}, edge{0,0,0,9}}}, {node_weight{}}},
                g2{{{edge{0,inversion_constant<true>{},1,-7}, edge{0,inversion_constant<true>{},0,-7}}}, {node_weight{}}};
              check_1_1w(g, g2);
            }
          }
          else
          {
            {
              const Graph
                g{{{edge{0,0,1}, edge{0,0,0}}}, {node_weight{}}},
                g2{{{edge{0,inversion_constant<true>{},1}, edge{0,inversion_constant<true>{},0}}}, {node_weight{}}};
              check_1_1(g, g2);
            }

            if constexpr(!std::is_empty_v<edge_weight>)
            {
              const Graph
                g{{{edge{0,0,1,9}, edge{0,0,0,9}}}, {node_weight{}}},
                g2{{{edge{0,inversion_constant<true>{},1,-7}, edge{0,inversion_constant<true>{},0,-7}}}, {node_weight{}}};
              check_1_1w(g, g2);
            }
          }
        }
      }

      template<class Graph>
      void check_2_1()
      {
        using edge = typename Graph::edge_init_type;

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{edge{1,0,1}, edge{0,1,0}}}; }, LINE("Mismatched full edges"));

        // x--<--x  x-->--x

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph
            g{{edge{0,1,0}}, {edge{0,1,0}}},
            g2{{edge{1,0,0}}, {edge{1,0,0}}};
            
          check_2_1(g, g2);
        }
        else
        {
          const Graph
            g{{edge{0,1,0}}, {edge{0,1,0}}},
            g2{{edge{1,0,0}}, {edge{1,0,0}}};
            
          check_2_1(g, g2);
        }        
      }

      template<class Graph>
      void check_3_1()
      {
        using edge = typename Graph::edge_init_type;

        m_Checker.template check_exception_thrown<std::logic_error>([](){ Graph{{}, {edge{1,1,1}, edge{1,1,1}}, {}}; }, LINE("Mismatched complementary indices"));

        //    />\
        //    \ /
        // x   x  x

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph g{{}, {edge{1,1,1}, edge{1,1,0}}, {}};
          check_3_1(g);
        }
        else
        {
          const Graph g{{}, {edge{1,1,1}, edge{1,1,0}}, {}};
          check_3_1(g);
        }
      }

      template<class Graph>
      void check_3_2()
      {
        using edge = typename Graph::edge_init_type;

        // x-->--x-->--x
        // x--<--x--<--x

        if constexpr(is_static_graph_v<Graph>)
        {
          constexpr Graph
            g{{edge{0,1,0}}, {edge{0,1,0}, edge{1,2,0}}, {edge{1,2,1}}},
            g2{{edge{1,0,1}}, {edge{2,1,0}, edge{1,0,0}}, {edge{2,1,0}}};

            check_3_2(g, g2);
        }
        else
        {
          const Graph
            g{{edge{0,1,0}}, {edge{0,1,0}, edge{1,2,0}}, {edge{1,2,1}}},
            g2{{edge{1,0,1}}, {edge{2,1,0}, edge{1,0,0}}, {edge{2,1,0}}};
            
          check_3_2(g, g2);
        }
      }
        
    private:
      using init_checker<Checker>::m_Checker;
      using init_checker<Checker>::check_graph;
      
      template<class Graph>
      void check_1_1(const Graph& g, const Graph& g2)
      {
        using maths::inversion_constant;
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0,0,1}, edge{0,0,0}}}, {NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{0,inversion_constant<true>{},1}, edge{0,inversion_constant<true>{},0}}}, {NodeWeight{}}, LINE(""));

        m_Checker.check_regular_semantics(g, g2, LINE("Regular semantics"));
      }

      template<class Graph>
      void check_1_1w(const Graph& g, const Graph& g2)
      {
        using maths::inversion_constant;
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0,0,1,9}, edge{0,0,0,9}}}, {NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{0,inversion_constant<true>{},1,-7}, edge{0,inversion_constant<true>{},0,-7}}}, {NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_2_1(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0,1,0}}, {edge{0,1,0}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
        check_graph(g2, {{edge{1,0,0}}, {edge{1,0,0}}}, {NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{}, {edge{1,1,1}, edge{1,1,0}}, {}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;
        
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(g, {{edge{0,1,0}}, {edge{0,1,0}, edge{1,2,0}}, {edge{1,2,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));

        check_graph(g2, {{edge{1,0,1}}, {edge{2,1,0}, edge{1,0,0}}, {edge{2,1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}}, LINE(""));

        m_Checker.check_regular_semantics(g, g2, LINE("Regular semantics"));
      }
    };
  }
}
