////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for performing allocation checks within the unit testing framework.

    The allocation testing framework is built upon the idea of supplying and testing
    predictions for particular allocation events, in particular:

    - copying
    - mutating
    - para copy (use of the copy-like constructor taking an allocator as an extra argument)
    - para move (use of the move-like constructor taking an allocator as an extra argument)
    - assignment with/without propagation of the allocator
    - swap with/without propagation of the allocator

    To help with readability of actual tests, various user defined literals are introduced
    so that, for example, 1_c may be used to mean a prediction of 1 allocation for a copy
    event. This is more expressive than just '1' and less verbose than copy_prediction{1},
    particularly bearing in mind that often several predictions are supplied together.

    In addition to predictions, clients must also supply a function object, per allocator
    which consumes a container and returns a copy of the allocator. With these ingredients,
    together with a container which uses the shared_counting_allocator, the following
    scenario may be realized:

    A extract a copy of the allocator
    B acquire the number of allocations
    C perform an operation with an expected number of allocations
    D acquire the number of allocations
    E compare the prediction to the difference between D and B.

    Note that the framework is designed to work smoothly with std::scoped_allocator_adaptor
    and/or with containers containing multiple allocators, scoped or otherwise.
*/

#include "FreeCheckers.hpp"
#include "AllocationCheckersDetails.hpp"

#include "ArrayUtilities.hpp"
#include "Utilities.hpp"

namespace sequoia::unit_testing
{
  enum class allocation_event { copy, mutation, para_copy, para_move, assign_prop, assign};

  /*! Type-safe wrapper for allocation predictions, to avoid mixing different allocation events */
  template<allocation_event Event>
  class alloc_prediction
  {
  public:
    constexpr explicit alloc_prediction(int n = 0) noexcept : m_Num{n} {}

    [[nodiscard]]
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
  operator "" _pm(unsigned long long int n) noexcept
  {
    return para_move_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  constexpr assign_prediction
  operator "" _anp(unsigned long long int n) noexcept
  {
    return assign_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  constexpr assign_prop_prediction
  operator "" _awp(unsigned long long int n) noexcept
  {
    return assign_prop_prediction{static_cast<int>(n)};
  }

  /*! \brief Base class for use with both plain (shared counting) allocators and std::scoped_allocator_adaptor

      This class wraps one of the essential ingredients for allocation testing: a function
      object which can acquire a copy of a container's allocator. Note that a container with
      multiple independent allocators will generally be associated with multiple instantiations
      of this class. (Each of these independent allocators may be scoped.)

      The class is designed for inheritance but not for the purpose of type erasure and so
      has a protected destructor etc.
   */
  template<class Container, class Allocator>
  class allocation_info_base
  {
  public:
    template<class Fn>
    explicit allocation_info_base(Fn&& allocGetter)
      : m_AllocatorGetter{std::forward<Fn>(allocGetter)}
    {
      if(!m_AllocatorGetter)
        throw std::logic_error("allocation_info must be supplied with a non-null function object");
    }

    allocation_info_base(const allocation_info_base&) = default;

    [[nodiscard]]
    int count(const Container& c) const noexcept
    {        
      return m_AllocatorGetter(c).allocs();
    }

    template<class... Args>
    [[nodiscard]]
    Allocator make_allocator(Args&&... args) const
    {
      return Allocator{std::forward<Args>(args)...};
    }

    [[nodiscard]]
    Allocator allocator(const Container& c) const
    {
      return m_AllocatorGetter(c);
    }
  protected:
    ~allocation_info_base() = default;

    allocation_info_base(allocation_info_base&&) noexcept = default;

    allocation_info_base& operator=(const allocation_info_base&)     = default;
    allocation_info_base& operator=(allocation_info_base&&) noexcept = default;

    using getter = std::function<Allocator(const Container&)>;

    [[nodiscard]]
    getter make_getter() const
    {
      return m_AllocatorGetter;
    }
  private:

    getter m_AllocatorGetter;
  };

  /*! \brief Class for use with a container possessing a (shared counting) allocator

      By inheriting from allocation_info_base, this class is able to acquire a copy
      of an allocator from a container. On top of this, the class holds predictions
      for the various allocation events.
   */
  template<class Container, class Allocator, class Predictions>
  class basic_allocation_info : public allocation_info_base<Container, Allocator>
  {
  private:
    using base_t = allocation_info_base<Container, Allocator>;
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

  /*! \brief A specialization of basic_allocation_info appropriate for std::scoped_allocator_adaptor

      The essential difference to the primary template is that multiple sets of predictions must
      be supplied, one for each level within the scoped_allocator_adaptor.
   */
  template<class Container, class... Allocators, class Predictions>
  class basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>
    : public allocation_info_base<Container, std::scoped_allocator_adaptor<Allocators...>>
  {
  private:
    using base_t = allocation_info_base<Container, std::scoped_allocator_adaptor<Allocators...>>;
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

    /// unpacks the scoped_allocator_adaptor, returning basic_allocation_info for the
    /// allocator at the ith level.
    template<std::size_t I>
    [[nodiscard]]
    decltype(auto) unpack() const
    {
      auto scopedGetter{[getter=this->make_getter()](const Container& c){
          return get<I>(getter(c));
        }
      };

      using Alloc = decltype(scopedGetter(std::declval<Container>()));

      return basic_allocation_info<Container, Alloc, Predictions>{scopedGetter, m_Predictions[I]};
    }

  private:
    template<std::size_t I, class... As>
    [[nodiscard]]
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
