////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"

namespace sequoia
{
  using namespace maths;

  namespace testing
  {
    template<class Checker>
    class init_checker
    {
    public:
      init_checker(Checker& checker) : m_Checker{checker} {}

      [[nodiscard]]
      std::string report_line(const std::filesystem::path& file, int line, std::string_view message)
      {
        return m_Checker.report_line(file, line, message);
      }

    protected:
      Checker& m_Checker;

      ~init_checker() = default;

      template<
        class Graph,
        class Edge=typename Graph::edge_init_type,
        class NodeWeight=typename Graph::node_weight_type
      >
      void check_graph(std::string_view description, const Graph& g, std::initializer_list<std::initializer_list<Edge>> connPrediction, [[maybe_unused]] std::initializer_list<NodeWeight> nodePrediction)
      {
        if constexpr(std::is_empty_v<NodeWeight>)
        {
          m_Checker.check_graph(description, g, connPrediction);
        }
        else
        {
          m_Checker.check_graph(description, g, connPrediction, nodePrediction);
        }
      }

      template<class Graph>
      void check_0_0()
      {
        if constexpr(static_nodes<Graph>)
        {
          // TO DO Should work in C++ 20;
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
          if constexpr(!static_nodes<Graph>)
                        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched nodes and edges"), [](){ return Graph{{{},{}}, {NodeWeight{}}}; });

          if constexpr(static_nodes<Graph>)
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
          if constexpr(!static_nodes<Graph>)
                        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched nodes and edges"), [](){ return Graph{{{}}, {NodeWeight{}, NodeWeight{}}}; });

          if constexpr(static_nodes<Graph>)
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
        check_graph(LINE(""), g, {}, {});

        //using conn_prediction_t = std::initializer_list<std::initializer_list<typename Graph::edge_init_type>>;
        //using nodes_prediction_t = std::initializer_list<typename Graph::node_weight_type>;
        //m_Checker.template check_equivalence<Graph, conn_prediction_t, nodes_prediction_t>(LINE(""), g, conn_prediction_t{}, nodes_prediction_t{});
      }

      template<class Graph>
      void check_1_0(const Graph& g)
      {
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_2_0(const Graph& g)
      {
        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{}, {}}, {NodeWeight{}, NodeWeight{}});
      }
    };

    template<class Checker>
    class undirected_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;
      using init_checker<Checker>::report_line;

