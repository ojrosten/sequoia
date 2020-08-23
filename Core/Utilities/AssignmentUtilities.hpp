////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Helper for dealing with allocator propagation during copy assignment.
 */

#include "Utilities.hpp"

#include <memory>

namespace sequoia::impl
{
  template<class Excluded, template<class> class TypeToType, class Fn, class... Ts>
  void invoke_filtered(Fn f, Ts... t)
  {
    invoke_with_specified_args(f, make_filtered_sequence<Excluded, TypeToType, Ts...>{}, t...);
  }
  
  template<class T>
  struct type_to_type
  {
    template<invocable<T> AllocGetter>
    struct mapper
    {
      using type = std::invoke_result_t<AllocGetter, T>;
    };
  };

  struct assignment_helper
  {
    template<class T, invocable<T>... AllocGetters>
    constexpr static void assign(T& to, const T& from, [[maybe_unused]] AllocGetters... allocGetters)
    {
      invoke_filtered<void, type_to_type<T>::template mapper>([&to, &from](auto... filteredAllocGetters){ assign_filtered(to, from, filteredAllocGetters...); }, allocGetters...);
    }

  private:
    template<class T, invocable<T>... FilteredAllocGetters>
    constexpr static void assign_filtered(T& to, const T& from, [[maybe_unused]] FilteredAllocGetters... allocGetters)
    {
      if constexpr((naive_treatment<T, FilteredAllocGetters>() && ...))
      {
        auto tmp{from};
        to = std::move(tmp);
      }
      else
      {
        static_assert(consistency<T, FilteredAllocGetters...>());
        
        T tmp{from, get_allocator(to, from, allocGetters)...};
        constexpr bool
          copyPropagation{copy_propagation<T, FilteredAllocGetters...>()},
          movePropagation{move_propagation<T, FilteredAllocGetters...>()};
        
        if constexpr (movePropagation || !copyPropagation)
        {
          to = std::move(tmp);
        }
        else
        {
          if constexpr(!swap_propagation<T, FilteredAllocGetters...>())
          {          
            to.reset(allocGetters(tmp)...);
          }

          to.swap(tmp);
        }
      }
    }
    
    template<class T, invocable<T> AllocGetter>
    constexpr static bool naive_treatment() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, T>;
      if constexpr(std::is_void_v<allocator>)
      {
        return true;
      }
      else
      {
        return std::allocator_traits<allocator>::is_always_equal::value;
      }
    }

    template<class T, invocable<T> AllocGetter, invocable<T>... AllocGetters>
    constexpr static bool consistency() noexcept
    {
      if constexpr(sizeof...(AllocGetters) > 0)
      {
        return (consistent<T, AllocGetter, AllocGetters>() && ...);
      }

      return true;
    }

    template<class T, invocable<T> AllocGetterL, invocable<T> AllocGetterR>
    constexpr static bool consistent() noexcept
    {
      return (copy_propagation<T, AllocGetterL>() == copy_propagation<T, AllocGetterR>())
          && (move_propagation<T, AllocGetterL>() == move_propagation<T, AllocGetterR>())
          && (swap_propagation<T, AllocGetterL>() == swap_propagation<T, AllocGetterR>());
    }

    template<class T, class AllocGetter>
    static auto get_allocator(const T& to, const T& from, AllocGetter allocGetter)
    {
      if constexpr(copy_propagation<T, AllocGetter>())
      {
        return allocGetter(from);
      }
      else
      {
        return allocGetter(to);
      }
    }

    template<class T, class AllocGetter, class... AllocGetters>
    constexpr static bool copy_propagation() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, T>;
      return std::allocator_traits<allocator>::propagate_on_container_copy_assignment::value
        || std::allocator_traits<allocator>::is_always_equal::value;
    }

    template<class T, class AllocGetter, class... AllocGetters>
    constexpr static bool move_propagation() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, T>;
      return std::allocator_traits<allocator>::propagate_on_container_move_assignment::value
        || std::allocator_traits<allocator>::is_always_equal::value;
    }

    template<class T, class AllocGetter, class... AllocGetters>
    constexpr static bool swap_propagation() noexcept
    {
      using allocator = std::invoke_result_t<AllocGetter, T>;
      return std::allocator_traits<allocator>::propagate_on_container_swap::value
        || std::allocator_traits<allocator>::is_always_equal::value;
    }
  };
}
