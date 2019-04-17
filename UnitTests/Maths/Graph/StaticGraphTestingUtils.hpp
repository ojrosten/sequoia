////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtils.hpp"

namespace sequoia::unit_testing
{
  template
  <
    directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct details_checker<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
    using type = maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>;

    template<class Logger>
    static void check(Logger& logger, const type& graph, const type& prediction, std::string_view description)
    {
      using connectivity_t = typename type::connectivity_type;

      check_equality(logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction), description);

      // TO DO: nodes
    }    
  };

  template
  <
    directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct details_checker<maths::embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
    using type = maths::embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>;

    template<class Logger>
    static void check(Logger& logger, const type& graph, const type& prediction, std::string_view description)
    {
      using connectivity_t = typename type::connectivity_type;

      check_equality(logger, static_cast<const connectivity_t&>(graph), static_cast<const connectivity_t&>(prediction), description);

      // TO DO: nodes
    }    
  };

  template
  <
    directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct equivalence_checker<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
    using type = maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>;

    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;    

    template<class Logger>
    static void check(Logger& logger, const type& graph, connectivity_equivalent_type prediction, std::string_view description)
    {
      using connectivity_t = typename type::connectivity_type;

      check_equivalence(logger, static_cast<const connectivity_t&>(graph), prediction, description);

      // TO DO: nodes
    }    
  };

  template
  <
    directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct equivalence_checker<maths::embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
    using type = maths::embedded_static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>;

    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<typename type::edge_init_type>>;    

    template<class Logger>
    static void check(Logger& logger, const type& graph, connectivity_equivalent_type prediction, std::string_view description)
    {
      using connectivity_t = typename type::connectivity_type;

      check_equivalence(logger, static_cast<const connectivity_t&>(graph), prediction, description);

      // TO DO: nodes
    }    
  };
}
