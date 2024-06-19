////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    // Overload for my graph
    // Problem: for graphs with a null edge weight, I don't define vertices
    template<network G>
    auto vertices(const G& g);

    template<class G>
    using vertex_range_t = std::invoke_result_t<decltype(vertices<G>), G>;

    template<class G>
    using vertex_iterator_t = std::ranges::iterator_t<vertex_range_t<G>>; // paper missing ranges::

    // Overload for my graph
    template<network G>
    std::size_t vertex_id(const G& g, vertex_iterator_t<G> ui);

    template<class G>
    using vertex_id_t = std::invoke_result_t<decltype(vertex_id<G>), G, vertex_iterator_t<G>>;

    // Overload for my graph
    template<network G>
    typename G::const_edge_range edges(const G& g, vertex_id_t<G> uid) { return g.cedges(uid); }

    template<class G>
    using vertex_edge_range_t = std::invoke_result_t<decltype(edges<G>), G, vertex_id_t<G>>;

    template<class G>
    using edge_reference_t = std::ranges::range_reference_t<vertex_edge_range_t<G>>; // paper missing ranges::

    template <class G, class E> // Problem: E is not used!
    concept basic_targeted_edge = requires(G&& g, edge_reference_t<G> uv) { target_id(g, uv); }; // -> std::size_t ??
    // Does paper mean to std::forward<G>(g)? 
  }

  [[nodiscard]]
  std::filesystem::path experimental_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void experimental_test::run_tests()
  {
  }
}
