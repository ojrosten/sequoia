////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file AllocationCheckers.hpp
    \brief Utilities for performing allocation checks within the unit testing framework.
*/

#include "FreeCheckers.hpp"
#include "AllocationCheckersDetails.hpp"

#include "ArrayUtilities.hpp"
#include "Utilities.hpp"

namespace sequoia::unit_testing
{
  enum class allocation_event { copy, mutation, para_copy, para_move, assign_prop, assign};

  template<allocation_event Event>
  class alloc_prediction
  {
  public:
    constexpr /*explicit*/ alloc_prediction(int n = 0) : m_Num{n} {}

    constexpr operator int() const noexcept
    {
      return m_Num;
    }
  private:
    int m_Num{};
  };

  using copy_prediction        = alloc_prediction<allocation_event::copy>;
  using mutation_prediction    = alloc_prediction<allocation_event::mutation>;
  using para_copy_prediction   = alloc_prediction<allocation_event::para_copy>;
  using para_move_prediction   = alloc_prediction<allocation_event::para_move>;
  using assign_prop_prediction = alloc_prediction<allocation_event::assign_prop>;
  using assign_prediction      = alloc_prediction<allocation_event::assign>;

  [[nodiscard]]
  constexpr copy_prediction
  operator "" _c(unsigned long long int n) noexcept
  {
    return copy_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  constexpr mutation_prediction
  operator "" _mu(unsigned long long int n) noexcept
  {
    return mutation_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  constexpr para_copy_prediction
  operator "" _pc(unsigned long long int n) noexcept
  {
    return para_copy_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  constexpr para_move_prediction
  operator "" _py(unsigned long long int n) noexcept
  {
    return para_move_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  constexpr assign_prediction
  operator "" _as(unsigned long long int n) noexcept
  {
    return assign_prediction{static_cast<int>(n)};
  }
  
  template<class Container, class Allocator, class Predictions>
  class basic_allocation_info : public impl::allocation_info_base<Container, Allocator>
  {
  private:
    using base_t = impl::allocation_info_base<Container, Allocator>;
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
      
    template<class Fn>
    basic_allocation_info(Fn&& allocGetter, Predictions predictions)
      : base_t{std::forward<Fn>(allocGetter)}
      , m_Predictions{std::move(predictions)}
    {}

    [[nodiscard]]
    const Predictions& get_predictions() const noexcept
    {
      return m_Predictions;
    }
  private:
    Predictions m_Predictions;
  };

  template<class Container, class... Allocators, class Predictions>
  class basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>
    : public impl::allocation_info_base<Container, std::scoped_allocator_adaptor<Allocators...>>
  {
  private:
    using base_t = impl::allocation_info_base<Container, std::scoped_allocator_adaptor<Allocators...>>;
  public:
    constexpr static auto N{sizeof...(Allocators)};

    static_assert(N > 0);

    using container_type   = Container;
    using allocator_type   = std::scoped_allocator_adaptor<Allocators...>;
    using predictions_type = Predictions;

    template<class Fn>
    basic_allocation_info(Fn&& allocGetter, std::initializer_list<Predictions> predictions)
      : base_t{std::forward<Fn>(allocGetter)}
      , m_Predictions{utilities::to_array<Predictions, N>(predictions)}
    {}

    template<std::size_t I> decltype(auto) unpack() const
    {
      if constexpr(I==0)
      {
        return *this;
      }
      else
      {
        auto scopedGetter{[getter=this->make_getter()](const Container& c){
            return get<I>(getter(c));
          }
        };

        using Alloc = decltype(scopedGetter(std::declval<Container>()));

        return basic_allocation_info<Container, Alloc, Predictions>{scopedGetter, m_Predictions[I]};
      }
    }

    [[nodiscard]]
    const Predictions& get_predictions() const noexcept
    {
      return m_Predictions[0];
    }
  private:
    template<std::size_t I, class... As>
    static auto get(const std::scoped_allocator_adaptor<As...>& alloc)
    {
      if constexpr(I==0)
      {
        return alloc.outer_allocator();
      }
      else
      {
        return get<I-1>(alloc.inner_allocator());
      }
    }
      
    std::array<Predictions, N> m_Predictions;
  };
}
