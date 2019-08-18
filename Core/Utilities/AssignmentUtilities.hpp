////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file AssignmentUtilities.hpp
    \brief Helper for dealing with allocator propagation during copy assignment.
 */

#include <memory>

namespace sequoia::impl
{
  template<bool DefaultCopyable,class Container>
  void assign(Container& to, const Container& from)
  {
    if constexpr(!has_allocator_type_v<Container>)
    {
      auto tmp{from};
      to = std::move(tmp);
    }
    else
    {
      using allocator = typename Container::allocator_type;

      constexpr bool copyPropagation{
        std::allocator_traits<allocator>::propagate_on_container_copy_assignment::value
      };

      constexpr bool alwaysEqual{
        std::allocator_traits<allocator>::is_always_equal::value
      };

      if constexpr(DefaultCopyable && (copyPropagation || alwaysEqual))
      {
        to = from;
      }
      else
      {
        constexpr bool movePropagation{
          std::allocator_traits<allocator>::propagate_on_container_move_assignment::value
        };

        constexpr bool copyConsistentWithMove{alwaysEqual || (copyPropagation == movePropagation)};

        auto getAlloc{
          [](const Container& to, const Container& from){           
            if constexpr(copyPropagation)
            {
              return from.get_allocator();
            }
            else
            {
              return to.get_allocator();
            }
          }
        };
        
        Container tmp{from, getAlloc(to, from)};
        if constexpr (copyConsistentWithMove)
        {
          to = std::move(tmp);
        }
        else
        {
          if constexpr(copyPropagation)
            to = Container{tmp.get_allocator()};

          to.swap(tmp);
        }
      }
    }
  }
}
