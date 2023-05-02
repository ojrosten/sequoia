////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Maths/Graph/NodeStorage.hpp"
#include "sequoia/Maths/Graph/StaticNodeStorage.hpp"
#include "sequoia/Maths/Graph/HeterogeneousNodeStorage.hpp"

#include "sequoia/TestFramework/AllocationTestUtilities.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<class Nodes>
    struct node_value_tester
    {
      using type = Nodes;

      template<test_mode Mode>
      static void test(equality_check_t, test_logger<Mode>& logger, const Nodes& nodes, const Nodes& prediction)
      {
        check(equality, "Sizes different", logger, nodes.size(), prediction.size());

        check(with_best_available, "const_node_iter", logger, nodes.cbegin_node_weights(), nodes.cend_node_weights(), prediction.cbegin_node_weights(), prediction.cend_node_weights());
        check(with_best_available, "const_reverse_node_iter", logger, nodes.crbegin_node_weights(), nodes.crend_node_weights(), prediction.crbegin_node_weights(), prediction.crend_node_weights());
      }
    };

    template<class Nodes>
      requires std::is_empty_v<typename Nodes::weight_type>
    struct node_value_tester<Nodes>
    {
      using type = Nodes;

      template<test_mode Mode>
      static void test(equality_check_t, test_logger<Mode>&, const Nodes&, const Nodes&)
      {
      }
    };

    template<class Nodes>
    struct node_equivalence_checker
    {
      using type = Nodes;

      using equivalent_type = std::initializer_list<typename type::weight_type>;

      template<test_mode Mode>
      static void test(equivalence_check_t, test_logger<Mode>& logger, const Nodes& nodes, equivalent_type prediction)
      {
        check(equality, "Sizes different", logger, nodes.size(), prediction.size());

        check(with_best_available, "const_node_iter", logger, nodes.cbegin_node_weights(), nodes.cend_node_weights(), prediction.begin(), prediction.end());
        check(with_best_available, "implicitly const range", logger, nodes.node_weights().begin(), nodes.node_weights().end(), prediction.begin(), prediction.end());
        check(with_best_available, "explicitly const range", logger, nodes.cnode_weights().begin(), nodes.cnode_weights().end(), prediction.begin(), prediction.end());

        using weight_proxy_type = Nodes::weight_proxy_type;
        if constexpr(object::transparent_wrapper<weight_proxy_type>)
        {
          auto& n{const_cast<Nodes&>(nodes)};
          check(with_best_available, "range", logger, n.node_weights().begin(), n.node_weights().end(), prediction.begin(), prediction.end());
        }
      }
    };

    template<class Nodes>
      requires std::is_empty_v<typename Nodes::weight_type>
    struct node_equivalence_checker<Nodes>
    {
      using type = Nodes;

      using equivalent_type = std::initializer_list<typename type::weight_type>;

      template<test_mode Mode>
      static void test(equivalence_check_t, test_logger<Mode>& logger, const Nodes& nodes, equivalent_type)
      {
        testing::check("Node storage should have zero size for empty node weights", logger, nodes.empty());
      }
    };
  }

  // Details Checkers

  template<class WeightMaker, class Traits>
  struct value_tester<maths::graph_impl::node_storage<WeightMaker, Traits>>
    : impl::node_value_tester<maths::graph_impl::node_storage<WeightMaker, Traits>>
    , impl::node_equivalence_checker<maths::graph_impl::node_storage<WeightMaker, Traits>>
  {
    using impl::node_value_tester<maths::graph_impl::node_storage<WeightMaker, Traits>>::test;
    using impl::node_equivalence_checker<maths::graph_impl::node_storage<WeightMaker, Traits>>::test;
  };

  // Static

  template<class WeightMaker, std::size_t N>
  struct value_tester<maths::graph_impl::static_node_storage<WeightMaker, N>>
    : impl::node_value_tester<maths::graph_impl::static_node_storage<WeightMaker, N>>
    , impl::node_equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
  {
    using impl::node_value_tester<maths::graph_impl::static_node_storage<WeightMaker, N>>::test;
    using impl::node_equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>::test;
  };

  // Heterogeneous

  template<class... Ts>
  struct value_tester<maths::graph_impl::heterogeneous_node_storage<Ts...>>
  {
    using type = maths::graph_impl::heterogeneous_node_storage<Ts...>;
    using equivalent_type = std::tuple<Ts...>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& nodes, const type& prediction)
    {
      if(check(equality, "Node storaage sizes different", logger, nodes.size(), prediction.size()))
      {
        check_elements(logger, nodes, prediction);
      }
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& nodes, const equivalent_type& prediction)
    {
      if (check(equality, "Node storage sizes different", logger, nodes.size(), sizeof...(Ts)))
      {
        check_elements(logger, nodes, prediction);
      }
    }

  private:
    template<test_mode Mode, std::size_t I=0>
    static void check_elements([[maybe_unused]] test_logger<Mode>& logger,
                               [[maybe_unused]] const type& nodes,
                               [[maybe_unused]] const type& prediction)
    {
      if constexpr(I < sizeof...(Ts))
      {
        const std::string message{std::to_string(I) + "th element incorrect"};
        check(equality, message, logger, nodes.template node_weight<I>(), prediction.template node_weight<I>());
        check_elements<Mode, I+1>(logger, nodes, prediction);
      }
    }

    template<test_mode Mode, std::size_t I = 0>
    static void check_elements([[maybe_unused]] test_logger<Mode>& logger,
                               [[maybe_unused]] const type& nodes,
                               [[maybe_unused]] const equivalent_type& prediction)
    {
      if constexpr (I < sizeof...(Ts))
      {
        const auto message{ std::to_string(I).append("th element incorrect") };
        check(equality, message, logger, nodes.template node_weight<I>(), std::get<I>(prediction));
        check_elements<Mode, I + 1>(logger, nodes, prediction);
      }
    }
  };


  template<bool PropagateCopy=true, bool PropagateMove=true, bool PropagateSwap=true>
  struct node_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, PropagateCopy, PropagateMove, PropagateSwap>>;
  };

  template<class WeightMaker, bool PropagateCopy=true, bool PropagateMove=true, bool PropagateSwap=true>
  class node_storage_tester
    : public maths::graph_impl::node_storage<WeightMaker, node_storage_traits<PropagateCopy, PropagateMove, PropagateSwap>>
  {
  private:
    using base_t = maths::graph_impl::node_storage<WeightMaker, node_storage_traits<PropagateCopy, PropagateMove, PropagateSwap>>;

  public:
    using allocator_type = typename base_t::node_weight_container_type::allocator_type;
    using size_type      = typename base_t::size_type;
    using weight_type    = typename base_t::weight_type;

    node_storage_tester() = default;

    explicit node_storage_tester(const allocator_type& allocator)
      : base_t(allocator)
    {}

    explicit node_storage_tester(const size_type n)
      : base_t(n)
    {}

    node_storage_tester(const size_type n, const allocator_type& allocator)
      : base_t(n, allocator)
    {}

    node_storage_tester(std::initializer_list<weight_type> weights)
      : base_t{weights}
    {}

    node_storage_tester(std::initializer_list<weight_type> weights, const allocator_type& allocator)
      : base_t{weights, allocator}
    {}

    node_storage_tester(const node_storage_tester&) = default;

    node_storage_tester(const node_storage_tester& s, const allocator_type& allocator)
      : base_t{s, allocator}
    {}

    node_storage_tester(node_storage_tester&&) noexcept = default;

    node_storage_tester(node_storage_tester&& s, const allocator_type& allocator)
      : base_t{std::move(s), allocator}
    {}

    ~node_storage_tester() = default;

    node_storage_tester& operator=(const node_storage_tester&) = default;

    node_storage_tester& operator=(node_storage_tester&&) = default;

    friend void swap(node_storage_tester& lhs, node_storage_tester& rhs)
    {
      lhs.swap(rhs);
    }

    using base_t::reserve;
    using base_t::capacity;
    using base_t::shrink_to_fit;
    using base_t::add_node;
    using base_t::insert_node;
    using base_t::erase_node;
    using base_t::erase_nodes;
    using base_t::clear;
    using base_t::get_node_allocator;
  };

  template<class WeightMaker, std::size_t N>
  class static_node_storage_tester : public maths::graph_impl::static_node_storage<WeightMaker, N>
  {
  public:
    using maths::graph_impl::static_node_storage<WeightMaker, N>::static_node_storage;
  };


  template<class WeightMaker, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  struct value_tester<node_storage_tester<WeightMaker, PropagateCopy, PropagateMove, PropagateSwap>>
    : public value_tester<maths::graph_impl::node_storage<WeightMaker, node_storage_traits<PropagateCopy, PropagateMove, PropagateSwap>>>
  {
  };

  template<class WeightMaker, std::size_t N>
  struct value_tester<static_node_storage_tester<WeightMaker, N>>
    : public value_tester<maths::graph_impl::static_node_storage<WeightMaker, N>>
  {
  };
}
