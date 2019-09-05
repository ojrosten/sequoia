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
    template<class Container, class... AllocGetters>
    constexpr static void assign(Container& to, const Container& from, [[maybe_unused]] AllocGetters... allocGetters)
    {
      if constexpr((naive_treatment<Container, AllocGetters>() && ...))
      {
        auto tmp{from};
        to = std::move(tmp);
      }
      else
      {
        static_assert(consistency<Container, AllocGetters...>());
        
        Container tmp{from, get_allocator(to, from, allocGetters)...};
        if constexpr (copy_propagation<Container, AllocGetters...>() == move_propagation<Container, AllocGetters...>())
        {
          to = std::move(tmp);
        }
        else
        {
          if constexpr(copy_propagation<Container, AllocGetters...>())
          {
            to = Container{allocGetters(tmp)...};
          }

          to.swap(tmp);
        }
      }
    }

  private:
    template<class Container, class AllocGetter>
    constexpr static bool naive_treatment() noexcept
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

    template<class Container, class AllocGetter, class... AllocGetters>
    constexpr static bool consistency() noexcept
    {
      if constexpr(sizeof...(AllocGetters) > 0)
      {
        return (consistent<Container, AllocGetter, AllocGetters>() && ...);
      }

      return true;
    }

    template<class Container, class AllocGetterL, class AllocGetterR>
    constexpr static bool consistent() noexcept
    {
      return (copy_propagation<Container, AllocGetterL>() == copy_propagation<Container, AllocGetterR>())
          && (move_propagation<Container, AllocGetterL>() == move_propagation<Container, AllocGetterR>())
          && (swap_propagation<Container, AllocGetterL>() == swap_propagation<Container, AllocGetterR>());
    }

    template<class Container, class AllocGetter>
    static auto get_allocator(const Container& to, const Container& from, AllocGetter allocGetter)
    {
      if constexpr(copy_propagation<Container, AllocGetter>())
      {
        return allocGetter(from);
      }
      else
      {
        return allocGetter(to);
      }
    }

    template<class Container, class AllocGetter, class... AllocGetters>
    constexpr static bool copy_propagation() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, Container>;
      return std::allocator_traits<allocator>::propagate_on_container_copy_assignment::value
        || std::allocator_traits<allocator>::is_always_equal::value;
    }

    template<class Container, class AllocGetter, class... AllocGetters>
    constexpr static bool move_propagation() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, Container>;
      return std::allocator_traits<allocator>::propagate_on_container_move_assignment::value
        || std::allocator_traits<allocator>::is_always_equal::value;
    }

    template<class Container, class AllocGetter, class... AllocGetters>
    constexpr static bool swap_propagation() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, Container>;
      return std::allocator_traits<allocator>::propagate_on_container_swap::value
        || std::allocator_traits<allocator>::is_always_equal::value;
    }
  };
}
