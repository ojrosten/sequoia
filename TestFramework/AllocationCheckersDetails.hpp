////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for allocation checks.
*/

#include "SemanticsCheckersDetails.hpp"
#include <scoped_allocator>

namespace sequoia::testing
{  
  template<class Container, class Allocator, class Predictions>
  class basic_allocation_info;
}

namespace sequoia::testing::impl
{
  struct allocation_advice
  {
    std::string operator()(int count, int) const;
  };
  
  template<test_mode Mode, class Container, class Allocator, class Predictions>
  static void check_allocation(std::string_view description, std::string_view detail, sentinel<Mode>& sentry, const Container& container, const basic_allocation_info<Container, Allocator, Predictions>& info, const int previous, const int prediction)
  {
    const auto current{info.count(container)};      
    auto message{sentry.generate_message(detail)};

    check_equality(std::move(message), sentry.logger(), current - previous, prediction, allocation_advice{});
  }

  /*! \brief Wraps basic_allocation_info, together with two prior allocation counts.

      Consider two containers, x and y, which in some way interact e.g. via copy/move or swap
      followed by mutation. The class holds allocation counts (for
      a given allocator) of both x and y, prior to the operation of interest being performed.
      The number of allocations after the operation may be acquired by inspecting the Container
      via the function object stored in basic_allocation_info. Combining this with the result of
      invoking either first_count() or second_count(), as appropriate, gives a number which may
      be compared to the appropriate prediction stored within basic_allocation_info.

      The main complication is dealing correctly with the various propagation traits.
   */
  template<class Container, class Allocator, class Predictions>
  class dual_allocation_checker
  {
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<Container, Allocator, Predictions>;

    dual_allocation_checker(alloc_info i, const Container& x, const Container& y)
      : m_Info{std::move(i)}
      , m_FirstCount{m_Info.count(x)}
      , m_SecondCount{m_Info.count(y)}
      , m_AllocatorsEqual{m_Info.allocator(x) == m_Info.allocator(y)}
    {}

    [[nodiscard]]
    const alloc_info& info() const noexcept
    {
      return m_Info;
    }

    [[nodiscard]]
    int first_count() const noexcept
    {
      return m_FirstCount;
    }

    [[nodiscard]]
    int second_count() const noexcept
    {
      return m_SecondCount;
    }

    template<test_mode Mode>
    void check_no_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& x, const Container& y) const
    {
      check_allocation("", append_indented(description, "Unexpected allocation detected (x)"), sentry, x, info(), first_count(), 0);
      check_allocation("", append_indented(description, "Unexpected allocation detected (y)"), sentry, y, info(), second_count(), 0);
    }

    template<test_mode Mode>
    void check_copy_assign_y_to_x(std::string_view description, sentinel<Mode>& sentry, const Container& xContainer, const Container& yContainer) const
    {
      constexpr bool propagate{std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value};
      
      int yPrediction{};
      if constexpr(propagate)
      {
        yPrediction = info().get_predictions().assign_y_to_x.with_propagation;        
        check_allocation(description, "Unexpected allocation detected for copy assignment (x)", sentry, xContainer, info(), second_count(), yPrediction);
      }
      else
      {
        const int xPrediction{info().get_predictions().assign_y_to_x.without_propagation};        
        check_allocation(description, "Unexpected allocation detected for copy assignment (x)", sentry, xContainer, info(), first_count(), xPrediction);
      }

      check_allocation(description, "Unexpected allocation detected for copy assignment (y)", sentry, yContainer, info(), second_count(), yPrediction);
    }

    template<test_mode Mode>
    void check_move_assign_y_to_x(std::string_view description, sentinel<Mode>& sentry, const Container& xContainer) const
    {
      constexpr bool propagate{std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value};

      const bool copyLike{!propagate && !m_AllocatorsEqual};

      const int xPrediction{
        copyLike ? info().get_predictions().assign_without_propagation_allocs() : 0        
      };
      
      if constexpr(propagate)
      {
        check_allocation(description, "Unexpected allocation detected for move assignment (x)", sentry, xContainer, info(), second_count(), xPrediction);
      }
      else
      {
        check_allocation(description, "Unexpected allocation detected for move assignment (x)", sentry, xContainer, info(), first_count(), xPrediction);
      }
    }

