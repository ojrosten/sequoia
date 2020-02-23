////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCheckers.hpp
    \brief Utilties for performing allocation checks within the unit testing framework.
*/

#include "UnitTestCheckers.hpp"
#include "AllocationCheckersDetails.hpp"

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
      
    int copy_x{};
    individual_allocation_predictions y;
    assignment_allocation_predictions assign_y_to_x;
  };

  struct move_only_allocation_predictions
  {
    int para_move{};
  };

  template<class Container, class Allocator, class Predictions>
  class allocation_info : public impl::allocation_info_base<Container, Allocator>
  {
  private:
    using base_t = impl::allocation_info_base<Container, Allocator>;
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
      
    template<class Fn>
    allocation_info(Fn&& allocGetter, Predictions predictions)
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
  class allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>
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
    allocation_info(Fn&& allocGetter, std::initializer_list<Predictions> predictions)
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

        return allocation_info<Container, Alloc>{scopedGetter, m_Predictions[I]};
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

  template<class Fn>
  allocation_info(Fn&& allocGetter, allocation_predictions predictions)
    -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  template<class Fn>
  allocation_info(Fn&& allocGetter, std::initializer_list<allocation_predictions> predictions)
    -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  // Done through inheritance rather than a using declaration
  // in order to make use of CTAD. A shame argument deduction
  // can't be used for using declarations...
    
  template<class Container, class Allocator>
  class move_only_allocation_info
    : public allocation_info<Container, Allocator, move_only_allocation_predictions>
  {
  public:
    using allocation_info<Container, Allocator, move_only_allocation_predictions>::allocation_info;
  };

  template<class Fn>
  move_only_allocation_info(Fn&& allocGetter, move_only_allocation_predictions predictions)
    -> move_only_allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  template<class Fn>
  move_only_allocation_info(Fn&& allocGetter, std::initializer_list<move_only_allocation_predictions> predictions)
    -> move_only_allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

  template<class Logger, class T, class Mutator, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<T, Allocators>... info)
  {
    typename Logger::sentinel s{logger, add_type_info<T>(description)};
      
    if(impl::check_regular_semantics(description, logger, impl::regular_allocation_actions{}, x, y, yMutator, std::tuple_cat(impl::make_allocation_checkers(info, x, y)...)))
    {
      impl::check_para_constructor_allocations(description, logger, y, yMutator, info...);
    }
  }

  template<class Logger, class T, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, T&& x, T&& y, const T& xClone, const T& yClone, move_only_allocation_info<T, Allocators>... info)
  {
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    if(impl::check_regular_semantics(description, logger, impl::move_only_allocation_actions{}, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::tuple_cat(impl::make_allocation_checkers(info, x, y)...)))
    {
      impl::check_para_constructor_allocations(description, logger, std::forward<T>(y), yClone, info...);
    }
  }
}
