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
  template<class X, template<class> class TypeToType>
  constexpr std::size_t count_occurances() noexcept
  {
    return 0ul;
  }

  template<class X, template<class> class TypeToType, class T, class... Ts>
  constexpr std::size_t count_occurances() noexcept
  {
    using type = typename TypeToType<T>::type;
    return std::size_t{std::is_same_v<X, type> + count_occurances<X, TypeToType, Ts...>()};
  }

  template<std::size_t I, std::size_t Encountered, class Excluded, template<class> class TypeToType, class T, class... Ts>
  constexpr std::size_t get_filter_index() noexcept
  {
    auto getter{
      []() {
        using type = typename TypeToType<T>::type;
        if constexpr((!std::is_same_v<Excluded, type> && (Encountered == I)) || !sizeof...(Ts))
        {
          return 0ul;
        }
        else
        {
          constexpr auto count{std::is_same_v<Excluded, type> ? Encountered : Encountered + 1};

          return 1ul + get_filter_index<I, count, Excluded, TypeToType, Ts...>();
        }
      }
    };

    return getter();
  }

  template<std::size_t I, class Excluded, template<class> class TypeToType, class... Ts>
  auto get(const std::tuple<Ts...>& ts)
  {
    constexpr auto i{get_filter_index<I, 0, Excluded, TypeToType, Ts...>()};
    return std::get<i>(ts);
  }

  template<class Excluded, template<class> class TypeToType, class Fn, class... Ts, std::size_t... I>
  void filter(Fn f, std::index_sequence<I...>, Ts... t)
  {
    f(get<I, Excluded, TypeToType>(std::tuple<Ts...>(t...))...);
  }

  template<class Excluded, template<class> class TypeToType, class Fn, class... Ts>
  void filter(Fn f, Ts... t)
  {
    constexpr auto N{sizeof...(Ts) - count_occurances<Excluded, TypeToType, Ts...>()};
    filter<Excluded, TypeToType>(f, std::make_index_sequence<N>{}, t...);
  }

  template<class Container>
  struct type_to_type
  {
    template<class AllocGetter>
    struct mapper
    {
      using type = std::invoke_result_t<AllocGetter, Container>;
    };
  };

  struct assignment_helper
  {
    template<class Container, class... AllocGetters>
    constexpr static void assign(Container& to, const Container& from, [[maybe_unused]] AllocGetters... allocGetters)
    {
      filter<void, type_to_type<Container>::template mapper>([&to, &from](auto... filteredAllocGetters){ assign_filtered(to, from, filteredAllocGetters...); }, allocGetters...);
    }

  private:
    template<class Container, class... FilteredAllocGetters>
    constexpr static void assign_filtered(Container& to, const Container& from, [[maybe_unused]] FilteredAllocGetters... allocGetters)
    {
      if constexpr((naive_treatment<Container, FilteredAllocGetters>() && ...))
      {
        auto tmp{from};
        to = std::move(tmp);
      }
      else
      {
        static_assert(consistency<Container, FilteredAllocGetters...>());
        
        Container tmp{from, get_allocator(to, from, allocGetters)...};
        if constexpr (copy_propagation<Container, FilteredAllocGetters...>() == move_propagation<Container, FilteredAllocGetters...>())
        {
          to = std::move(tmp);
        }
        else
        {
          if constexpr(copy_propagation<Container, FilteredAllocGetters...>())
          {
            to = Container{allocGetters(tmp)...};
          }

          to.swap(tmp);
        }
      }
    }
    
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
