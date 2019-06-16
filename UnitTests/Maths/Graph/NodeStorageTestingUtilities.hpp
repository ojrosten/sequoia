////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "NodeStorage.hpp"
#include "StaticNodeStorage.hpp"
#include "HeterogeneousNodeStorage.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Nodes, bool=std::is_empty_v<typename Nodes::weight_type>>
    struct node_detailed_equality_checker
    {
      using type = Nodes;

      template<class Logger>
      static void check(Logger& logger, const Nodes& nodes, const Nodes& prediction, std::string_view description)
      {
        check_equality(logger, nodes.size(), prediction.size(), combine_messages(description, "Sizes different"));

        check_range(logger, nodes.cbegin_node_weights(), nodes.cend_node_weights(), prediction.cbegin_node_weights(), prediction.cend_node_weights(), combine_messages(description, "const_node_iter"));

        check_range(logger, nodes.crbegin_node_weights(), nodes.crend_node_weights(), prediction.crbegin_node_weights(), prediction.crend_node_weights(), combine_messages(description, "const_reverse_node_iter"));
      }
    };

    template<class Nodes>
    struct node_detailed_equality_checker<Nodes, true>
    {
      using type = Nodes;
      
      template<class Logger>
      static void check(Logger& logger, const Nodes& nodes, const Nodes& prediction, std::string_view description)
      {
      }
    };

    template<class Nodes, bool=std::is_empty_v<typename Nodes::weight_type>>
    struct node_equivalence_checker
    {
      using type = Nodes;

      using equivalent_type = std::initializer_list<typename type::weight_type>;

      template<class Logger>
      static void check(Logger& logger, const Nodes& nodes, equivalent_type prediction, std::string_view description)
      {
        check_equality(logger, nodes.size(), prediction.size(), combine_messages(description, "Sizes different"));

        check_range(logger, nodes.cbegin_node_weights(), nodes.cend_node_weights(), prediction.begin(), prediction.end(), combine_messages(description, "const_node_iter"));
      }
    };

    template<class Nodes>
    struct node_equivalence_checker<Nodes, true>
    {
      using type = Nodes;

      using equivalent_type = std::initializer_list<typename type::weight_type>;

      template<class Logger>
      static void check(Logger& logger, const Nodes& nodes, equivalent_type prediction, std::string_view description)
      {
        unit_testing::check(logger, !prediction.size(), combine_messages(description, "Node storage should have zero size for empty node weights"));
      }
    };
  }

  // Details Checkers
  
  template<class WeightMaker, class Traits, bool IsEmpty>
  struct detailed_equality_checker<maths::graph_impl::node_storage<WeightMaker, Traits, IsEmpty>>
    : impl::node_detailed_equality_checker<maths::graph_impl::node_storage<WeightMaker, Traits, IsEmpty>>
  {   
  };

  // Equivalence Checkers

  template<class WeightMaker, class Traits, bool IsEmpty>
  struct equivalence_checker<maths::graph_impl::node_storage<WeightMaker, Traits, IsEmpty>>
    : impl::node_equivalence_checker<maths::graph_impl::node_storage<WeightMaker, Traits, IsEmpty>>
  {
  };

  // Static

  template<class WeightMaker, std::size_t N, bool IsEmpty>
  struct detailed_equality_checker<maths::graph_impl::static_node_storage<WeightMaker, N, IsEmpty>>
    : impl::node_detailed_equality_checker<maths::graph_impl::static_node_storage<WeightMaker, N, IsEmpty>>
  {
  };

  template<class WeightMaker, std::size_t N, bool IsEmpty>
  struct equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N, IsEmpty>>
    : impl::node_equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N, IsEmpty>>
  {
  };

  // Heterogeneous

  template<class... Ts>
  struct detailed_equality_checker<maths::graph_impl::heterogeneous_node_storage<Ts...>>
  {
    using type = maths::graph_impl::heterogeneous_node_storage<Ts...>;

    template<class Logger>
    static void check(Logger& logger, const type& nodes, const type& prediction, std::string_view description)
    {
      if(check_equality(logger, nodes.size(), prediction.size(), combine_messages(description, "Node storaage sizes different")))
      {
        check_elements(logger, nodes, prediction, description);
      }
    }

  private:
    template<class Logger, std::size_t I=0>
    static void check_elements(Logger& logger, const type& nodes, const type& prediction, std::string_view description)
    {
      if constexpr(I < sizeof...(Ts))
      {
        const std::string message{combine_messages(description, std::to_string(I) + "th element incorrect")};
        check_equality(logger, nodes.template node_weight<I>(), prediction.template node_weight<I>(), combine_messages(description, message));
        check_elements<Logger, I+1>(logger, nodes, prediction, description);
      }
    }
  };

  template<class... Ts>
  struct equivalence_checker<maths::graph_impl::heterogeneous_node_storage<Ts...>>
  {
    using type = maths::graph_impl::heterogeneous_node_storage<Ts...>;
    using equivalent_type = std::tuple<Ts...>;

    template<class Logger>
    static void check(Logger& logger, const type& nodes, const equivalent_type& prediction, std::string_view description)
    {
      if(check_equality(logger, nodes.size(), sizeof...(Ts), combine_messages(description, "Node storage sizes different")))
      {
        check_elements(logger, nodes, prediction, description);
      }
    }

  private:
    template<class Logger, std::size_t I=0>
    static void check_elements(Logger& logger, const type& nodes, const equivalent_type& prediction, std::string_view description)
    {
      if constexpr(I < sizeof...(Ts))
      {
        const std::string message{combine_messages(description, std::to_string(I) + "th element incorrect")};
        check_equality(logger, nodes.template node_weight<I>(), std::get<I>(prediction), combine_messages(description, message));
        check_elements<Logger, I+1>(logger, nodes, prediction, description);
      }
    }
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
  struct detailed_equality_checker<node_storage_tester<WeightMaker>>
    : public detailed_equality_checker<maths::graph_impl::node_storage<WeightMaker, node_storage_traits>>
  {
  };

  template<class WeightMaker>
  struct equivalence_checker<node_storage_tester<WeightMaker>>
    : public equivalence_checker<maths::graph_impl::node_storage<WeightMaker, node_storage_traits>>
  {
  };

  template<class WeightMaker, std::size_t N>
  struct detailed_equality_checker<static_node_storage_tester<WeightMaker, N>>
    : public detailed_equality_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
  {
  };

  template<class WeightMaker, std::size_t N>
  struct equivalence_checker<static_node_storage_tester<WeightMaker, N>>
    : public equivalence_checker<maths::graph_impl::static_node_storage<WeightMaker, N>>
  {
  };
}
