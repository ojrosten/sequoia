////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "NodeStorage.hpp"
#include "StaticNodeStorage.hpp"

namespace sequoia::unit_testing
{
  template<class WeightMaker, class Traits>
  struct details_checker<maths::graph_impl::node_storage<WeightMaker, Traits, false>>
  {
    using type = maths::graph_impl::node_storage<WeightMaker, Traits>;

    template<class Logger>
    static void check(Logger& logger, const type& nodes, const type& prediction, std::string_view description)
    {
      check_equality(logger, nodes.size(), prediction.size(), impl::concat_messages(description, "Sizes different"));

      check_range(logger, nodes.cbegin_node_weights(), nodes.cend_node_weights(), prediction.cbegin_node_weights(), prediction.cend_node_weights(), impl::concat_messages(description, "const_node_iter"));

      check_range(logger, nodes.crbegin_node_weights(), nodes.crend_node_weights(), prediction.crbegin_node_weights(), prediction.crend_node_weights(), impl::concat_messages(description, "const_reverse_node_iter"));
    }
  };

  template<class WeightMaker, class Traits>
  struct details_checker<maths::graph_impl::node_storage<WeightMaker, Traits, true>>
  {
    using type = maths::graph_impl::node_storage<WeightMaker, Traits>;

    template<class Logger>
    static void check(Logger& logger, const type& nodes, const type& prediction, std::string_view description)
    {
    }
  };

  template<class WeightMaker, class Traits>
  struct equivalence_checker<maths::graph_impl::node_storage<WeightMaker, Traits, false>>
  {
    using type = maths::graph_impl::node_storage<WeightMaker, Traits>;

    using equivalent_type = std::initializer_list<typename type::weight_type>;

    template<class Logger>
    static void check(Logger& logger, const type& nodes, equivalent_type prediction, std::string_view description)
    {
      check_equality(logger, nodes.size(), prediction.size(), impl::concat_messages(description, "Sizes different"));

      check_range(logger, nodes.cbegin_node_weights(), nodes.cend_node_weights(), prediction.begin(), prediction.end(), impl::concat_messages(description, "const_node_iter"));
    }
  };

  template<class WeightMaker, class Traits>
  struct equivalence_checker<maths::graph_impl::node_storage<WeightMaker, Traits, true>>
  {
    using type = maths::graph_impl::node_storage<WeightMaker, Traits>;

    using equivalent_type = std::initializer_list<typename type::weight_type>;

    template<class Logger>
    static void check(Logger& logger, const type& nodes, equivalent_type prediction, std::string_view description)
    {
      check(logger, !prediction.size(), impl::concat_messages(description, "Node storage should have zero size for empty node weights"));
    }
  };

  template<class WeightMaker, std::size_t N>
  struct details_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
    : public details_checker<maths::graph_impl::node_storage<WeightMaker, maths::graph_impl::static_node_storage_traits<WeightMaker, N>>>
  {
  };

  template<class WeightMaker, std::size_t N>
  struct equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
    : public equivalence_checker<maths::graph_impl::node_storage<WeightMaker, maths::graph_impl::static_node_storage_traits<WeightMaker, N>>>
  {
  };

  
  struct node_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{};
    template<class S> using container_type = std::vector<S, std::allocator<S>>;
  };

  template<class WeightMaker>
  class node_storage_tester : public maths::graph_impl::node_storage<WeightMaker, node_storage_traits>
  {
  public:
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::node_storage;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::reserve;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::capacity;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::shrink_to_fit;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::add_node;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::insert_node;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::erase_node;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::erase_nodes;
    using maths::graph_impl::node_storage<WeightMaker, node_storage_traits>::clear;
  };

  template<class WeightMaker, std::size_t N>
  class static_node_storage_tester : public maths::graph_impl::static_node_storage<WeightMaker, N>
  {
  public:
    using maths::graph_impl::static_node_storage<WeightMaker, N>::static_node_storage;
  };

  
  template<class WeightMaker>
  struct details_checker<node_storage_tester<WeightMaker>>
    : public details_checker<maths::graph_impl::node_storage<WeightMaker, node_storage_traits>>
  {
  };

  template<class WeightMaker>
  struct equivalence_checker<node_storage_tester<WeightMaker>>
    : public equivalence_checker<maths::graph_impl::node_storage<WeightMaker, node_storage_traits>>
  {
  };

  template<class WeightMaker, std::size_t N>
  struct details_checker<static_node_storage_tester<WeightMaker, N>>
    : public details_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
  {
  };

  template<class WeightMaker, std::size_t N>
  struct equivalence_checker<static_node_storage_tester<WeightMaker, N>>
    : public equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
  {
  };
}
