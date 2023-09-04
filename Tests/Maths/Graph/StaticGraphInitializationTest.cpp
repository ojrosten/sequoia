////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticGraphInitializationTest.hpp"
#include "GraphInitializationGenericTests.hpp"

#include "sequoia/Maths/Graph/StaticGraph.hpp"

namespace sequoia::testing
{
  namespace
  {
    struct unsortable
    {
      int x{};

      [[nodiscard]]
      friend constexpr bool operator==(const unsortable& lhs, const unsortable& rhs) noexcept = default;

      template<class Stream> friend Stream& operator<<(Stream& s, const unsortable& u)
      {
        s << std::to_string(u.x);
        return s;
      }
    };

    struct big_unsortable
    {
      int w{}, x{1}, y{2}, z{3};

      friend constexpr bool operator==(const big_unsortable& lhs, const big_unsortable& rhs) noexcept = default;

      template<class Stream> friend Stream& operator<<(Stream& s, const big_unsortable& u)
      {
        s << std::to_string(u.w) << ' ' << std::to_string(u.x) << ' ' << std::to_string(u.y) << ' ' << std::to_string(u.z);
        return s;
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path test_static_graph::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_static_graph::run_tests()
  {
    using namespace maths;

    test_generic_undirected<null_weight,null_weight>();
    test_generic_undirected<null_weight, int>();
    test_generic_undirected<null_weight,unsortable>();
    test_generic_undirected<null_weight, big_unsortable>();
    test_generic_undirected<int, null_weight>();
    test_generic_undirected<int, int>();
    test_generic_undirected<int, unsortable>();
    test_generic_undirected<int, big_unsortable>();

    test_generic_embedded_undirected<null_weight, null_weight>();
    test_generic_embedded_undirected<null_weight, int>();
    test_generic_embedded_undirected<null_weight, unsortable>();
    test_generic_embedded_undirected<null_weight, big_unsortable>();
    test_generic_embedded_undirected<int, null_weight>();
    test_generic_embedded_undirected<int, int>();
    test_generic_embedded_undirected<int, unsortable>();
    test_generic_embedded_undirected<int, big_unsortable>();
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_graph::test_generic_undirected()
  {
    using namespace maths;

    undirected_init_checker<test_static_graph> checker{*this};

    {
      using g_type = static_undirected_graph<0, 0, EdgeWeight, NodeWeight>;
      checker.template check_0_0<g_type>();
    }

    {
      using g_type = static_undirected_graph<0, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }

    {
      using g_type = static_undirected_graph<1, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_1<g_type>();
    }

    {
      using g_type = static_undirected_graph<2, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_2<g_type>();
    }

    {
      using g_type = static_undirected_graph<0, 2, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }

    {
      using g_type = static_undirected_graph<1, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_1<g_type>();

      // Think about alignment for non-empty T...
      if constexpr (std::is_empty_v<NodeWeight> && std::is_empty_v<EdgeWeight>)
      {
        check(equality, report_line("2 bytes for each half edge and 2 for the partition data"), sizeof(g_type), 4_sz);
      }

      static_assert(std::is_same_v<typename g_type::edge_index_type, unsigned char>);
    }

    {
      using g_type = static_undirected_graph<2, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_2<g_type>();
    }

    {
      using g_type = static_undirected_graph<3, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_3<g_type>();
    }

    {
      using g_type = static_undirected_graph<4, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_4<g_type>();
    }
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_graph::test_generic_embedded_undirected()
  {
    using namespace maths;

    undirected_embedded_init_checker<test_static_graph> checker{*this};

    {
      using g_type = static_embedded_graph<0, 0, EdgeWeight, NodeWeight>;
      checker.template check_0_0<g_type>();
    }

    {
      using g_type = static_embedded_graph<0, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }

    {
      using g_type = static_embedded_graph<1, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_1<g_type>();
    }

    {
      using g_type = static_embedded_graph<2, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_2<g_type>();
    }

    {
      using g_type = static_embedded_graph<0, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_0<g_type>();
    }

    {
      using g_type = static_embedded_graph<1, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_1<g_type>();
    }

    {
      using g_type = static_embedded_graph<2, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_2<g_type>();
    }

    {
      using g_type = static_embedded_graph<3, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_3<g_type>();
    }
  }
}