      template<class Graph>
      void check_all()
      {
        static_assert(!static_nodes<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
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

        //if constexpr(static_nodes<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{}}; });
        // m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{edge{0}}}; });
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

        if constexpr(static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in initializer list"), [](){ return Graph{{edge{0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("First partial index of loop out of range"), [](){ return Graph{{edge{1}, edge{0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Second partial index of loop out of range"), [](){ return Graph{{edge{0}, edge{1}}}; });
        if constexpr(static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{}, {}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between weights"), [](){ return Graph{{edge{0,2}, edge{0,-2}}}; });
        }

        //  /\
        //  \/
        //   x

        if constexpr(static_nodes<Graph>)
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
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0}, edge{0}}, {}}, {node_weight{}}}; });

          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0}, edge{0}}}, {node_weight{}, node_weight{}}}; });

          if constexpr(static_nodes<Graph>)
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in initializer list"), [](){ return Graph{{edge{0}, edge{0}, edge{0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Partial index out of range"), [](){ return Graph{{edge{0}, edge{0}, edge{0}, edge{1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too Many elements in initializer list"), [](){ return Graph{{edge{0}, edge{0}, edge{0}, edge{0}, edge{0}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Weight mismatch"), [](){ return Graph{{edge{0,2}, edge{0,-1}, edge{0,2}, edge{0,2}}}; });
        }

        //  /\
        //  \/
        //   x
        //  /\
        //  \/

        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in both sub-initializer list"), [](){ return Graph{{}, {}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in first sub-initializer list"), [](){ return Graph{{}, {edge{0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in second sub-initializer list"), [](){ return Graph{{edge{0}}, {}}; });

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in first sub-initializer list"), [](){ return Graph{{edge{0}, edge{0}}, {edge{1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in second sub-initializer list"), [](){ return Graph{{edge{0}}, {edge{1}, edge{0}}}; });

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched indices"), [](){ return Graph{{edge{0}}, {edge{0}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched weights"), [](){ return Graph{{edge{1,5}}, {edge{0,-5}}}; });
        }


        // x------x
        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in sub-initializer list"), [](){ return Graph{{edge{1}}, {edge{0}}, {}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched partial indices"), [](){ return Graph{{edge{1}}, {edge{0}, edge{0}}, {edge{1}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched weights"), [](){ return Graph{{edge{1,1}}, {edge{0,8}, edge{2,1}}, {edge{1,8}}}; });
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched weight"), [](){ return Graph{{edge{1,1}}, {edge{0,8}, edge{2,8}}, {edge{1,8}}}; });
        }

        // x-----x-----x
        if constexpr(static_nodes<Graph>)
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
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched weights"), [](){ return Graph{{edge{1,-2}}, {edge{1,-2}, edge{0,-2}, edge{1,-2}, edge{2,-3}}, {edge{1,-2}}};});

          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched weights"), [](){ return Graph{{edge{2,7}, edge{1,2}}, {edge{2,5}, edge{0,2}}, {edge{1,-7}, edge{0,5}}};});
        }

        using edge = typename Graph::edge_init_type;

        //      /\
        //      \/
        // x-----x-----x

        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>)
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
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched weights"), [](){ return Graph{{edge{2,1}, edge{1,0}, edge{1,1}}, {edge{2,1}, edge{0,0}, edge{0,0}}, {edge{1,1}, edge{0,1}}};});
        }

        //    x
        //   / \
        //  /   \
        // x=====x

        if constexpr(static_nodes<Graph>)
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

        check_graph(LINE(""), g, {{edge{0}, edge{0}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_1w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0, -2}, edge{0, -2}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0}, edge{0}, edge{0}, edge{0}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_2w(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0, 0}, edge{0, 0}, edge{0, 1}, edge{0, 1}}}, {NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{0, -1}, edge{0, -1}, edge{0, -1}, edge{0, -1}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_2_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}}, {edge{0}}}, {NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_2_1w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,-5}}, {edge{0,-5}}}, {NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}}, {edge{0}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1}}, {edge{0}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});

        m_Checker.check_semantics(LINE("Regular semantics"), g, {{edge{1}}, {edge{0}}, {edge{2}, edge{2}}});
      }

      template<class Graph>
      void check_3_2w(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,1}}, {edge{0,1}, edge{2,8}}, {edge{1,8}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1,2}}, {edge{0,2}, edge{2,-2}}, {edge{1,-2}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_3(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}}, {edge{0}, edge{1}, edge{1}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1}}, {edge{0}, edge{1}, edge{1}, edge{2}}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_3w(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g,  {{edge{1,-2}}, {edge{0,-2}, edge{1,-2}, edge{1,-2}, edge{2,-2}}, {edge{1,-2}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1,-2}}, {edge{0,-2}, edge{1,3}, edge{1,3}, edge{2,4}}, {edge{1,4}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_3_equilateral(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}, edge{2}}, {edge{0}, edge{2}}, {edge{0}, edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_3w_equilateral(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,2}, edge{2,7}}, {edge{0,2}, edge{2,0}}, {edge{0,7}, edge{1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_4(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}, edge{1}, edge{2}}, {edge{0}, edge{0}, edge{2}}, {edge{0}, edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_4w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;
        using edge_weight_type = typename edge::weight_type;

        using NodeWeight = typename Graph::node_weight_type;

        if constexpr (!std::totally_ordered<edge_weight_type>)
        {
          check_graph(LINE(""), g, {{edge{1,0}, edge{1,1}, edge{2,1}}, {edge{0,1}, edge{0,0}, edge{2,1}}, {edge{0,1}, edge{1,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        }
        else
        {
          check_graph(LINE(""), g, {{edge{1,0}, edge{1,1}, edge{2,1}}, {edge{0,0}, edge{0,1}, edge{2,1}}, {edge{0,1}, edge{1,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        }
      }
    };


    template<class Checker>
    class undirected_embedded_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;
      using init_checker<Checker>::report_line;

      template<class Graph>
      void check_all()
      {
        static_assert(!static_nodes<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
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

        //if constexpr(static_nodes<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{}}; });
        // m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{edge{0,0}}}; });
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in initializer list"), [](){ return Graph{{edge{0,1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("First partial index of loop out of range"), [](){ return Graph{{edge{1,1}, edge{0,1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Second partial index of loop out of range"), [](){ return Graph{{edge{0,1}, edge{1,0}}}; });
        if constexpr (static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{}, {}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("First complementary index is self-referential"), [](){ return Graph{{edge{0,0}, edge{0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Second complementary index is self-referential"), [](){ return Graph{{edge{0,0}, edge{0,1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("First complementary index is out of range"), [](){ return Graph{{edge{0,2}, edge{0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("First complementary index is out of range"), [](){ return Graph{{edge{0,0}, edge{0,2}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Weight mismatch"), [](){ return Graph{{edge{0,1,1}, edge{0,0,2}}}; });
        }

        //  /\
        //  \/
        //   x

        if constexpr(static_nodes<Graph>)
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

          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0,1}, edge{0,0}}, {}}, {node_weight{}}}; });

          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0,1}, edge{0,0}}}, {node_weight{}, node_weight{}}}; });

          if constexpr(static_nodes<Graph>)
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in initializer lists"), [](){ return Graph{{edge{0,1}, edge{0,0}, edge{0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Partial index out of range"), [](){ return Graph{{edge{0,2}, edge{0,3}, edge{0,0}, edge{1,1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too Many elements in initializer list"), [](){ return Graph{{edge{0,2}, edge{0,3}, edge{0,0}, edge{0,1}, edge{0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Self-referential complementary indices"), [](){ return Graph{{edge{0,0}, edge{0,3}, edge{0,2}, edge{0,1}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Complementary index out of range"), [](){ return Graph{{edge{0,4}, edge{0,3}, edge{0,0}, edge{0,1}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Weight mismatch"), [](){ return Graph{{edge{0,2,1}, edge{0,3,1}, edge{0,0,2}, edge{0,1,2}}}; });
        }

        //  /\
        //  \/
        //   x
        //  /\
        //  \/

        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in both sub-initializer lists"), [](){ return Graph{{}, {}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in first sub-initializer list"), [](){ return Graph{{}, {edge{0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in second sub-initializer list"), [](){ return Graph{{edge{1,0}}, {}}; });

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in first sub-initializer list"), [](){ return Graph{{edge{0,1}, edge{0,1}}, {edge{1,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in second sub-initializer list"), [](){ return Graph{{edge{1,0}}, {edge{1,0}, edge{0,0}}}; });

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Complementary index out of range"), [](){ return Graph{{edge{1,1}}, {edge{0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Complementary indices out of range"), [](){ return Graph{{edge{1,1}}, {edge{0,1}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Weight mismatch"), [](){ return Graph{{edge{1,0,1}, edge{0,0,0}}}; });
        }

        // x------x
        if constexpr(static_nodes<Graph>)
        {
          {
            constexpr Graph g{{edge{1,0}}, {edge{0,0}}};
            check_2_1(g);

            constexpr auto o{g.order()};
            m_Checker.check(equality, LINE("Check constexpr order"), o, 2_sz);

            constexpr auto s{g.size()};
            m_Checker.check(equality, LINE("Check constexpr size"), s, 1_sz);
          }

          if constexpr(!std::is_empty_v<edge_weight>)
          {
            constexpr Graph g{{edge{1,0,-3}}, {edge{0,0,-3}}};
            check_2_1w(g);

            constexpr auto o{g.order()};
            m_Checker.check(equality, LINE("Check constexpr order"), o, 2_sz);

            constexpr auto s{g.size()};
            m_Checker.check(equality, LINE("Check constexpr size"), s, 1_sz);
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

        if constexpr(static_nodes<Graph>) m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in sub-initializer list"), [](){ return Graph{{edge{1,0}}, {edge{0,0}}, {}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched partial indices"), [](){ return Graph{{edge{1,0}}, {edge{0,0}, edge{0,0}}, {edge{1,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched complementary indices"), [](){ return Graph{{edge{1,1}}, {edge{0,0}, edge{2,0}}, {edge{1,0}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Weight mismatch"), [](){ return Graph{{edge{1,0,0}}, {edge{0,0,1}, edge{2,0,0}}, {edge{1,1,0}}}; });
        }

        // x-----x-----x

        if constexpr(static_nodes<Graph>)
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched complementary index"), [](){ return Graph{{edge{1,1}}, {edge{1,2}, edge{0,1}, edge{1,0}, edge{2,0}}, {edge{1,3}}}; });

        using edge_weight = typename edge::weight_type;
        if constexpr(!std::is_empty_v<edge_weight>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Weight mismatch"), [](){ return Graph{{edge{1,1,2}}, {edge{1,2,2}, edge{0,0,0}, edge{1,0,2}, edge{2,0,-3}}, {edge{1,3,-3}}}; });
        }


        //      /\
        //      \/
        // x-----x-----x

        if constexpr(static_nodes<Graph>)
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched complementary indices"), [](){ return Graph{{edge{2,1}, edge{1,1}}, {edge{2,0}, edge{0,1}}, {edge{0,0}, edge{1,0}}}; });

        //    x
        //   / \
        //  /   \
        // x-----x

        if constexpr(static_nodes<Graph>)
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

        check_graph(LINE(""), g, {{edge{0,1}, edge{0,0}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_1w(const Graph& g)
      {

        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0,1, -1}, edge{0,0, -1}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_2(const Graph& g)
      {

        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0,2}, edge{0,3}, edge{0,0}, edge{0,1}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_2_1(const Graph& g)
      {

        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,0}}, {edge{0,0}}}, {NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_2_1w(const Graph& g)
      {

        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,0,-3}}, {edge{0,0,-3}}}, {NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {

        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,0}}, {edge{0,0}, edge{2,0}}, {edge{1,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1,1}}, {edge{2,0}, edge{0,0}}, {edge{1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});

        m_Checker.check_semantics(LINE("Regular semantics"), g, {{edge{1,0}}, {edge{0,0}}, {edge{2,1}, edge{2,0}}});
      }

      template<class Graph>
      void check_3_3(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1,1}}, {edge{1,2}, edge{0,0}, edge{1,0}, edge{2,0}}, {edge{1,3}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1,3}}, {edge{2,0}, edge{1,2}, edge{1,1}, edge{0,0}}, {edge{1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_3_equilateral(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{2,1}, edge{1,1}}, {edge{2,0}, edge{0,1}}, {edge{1,0}, edge{0,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }
    };



    template<class Checker>
    class directed_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;
      using init_checker<Checker>::report_line;

      template<class Graph>
      void check_all()
      {
        static_assert(!static_nodes<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
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

        //if constexpr(static_nodes<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{}}; });
        // m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{edge{0}}}; });
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

        m_Checker. template check_exception_thrown<std::logic_error>(LINE("Partial index out of range"), [](){ return Graph{{edge{1}}}; });

        //  />\
        //  \ /
        //   x

        using edge_weight = typename edge::weight_type;
        if constexpr(static_nodes<Graph>)
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
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0}}, {}}, {node_weight{}}}; });

          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0}}}, {node_weight{}, node_weight{}}}; });

          if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in initializer lists"), [](){ return Graph{{edge{0}}}; });
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in initializer lists"), [](){ return Graph{{edge{0}, edge{0}, edge{0}}}; });
        }
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Partial index out of range"), [](){ return Graph{{edge{0}, edge{1}}}; });

        //  />\
        //  \ /
        //   x
        //  / \
        //  \</

        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too few elements in initializer lists"), [](){ return Graph{{edge{1}}}; });
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in initializer lists"), [](){ return Graph{{edge{1}}, {edge{0}}}; });
        }

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Partial index out of range"), [](){ return Graph{{edge{2}}, {}}; });

        // x-->---x
        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>)
        {
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Only one element in initializer lists"), [](){ return Graph{{edge{1}}}; });
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Only two elements in initializer lists"), [](){ return Graph{{edge{1}}, {edge{0}}}; });
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Too many elements in initializer lists"), [](){ return Graph{{edge{1}}, {edge{0}}, {}, {}}; });
        }

        // x-->--x--<--x
        // x--<--x-->--x

        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>)
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

        check_graph(LINE(""), g, {{edge{0}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_1w(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0,10}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_1_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0}, edge{0}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_2_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}}, {}}, {NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{1}}, {}, {edge{1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{}, {edge{2}, edge{0}}, {}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});

        m_Checker.check_semantics(LINE("Regular semantics"), g, g2);
      }

      template<class Graph>
      void check_4_2(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{}, {}, {edge{2}, edge{3}}, {}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }
    };


    template<class Checker>
    class directed_embedded_init_checker : protected init_checker<Checker>
    {
    public:
      using init_checker<Checker>::init_checker;
      using init_checker<Checker>::report_line;

      template<class Graph>
      void check_all()
      {
        static_assert(!static_nodes<Graph>, "This call only makes sense for dynamic graphs, which are of the same type for all orders/sizes");
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

        //if constexpr(static_nodes<Graph>)
        //{
        //  using edge = typename Graph::edge_init_type;

        //  m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{}}; });
        // m_Checker.template check_exception_thrown<std::logic_error>(LINE("Initializer list too long"), [](){ return Graph{{edge{0,0,0}}}; });
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Partial index out of range"), [](){ return Graph{{edge{1,0,1}, edge{0,0,0}}}; });
        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched complementary indices"), [](){ return Graph{{edge{0,0,1}, edge{0,0,1}}}; });

        //  />\   /<\
        //  \ /   \ /
        //   x     x

        using edge_weight = typename edge::weight_type;
        if constexpr(static_nodes<Graph>)
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
          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0,0,1}, edge{0,0,0}}, {}}, {node_weight{}}}; });

          m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatch between node and edge init"), [](){ return Graph{{{edge{0,0,1}, edge{0,0,0}}}, {node_weight{}, node_weight{}}}; });

          if constexpr(static_nodes<Graph>)
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched full edges"), [](){ return Graph{{edge{1,0,1}, edge{0,1,0}}}; });

        // x--<--x  x-->--x

        if constexpr(static_nodes<Graph>)
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

        m_Checker.template check_exception_thrown<std::logic_error>(LINE("Mismatched complementary indices"), [](){ return Graph{{}, {edge{1,1,1}, edge{1,1,1}}, {}}; });

        //    />\
        //    \ /
        // x   x  x

        if constexpr(static_nodes<Graph>)
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

        if constexpr(static_nodes<Graph>)
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

        check_graph(LINE(""), g, {{edge{0,0,1}, edge{0,0,0}}}, {NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{0,inversion_constant<true>{},1}, edge{0,inversion_constant<true>{},0}}}, {NodeWeight{}});

        m_Checker.check_semantics(LINE("Regular semantics"), g, g2);
      }

      template<class Graph>
      void check_1_1w(const Graph& g, const Graph& g2)
      {
        using maths::inversion_constant;
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0,0,1,9}, edge{0,0,0,9}}}, {NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{0,inversion_constant<true>{},1,-7}, edge{0,inversion_constant<true>{},0,-7}}}, {NodeWeight{}});
      }

      template<class Graph>
      void check_2_1(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0,1,0}}, {edge{0,1,0}}}, {NodeWeight{}, NodeWeight{}});
        check_graph(LINE(""), g2, {{edge{1,0,0}}, {edge{1,0,0}}}, {NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_1(const Graph& g)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{}, {edge{1,1,1}, edge{1,1,0}}, {}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});
      }

      template<class Graph>
      void check_3_2(const Graph& g, const Graph& g2)
      {
        using edge = typename Graph::edge_init_type;

        using NodeWeight = typename Graph::node_weight_type;

        check_graph(LINE(""), g, {{edge{0,1,0}}, {edge{0,1,0}, edge{1,2,0}}, {edge{1,2,1}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});

        check_graph(LINE(""), g2, {{edge{1,0,1}}, {edge{2,1,0}, edge{1,0,0}}, {edge{2,1,0}}}, {NodeWeight{}, NodeWeight{}, NodeWeight{}});

        m_Checker.check_semantics(LINE("Regular semantics"), g, g2);
      }
    };
  }
}
