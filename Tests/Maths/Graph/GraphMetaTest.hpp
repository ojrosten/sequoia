////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class test_graph_meta final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept;

    void run_tests();
  private:

    void test_method_detectors();

    template
    <
      maths::graph_flavour GraphFlavour,
      template<class, class> class EdgeType
    >
    void test_undirected();

    void test_directed();

    void test_directed_embedded();

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class EdgeWeightCreator,
      template<class, class> class EdgeType
    >
    void test_undirected_unshared();

    template
    <
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      class EdgeWeightCreator,
      template<class, class> class EdgeType
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

    [[nodiscard]] friend constexpr auto operator<=>(const wrapper& lhs, const wrapper& rhs) noexcept = default;
  };
}
