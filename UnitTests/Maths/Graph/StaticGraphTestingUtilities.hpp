////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"
#include "StaticGraph.hpp"

namespace sequoia::unit_testing
{
  // Details Checkers
  
  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct detailed_equality_checker<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
    : impl::graph_detailed_equality_checker<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {   
  };

  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct detailed_equality_checker<maths::static_embedded_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
    : impl::graph_detailed_equality_checker<maths::static_embedded_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {    
  };

  // Equivalence Checkers

  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct equivalence_checker<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
    : impl::graph_equivalence_checker<maths::static_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {
  };

  template
  <
    maths::directed_flavour Directedness,      
    std::size_t Size,
    std::size_t Order,      
    class EdgeWeight,
    class NodeWeight,
    class Traits
  >
  struct equivalence_checker<maths::static_embedded_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
    : impl::graph_equivalence_checker<maths::static_embedded_graph<Directedness, Size, Order, EdgeWeight, NodeWeight, Traits>>
  {    
  };
}
