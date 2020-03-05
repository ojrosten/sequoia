////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file AllocationCheckers.hpp
    \brief Utilties for performing allocation checks within the unit testing framework.
*/

#include "UnitTestCheckers.hpp"
#include "AllocationCheckersDetails.hpp"

#include "ArrayUtilities.hpp"
#include "Utilities.hpp"

namespace sequoia::unit_testing
{
  struct individual_allocation_predictions
  {
    individual_allocation_predictions(int copyPrediction, int mutationPrediction)
      : copy{copyPrediction}          
      , mutation{mutationPrediction}
      , para_copy{copyPrediction}
      , para_move{copyPrediction}
    {}

    individual_allocation_predictions(int copyPrediction, int mutationPrediction, int copyLikePrediction)
      : copy{copyPrediction}          
      , mutation{mutationPrediction}
      , para_copy{copyLikePrediction}
      , para_move{copyPrediction}
    {}
      
    individual_allocation_predictions(int copyPrediction, int mutationPrediction, int copyLikePrediction, int moveLikePrediction)
      : copy{copyPrediction}          
      , mutation{mutationPrediction}
      , para_copy{copyLikePrediction}
      , para_move{moveLikePrediction}
    {}
      
    int copy{}, mutation{}, para_copy{}, para_move{};
  };

  struct assignment_allocation_predictions
  {
    assignment_allocation_predictions(int withPropagation, int withoutPropagation)
      : with_propagation{withPropagation}, without_propagation{withoutPropagation}
    {}
      
    int with_propagation{}, without_propagation{};
  };
 
  struct allocation_predictions
  {
    allocation_predictions(int copyX, individual_allocation_predictions yPredictions, assignment_allocation_predictions assignYtoX)
      : copy_x{copyX}, y{yPredictions}, assign_y_to_x{assignYtoX}
    {}

    [[nodiscard]]
    int para_move_allocs() const noexcept
    {
      return y.para_move;
    }

    [[nodiscard]]
    int assign_without_propagation_allocs() const noexcept
    {
      return assign_y_to_x.without_propagation;
    }

    [[nodiscard]]
    int mutation_allocs() const noexcept { return y.mutation; }

    int copy_x{};
    individual_allocation_predictions y;
    assignment_allocation_predictions assign_y_to_x;
  };

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
    const allocation_predictions& get_predictions() const noexcept
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

  // Done through inheritance rather than a using declaration
  // in order to make use of CTAD. Should be able to revert
  // to using in C++20...

  template<class Container, class Allocator>
  class allocation_info
    : public basic_allocation_info<Container, Allocator, allocation_predictions>
  {
  public:
    using basic_allocation_info<Container, Allocator, allocation_predictions>::basic_allocation_info;
  };

  template<class Fn>
  allocation_info(Fn&& allocGetter, allocation_predictions predictions)
    -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  template<class Fn>
  allocation_info(Fn&& allocGetter, std::initializer_list<allocation_predictions> predictions)
    -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;
    
  template<class Logger, class T, class Mutator, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<T, Allocators>... info)
  {
    typename Logger::sentinel s{logger, add_type_info<T>(description)};
      
    if(impl::check_regular_semantics(description, logger, impl::regular_allocation_actions{}, x, y, yMutator, std::tuple_cat(impl::make_allocation_checkers(info, x, y)...)))
    {
      impl::check_para_constructor_allocations(description, logger, y, yMutator, info...);
    }
  }
}
