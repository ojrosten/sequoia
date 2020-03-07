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
