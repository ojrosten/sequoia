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
  struct assignment_helper
  {    
    template<class Container, class AllocGetter>
    constexpr static void assign(Container& to, const Container& from, [[maybe_unused]] AllocGetter allocGetter)
    {
      if constexpr(naive_treatment<Container, AllocGetter>())
      {
        auto tmp{from};
        to = std::move(tmp);
      }
      else
      {
        using allocator = std::invoke_result_t<AllocGetter, Container>;

        constexpr bool copyPropagation{
          std::allocator_traits<allocator>::propagate_on_container_copy_assignment::value
        };

        constexpr bool movePropagation{
          std::allocator_traits<allocator>::propagate_on_container_move_assignment::value
        };

        constexpr bool copyConsistentWithMove{copyPropagation == movePropagation};

        auto getAlloc{
          [allocGetter](const Container& to, const Container& from){           
            if constexpr(copyPropagation)
            {
              return allocGetter(from);
            }
            else
            {
              return allocGetter(to);
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
          {
            to = Container{allocGetter(tmp)};
          }

          to.swap(tmp);
        }
      }
    }

  private:
    template<class Container, class AllocGetter>
    constexpr static bool naive_treatment()
    {
      using allocator = std::invoke_result_t<AllocGetter, Container>;
      if constexpr(std::is_void_v<allocator>)
      {
        return true;
      }
      else
      {
        return std::allocator_traits<allocator>::is_always_equal::value;
      }
    }
  };
}
