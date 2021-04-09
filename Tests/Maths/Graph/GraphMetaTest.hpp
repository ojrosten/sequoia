////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_graph_meta final : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_method_detectors();

    template
    <
      maths::graph_flavour GraphFlavour,
      template<class, template<class> class, class, class> class EdgeType
    >
    void test_undirected();

    void test_directed();

    void test_directed_embedded();

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class EdgeWeightPooling,
      template<class, template<class> class, class, class> class EdgeType
    >
    void test_undirected_unshared();

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class EdgeWeightPooling,
      template<class, template<class> class, class, class> class EdgeType
    >
    void test_undirected_shared();

    template
    <
      class EdgeWeight,
      class EdgeWeightPooling
    >
    void test_directed_impl();

    template
    <
      class EdgeWeight,
      class EdgeWeightPooling
    >
    void test_directed_embedded_impl();

    void test_static_edge_index_generator();
  };

  template<class T> struct wrapper
  {
    T val{};

    [[nodiscard]] friend constexpr bool operator==(const wrapper& lhs, const wrapper& rhs) noexcept = default;

    [[nodiscard]] friend constexpr bool operator!=(const wrapper& lhs, const wrapper& rhs) noexcept = default;
  };
}
