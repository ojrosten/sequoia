#pragma once

#include "TestGraphHelper.hpp"

namespace sequoia::unit_testing
{
  class test_graph_meta : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:
    void run_tests() override;

    template<
      maths::graph_flavour GraphFlavour,
      template<class, template<class> class, class, class> class EdgeType
    >
    void test_undirected();

    void test_directed();
    
    void test_directed_embedded();

        
    template<
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      template<class> class EdgeWeightStorage,
      template<class, template<class> class, class, class> class EdgeType
    >
    void test_undirected_unshared();

    template<
      maths::graph_flavour GraphFlavour,
      class EdgeWeight,
      template<class> class EdgeWeightStorage,
      template<class, template<class> class, class, class> class EdgeType
    >
    void test_undirected_shared();

    template<
      class EdgeWeight,
      template<class> class EdgeWeightStorage
    >
    void test_directed_impl();

    template<
      class EdgeWeight,
      template<class> class EdgeWeightStorage
    >
    void test_directed_embedded_impl();
  };

  template<class T> struct wrapper
  {
    T val{};
  };

  template<class T> constexpr bool operator==(const wrapper<T>& lhs, const wrapper<T>& rhs) noexcept
  {
    return lhs.val == rhs.val;
  }

  template<class T> constexpr bool operator!=(const wrapper<T>& lhs, const wrapper<T>& rhs) noexcept
  {
    return !(lhs == rhs);
  }
}
