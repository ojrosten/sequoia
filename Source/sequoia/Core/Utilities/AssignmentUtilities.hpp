////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Helper for dealing with allocator propagation during copy assignment.

    Consider a type which wraps one or containers. If the enclosing type is
    allocator-aware, getting copy assigment right in situations were it cannot
    be defaulted is non-trivial. This file defines the class sequoia::assignment_helper
    to help with this.
 */

#include "sequoia/Core/Meta/Utilities.hpp"

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
    template<std::invocable<T> AllocGetter>
    struct mapper
    {
      using type = std::invoke_result_t<AllocGetter, T>;
    };
  };
}

namespace sequoia
{
  /*! \brief Helper class to assist with copy assignment for allocator-aware types.
  
      Consider a type, `T`, which is allocator-aware and for which copy assignment cannot
      be defaulted. Suppose that `T` wraps containers `Cs...`. To utilize the this helper,
      do the following:
        -# Ensure assignment_helper is befriended by `T`.
        -# Furnish `T` with a `reset` method, the arguments of which are the allocators associated with `Cs...`.
           Internally, `reset` should construct instances `Cs...` using the allocators and copy assign these
           to the wrapped containers. It is important that copy assignment is used since allocator propagation
           may differ between copying and moving.
        -# Utilize assignment_helper to define copy assignment. A single allocator example is provided
           by sequoia::data_structures::bucketed_storage and a two allocator example is provided by 
           sequoia::data_structures::partitioned_sequence_base.
           The basic idea is that assignment_helper::assign takes references to two instances of `T`,
           together with a sequence of function objects which return the requisite allocators.

      This pattern has an extra layer of generality, an example of which may be seen in sequoia::maths::connectivity.
      Depending on how its template arguments are chosen, sequoia::maths::connectivity may wrap either one or two
      allocator-aware types. To deal with these situations homogeneously, assignment_helper::assign ignores
      any of the supplied function objects if they return void.
  */

  struct assignment_helper
  {
    /*! Can be used to implement non-defaultable copy assignment for allocator-aware classes */
    template<class T, std::invocable<T>... AllocGetters>
    constexpr static void assign(T& to, const T& from, [[maybe_unused]] AllocGetters... allocGetters)
    {
      impl::invoke_filtered<void, impl::type_to_type<T>::template mapper>([&to, &from](auto... filteredAllocGetters){ assign_filtered(to, from, filteredAllocGetters...); }, allocGetters...);
    }

  private:
    template<class T, std::invocable<T>... FilteredAllocGetters>
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

    template<class T, std::invocable<T> AllocGetter>
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

    template<class T, std::invocable<T> AllocGetter, std::invocable<T>... AllocGetters>
    constexpr static bool consistency() noexcept
    {
      if constexpr(sizeof...(AllocGetters) > 0)
      {
        return (consistent<T, AllocGetter, AllocGetters>() && ...);
      }

      return true;
    }

    template<class T, std::invocable<T> AllocGetterL, std::invocable<T> AllocGetterR>
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