    template<test_mode Mode>
    void check_mutation_after_swap(std::string_view description, sentinel<Mode>& sentry, const Container& lhs, const Container& rhs) const
    {
      const auto prediction{info().get_predictions().mutation_allocs()};
      auto lhCount{first_count()}, rhCount{second_count()};
      
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_swap::value)
      {
        using std::swap;
        swap(lhCount, rhCount);
      }

      check_allocation(description, "Unexpected allocation detected following mutation after swap (y)", sentry, lhs, info(), lhCount, prediction);
      check_allocation(description, "Unexpected allocation detected following mutation after swap (x)", sentry, rhs, info(), rhCount, 0);
    }
  private:    
    alloc_info m_Info{};    
    int m_FirstCount{}, m_SecondCount{};
    bool m_AllocatorsEqual{};    
  };

  //================================ Deduction guide ================================//

  template<class Container, class Allocator, class Predictions>
  dual_allocation_checker(basic_allocation_info<Container, Allocator, Predictions>, const Container&, const Container&)
    -> dual_allocation_checker<Container, Allocator, Predictions>;

  /*! \brief Wraps basic_allocation_info, together with the prior allocation count.

      Consider a container, x, on which some potentially allocating operation is performed.
      Prior to this operation, an instantiation of the allocation_checker class template
      acquires the number of allocations already performed. Calling the check method after
      the operation of interest, the container is inspected via the function object stored
      in basic_allocation_info. This allows the change in the number of allocations to be
      computed, which is then compared with the prediction.
   */

  template<class Container, class Allocator, class Predictions>
  class allocation_checker
  {
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<Container, Allocator, Predictions>;
    
    allocation_checker(alloc_info i, const int priorCount=0)
      : m_Info{std::move(i)}
      , m_PriorCount{priorCount}
    {}

    allocation_checker(alloc_info i, const Container& x)
      : m_Info{std::move(i)}
      , m_PriorCount{m_Info.count(x)}
    {}

    template<test_mode Mode>
    void check(std::string_view description, std::string_view detail, sentinel<Mode>& sentry, const Container& container, const int prediction) const
    {
      check_allocation(description, detail, sentry, container, info(), m_PriorCount, prediction);
    }

    [[nodiscard]]
    const alloc_info& info() const noexcept
    {
      return m_Info;
    }
  private:
    alloc_info m_Info{};
    int m_PriorCount{};
  };

  //================================ Deduction guides ================================//

  template<class Container, class Allocator, class Predictions>
  allocation_checker(basic_allocation_info<Container, Allocator, Predictions>, const Container&)
    -> allocation_checker<Container, Allocator, Predictions>;

  template<class Container, class Allocator, class Predictions>
  allocation_checker(basic_allocation_info<Container, Allocator, Predictions>, int)
    -> allocation_checker<Container, Allocator, Predictions>;

  //================================ Specializations of do_swap ================================//
  
  template<class T, class... Allocators, class... Predictions>
  struct do_swap<dual_allocation_checker<T, Allocators, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
        || std::allocator_traits<Allocators>::is_always_equal::value) && ...)
    };
  };

  template<class T, class... Allocators, class... Predictions>
  struct do_swap<allocation_checker<T, Allocators, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
        || std::allocator_traits<Allocators>::is_always_equal::value) && ...)
    };
  };

  //================================ make_scoped_allocation_checkers ================================//

  template
  <
    template<class, class, class> class Checker,
    class Container,
    class Predictions,
    class... Allocators,
    class... Args,
    std::size_t... I
  >
  [[nodiscard]]
  auto make_scoped_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, std::index_sequence<I...>, Args&&... args)
  {
    return std::make_tuple(Checker{info.template unpack<I>(), std::forward<Args>(args)...}...);
  }

  template
  <
    template<class, class, class> class Checker,
    class Container,
    class Predictions,
    class... Allocators,
    class... Args
  >
  [[nodiscard]]
  auto make_scoped_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_scoped_allocation_checkers<Checker>(info, std::make_index_sequence<sizeof...(Allocators)>{}, std::forward<Args>(args)...);
  }

  //================================ make_dual_allocation_checkers ================================//

  template<class Container, class Predictions, class... Args, class... Allocators>
  [[nodiscard]]
  auto make_dual_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_scoped_allocation_checkers<dual_allocation_checker>(info, std::forward<Args>(args)...);
  }

  template<class Container, class Allocator, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<dual_allocation_checker<Container, Allocator, Predictions>>
  make_dual_allocation_checkers(const basic_allocation_info<Container, Allocator, Predictions>& info, Args&&... args)
  {
    using checker = dual_allocation_checker<Container, Allocator, Predictions>;
    return {checker{info, std::forward<Args>(args)...}};
  }

  //================================ make_allocation_checkers ================================//

  template<class Container, class Predictions, class... Args, class... Allocators>
  [[nodiscard]]
  auto make_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_scoped_allocation_checkers<allocation_checker>(info, std::forward<Args>(args)...);
  }

  template<class Container, class Allocator, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<allocation_checker<Container, Allocator, Predictions>>
  make_allocation_checkers(const basic_allocation_info<Container, Allocator, Predictions>& info, Args&&... args)
  {
    using checker = allocation_checker<Container, Allocator, Predictions>;
    return {checker{info, std::forward<Args>(args)...}};
  }

  //================================ Variadic Allocation Checking ================================//

  template<test_mode Mode, class CheckFn, class Checker, class... Checkers>
  void check_allocation(std::string_view description, sentinel<Mode>& sentry, CheckFn check, const Checker& checker, const Checkers&... moreCheckers)
  {
    check(description, checker);

    if constexpr (sizeof...(Checkers) > 0)
    {
      check_allocation(description, sentry, check, moreCheckers...); 
    }
  }

  //================================ checks using dual_allocation_checker ================================//

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_no_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &x, &y](std::string_view message, const auto& checker){
        checker.check_no_allocation(message, sentry, x, y);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }
      
  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_assign_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& xContainer, const Container& yContainer, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &xContainer, &yContainer](std::string_view message, const auto& checker){
        checker.check_copy_assign_y_to_x(message, sentry, xContainer, yContainer);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_move_assign_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& xContainer, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &xContainer](std::string_view message, const auto& checker){
        checker.check_move_assign_y_to_x(message, sentry, xContainer);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& xContainer, const Container& yContainer, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &xContainer, &yContainer](std::string_view message, const auto& checker){
        checker.check_mutation_after_swap(message, sentry, xContainer, yContainer);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_mutation_after_swap(std::string_view description, sentinel<Mode>& sentry, Container& lhs, const Container& rhs, const Container& y, Mutator yMutator, dual_allocation_checker<Container, Allocators, Predictions>... checkers)
  {
    if(check(sentry.generate_message("Mutation after swap pre-condition violated"), sentry.logger(), lhs == y))
    {    
      yMutator(lhs);
      check_mutation_allocation(sentry.generate_message("Unexpected allocation detected following mutation after swap"), sentry, lhs, rhs, checkers...);

      check(sentry.generate_message("Mutation is not doing anything following copy then swap"), sentry.logger(), lhs != y);
    }
  }

  //================================ checks using allocation_checker ================================//

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_x_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &container](std::string_view message, const auto& checker){
        const auto prediction{checker.info().get_predictions().copy_x};
        checker.check(message, "Unexpected allocation detected for copy construction (x)", sentry, container, prediction);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_y_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &container](std::string_view message, const auto& checker){
        const auto prediction{checker.info().get_predictions().y.copy};
        checker.check(message, "Unexpected allocation detected for copy construction (y)", sentry, container, prediction);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_move_y_allocation(sentinel<Mode>& sentry, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &container](std::string_view message, const auto& checker){
        checker.check(message, "Unexpected allocation detected for move construction (y)", sentry, container, 0);
      }
    };

    check_allocation("", sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& yContainer, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &yContainer](std::string_view message, const auto& checker){
        const auto prediction{checker.info().get_predictions().mutation_allocs()};
        checker.check(message, "", sentry, yContainer, prediction);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_para_copy_y_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &container](std::string_view message, const auto& checker){
        const auto prediction{checker.info().get_predictions().y.para_copy};
        checker.check(message, "Unexpected allocation detected for para-copy construction (y)", sentry, container, prediction);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }
  
  template<test_mode Mode, class Container, class... Allocators, class... Predictions>
  void check_para_copy_y_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& container, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&sentry,&container](auto&&... checkers){
        check_para_copy_y_allocation(description, sentry, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, checkers);
  }

  template<test_mode Mode, class Container, class Prediction, class Allocator, class... Allocators, class... Predictions>
  void check_para_move_y_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&sentry, &container](std::string_view message, const auto& checker){
        const auto prediction{checker.info().get_predictions().para_move_allocs()};
        checker.check(message, "Unexpected allocation detected for para-move construction (y)", sentry, container, prediction);
      }
    };

    check_allocation(description, sentry, checkFn, checker, moreCheckers...);
  }
  
  template<test_mode Mode, class Container, class... Allocators, class... Predictions>
  void check_para_move_y_allocation(std::string_view description, sentinel<Mode>& sentry, const Container& container, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&sentry,&container](auto&&... checkers){
        check_para_move_y_allocation(description, sentry, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, checkers);
  }

  
  /*! \brief actions common to both move-only and regular types. */
  struct allocation_actions : pre_condition_actions
  {
    constexpr static bool has_post_equality_action{true};
    constexpr static bool has_post_nequality_action{true};
    constexpr static bool has_post_move_action{true};
    constexpr static bool has_post_move_assign_action{true};
    constexpr static bool has_post_swap_action{true};

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_equality_action(std::string_view description, sentinel<Mode>& sentry, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_no_allocation("Unexpected allocation detected for operator==", sentry, x, y, checkers...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_nequality_action(std::string_view description, sentinel<Mode>& sentry, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_no_allocation("Unexpected allocation detected for operator!=", sentry, x, y, checkers...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_move_action(sentinel<Mode>& sentry, const Container& y, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_y_allocation(sentry, y, checkers...);
    }

    template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_move_assign_action(std::string_view description, sentinel<Mode>& sentry, Container& y, const Container& yClone, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_assign_allocation(description, sentry, y, checkers...);
      check_mutation_after_move(description, "assignment", sentry, y, yClone, std::move(yMutator), allocation_checker{checkers.info(), y}...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_swap_action([[maybe_unused]] std::string_view description, sentinel<Mode>& sentry, const Container& x, const Container& y, const Container&, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      if constexpr(((   std::allocator_traits<Allocators>::propagate_on_container_move_assignment::value
                     && std::allocator_traits<Allocators>::propagate_on_container_swap::value) && ... ))
      {
        check_no_allocation(sentry.generate_message("Unexpected allocation detected for swap"), sentry, y, x, checkers...);
      }
    }
  };

  /*! \name OverloadSet

      Functions for performing allocation checks which are common to types with both regular and
      move-only semantics. These functions provide an extra level of indirection in order that
      the current number of allocations may be acquired before proceeding
   */
 
  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  bool check_preconditions(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_preconditions(description, sentry, actions, x, y, dual_allocation_checker<Container, Allocators, Predictions>{checkers.info(), x, y}...);
  }

  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  std::optional<Container> check_move_construction(sentinel<Mode>& sentry, const Actions& actions, Container&& z, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_move_construction(sentry, actions, std::forward<Container>(z), y, allocation_checker{checkers.info(), z}...);
  }

  template<test_mode Mode, class Actions, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_move_assign(std::string_view description, sentinel<Mode>& sentry, const Actions& actions, Container& u, Container&& v, const Container& y, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_move_assign(description, sentry, actions, u, std::forward<Container>(v), y, std::move(yMutator), dual_allocation_checker{checkers.info(), u, v}...);
  }

  template<test_mode Mode, class Container, class Mutator, class... Checkers>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, sentinel<Mode>& sentry, Container& u, const Container& y, Mutator yMutator, Checkers... checkers)
  {
    yMutator(u);
    auto mess{append_indented("mutation after move", moveType)};
    check_mutation_allocation(append_indented(description, mess), sentry, u, checkers...);

    mess = append_indented("Mutation is not doing anything following move", moveType);
    check(sentry.generate_message(mess), sentry.logger(), u != y);    
  }

  template<test_mode Mode, class Container, class Mutator, class... Checkers, std::size_t... I>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, sentinel<Mode>& sentry, Container& u, const Container& y, Mutator yMutator, std::tuple<Checkers...> checkers, std::index_sequence<I...>)
  {
    check_mutation_after_move(description, moveType, sentry, u, y, std::move(yMutator), std::get<I>(checkers)...);
  }

  template<test_mode Mode, class Container, class Mutator, class... Checkers>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, sentinel<Mode>& sentry, Container& u, const Container& y, Mutator yMutator, std::tuple<Checkers...> checkers)
  {
    check_mutation_after_move(description, moveType, sentry, u, y, std::move(yMutator), std::move(checkers), std::make_index_sequence<sizeof...(Checkers)>{});
  }
}
