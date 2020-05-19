////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for allocation checks within the testing framework.

    
*/

#include "SemanticsCheckersDetails.hpp"
#include <scoped_allocator>

namespace sequoia::unit_testing
{  
  template<class Container, class Allocator, class Predictions>
  class basic_allocation_info;
}

namespace sequoia::unit_testing::impl
{
  /*! \brief Wraps basic_allocation_info, together with two ints which hold various allocation counts.

      Depending on the scenario, the interpretation of the two ints, accessed through first_ccount() 
      and second_count() is slightly different.

      A. There are two containers, x and y, which in some way interact e.g. via copy/move or swap
         followed by mutation. In this case the ints should hold the number of allocations (for
         a given allocator of) both x and y, prior to the operation of interest being performed.
         The number of allocations after the operation may be acquired by inspecting the Container
         via the function object stored in basic_allocation_info. Combining this with the result of
         invoking either first_count() or second_count(), as appropriate, gives a number which may
         be compared to the appropriate prediction stored within basic_allocation_info.

      B. There is a single container, x, on which a potentially allocating operation is performed.
         NOW IT IS ONLY NECESSARY TO USE 1 INT, BUT TWO ARE BEING USED!!

   */

  template<test_mode Mode, class Container, class Allocator, class Predictions>
  static void check_allocation(std::string_view description, std::string_view detail, std::string_view suffix, test_logger<Mode>& logger, const Container& container, const basic_allocation_info<Container, Allocator, Predictions>& info, const int previous, const int prediction)
  {
    typename test_logger<Mode>::sentinel s{logger, add_type_info<Allocator>(description)};

    const auto current{info.count(container)};      
    auto message{combine_messages(combine_messages(description, detail), suffix)};

    check_equality(std::move(message), logger, current - previous, prediction);
  }

  template<class Container, class Allocator, class Predictions>
  class allocation_checker_data
  {
  public:
    using alloc_info = basic_allocation_info<Container, Allocator, Predictions>;

    allocation_checker_data(const int firstCount, const int secondCount, alloc_info i)
      : m_FirstCount{firstCount}
      , m_SecondCount{secondCount}
      , m_Info{std::move(i)}
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
  private:      
    int m_FirstCount{}, m_SecondCount{};
    
    alloc_info m_Info{};
  };

  template<class Container, class Allocator, class Predictions>
  class dual_allocation_checker
  {
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<Container, Allocator, Predictions>;

    dual_allocation_checker(const Container& x, const Container& y, alloc_info i)
      : m_Data{i.count(x), i.count(y), std::move(i)}
      , m_AllocatorsEqual{m_Data.info().allocator(x) == m_Data.info().allocator(y)}
    {}

    [[nodiscard]]
    const alloc_info& info() const noexcept
    {
      return m_Data.info();
    }

    [[nodiscard]]
    int first_count() const noexcept
    {
      return m_Data.first_count();
    }

    [[nodiscard]]
    int second_count() const noexcept
    {
      return m_Data.second_count();
    }

    template<test_mode Mode>
    void check_no_allocation(std::string_view description, test_logger<Mode>& logger, const Container& x, const Container& y) const
    {
      check_allocation(description, "No allocation for x", "", logger, x, info(), first_count(), 0);
      check_allocation(description, "No allocation for y", "", logger, y, info(), second_count(), 0);
    }

    template<test_mode Mode>
    void check_copy_assign_y_to_x(std::string_view description, test_logger<Mode>& logger, const Container& xContainer, const Container& yContainer) const
    {
      typename test_logger<Mode>::sentinel s{logger, add_type_info<Allocator>(description)};

      constexpr bool propagate{std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value};
      
      int yPrediction{};
      if constexpr(propagate)
      {
        yPrediction = info().get_predictions().assign_y_to_x.with_propagation;        
        check_allocation(description, "Copy assignment x allocations", "", logger, xContainer, info(), second_count(), yPrediction);
      }
      else
      {
        const int xPrediction{info().get_predictions().assign_y_to_x.without_propagation};        
        check_allocation(description, "Copy assignment x allocations", "", logger, xContainer, info(), first_count(), xPrediction);
      }

      check_allocation(description, "Copy assignment y allocations", "", logger, yContainer, info(), second_count(), yPrediction);
    }

    template<test_mode Mode>
    void check_move_assign_y_to_x(std::string_view description, test_logger<Mode>& logger, const Container& xContainer) const
    {
      typename test_logger<Mode>::sentinel s{logger, add_type_info<Allocator>(description)};

      constexpr bool propagate{std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value};

      const bool copyLike{!propagate && !m_AllocatorsEqual};

      const int xPrediction{
        copyLike ? info().get_predictions().assign_without_propagation_allocs() : 0        
      };
      
      if constexpr(propagate)
      {
        check_allocation(description, "Move assignment x allocations", "", logger, xContainer, info(), second_count(), xPrediction);
      }
      else
      {
        check_allocation(description, "Move assignment x allocations", "", logger, xContainer, info(), first_count(), xPrediction);
      }
    }

    template<test_mode Mode>
    void check_mutation_after_swap(std::string_view description, test_logger<Mode>& logger, const Container& lhs, const Container& rhs) const
    {
      typename test_logger<Mode>::sentinel s{logger, add_type_info<Allocator>(description)};

      const auto prediction{info().get_predictions().mutation_allocs()};
      auto lhCount{first_count()}, rhCount{second_count()};
      
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_swap::value)
      {
        using std::swap;
        swap(lhCount, rhCount);
      }

      check_allocation(description, "y allocations", "", logger, lhs, info(), lhCount, prediction);
      check_allocation(description, "x allocations", "", logger, rhs, info(), rhCount, 0);
    }
  private:
    using data = allocation_checker_data<Container, Allocator, Predictions>;

    data m_Data;
    bool m_AllocatorsEqual{};
    
  };


  template<class Container, class Allocator, class Predictions>
  class para_allocation_checker
  {
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<Container, Allocator, Predictions>;
    
    explicit para_allocation_checker(alloc_info i)
      : m_Data{0, 0, std::move(i)}
    {}

    para_allocation_checker(const Container& x, const int priorCount, alloc_info i)
      : m_Data{i.count(x), priorCount, std::move(i)}
    {}

    [[nodiscard]]
    const alloc_info& info() const noexcept
    {
      return m_Data.info();
    }

    template<test_mode Mode>
    void check_para_copy_y(std::string_view description, test_logger<Mode>& logger, const Container& container) const
    {
      check_para_copy(description, "(y)", logger, container, info(), info().get_predictions().y.para_copy);
    }

    template<test_mode Mode>
    void check_para_move_y(std::string_view description, test_logger<Mode>& logger, const Container& container) const
    {
      const auto prediction{info().get_predictions().para_move_allocs()};

      check_para_move(description, "(y)", logger, container, info(), prediction);
    }

    template<test_mode Mode>
    void check_mutation(std::string_view description, test_logger<Mode>& logger, const Container& yContainer) const
    {
      check_allocation(description, "", "", logger, yContainer, info(), first_count(), info().get_predictions().mutation_allocs());
    }
  private:
    using data = allocation_checker_data<Container, Allocator, Predictions>;

    data m_Data;

    [[nodiscard]]
    int first_count() const noexcept
    {
      return m_Data.first_count();
    }

    [[nodiscard]]
    int second_count() const noexcept
    {
      return m_Data.second_count();
    }

    template<test_mode Mode>
    void check_para_copy(std::string_view description, std::string_view suffix, test_logger<Mode>& logger, const Container& container, const alloc_info& info, const int prediction) const
    {
      check_allocation(description, "Para copy construction allocation", suffix, logger, container, info, second_count(), prediction);
    }

    template<test_mode Mode>
    void check_para_move(std::string_view description, std::string_view suffix, test_logger<Mode>& logger, const Container& container, const alloc_info& info, const int prediction) const
    {
      check_allocation(description, "Para move construction allocation", suffix, logger, container, info, first_count(), prediction);
    }
  };

  template<class Container, class Allocator, class Predictions>
  class allocation_checker
  {
  public:
    using container_type   = Container;
    using allocator_type   = Allocator;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<Container, Allocator, Predictions>;
    
    allocation_checker(const Container& x, const int priorCount, alloc_info i)
      : m_Data{i.count(x), priorCount, std::move(i)}
    {}
   
    template<test_mode Mode>
    void check_copy_x(std::string_view description, test_logger<Mode>& logger, const Container& container) const
    {
      check_copy(description, "(x)", logger, container, info(), second_count(), info().get_predictions().copy_x);
    }

    template<test_mode Mode>
    void check_copy_y(std::string_view description, test_logger<Mode>& logger, const Container& container) const
    {
      check_copy(description, "(y)", logger, container, info(), second_count(), info().get_predictions().y.copy);
    }

    template<test_mode Mode>
    void check_move_y(std::string_view description, test_logger<Mode>& logger, const Container& container) const
    {
      check_allocation(description, "Move construction allocation", "(y)", logger, container, info(), second_count(), 0);
    }

    template<test_mode Mode>
    void check_mutation(std::string_view description, test_logger<Mode>& logger, const Container& yContainer) const
    {
      check_allocation(description, "", "", logger, yContainer, info(), first_count(), info().get_predictions().mutation_allocs());
    }

    [[nodiscard]]
    const alloc_info& info() const noexcept
    {
      return m_Data.info();
    }

    [[nodiscard]]
    int first_count() const noexcept
    {
      return m_Data.first_count();
    }
  private:
    using data = allocation_checker_data<Container, Allocator, Predictions>;

    data m_Data;

    [[nodiscard]]
    int second_count() const noexcept
    {
      return m_Data.second_count();
    }

    template<test_mode Mode>
    static void check_copy(std::string_view description, std::string_view suffix, test_logger<Mode>& logger, const Container& container, const alloc_info& info, const int previous, const int prediction)
    {
      check_allocation(description, "Copy construction allocation", suffix, logger, container, info, previous, prediction);
    }
  };
  
  template<class T, class... Allocators, class... Predictions>
  struct do_swap<dual_allocation_checker<T, Allocators, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
        || std::allocator_traits<Allocators>::is_always_equal::value) && ...) };
  };

  template<class T, class... Allocators, class... Predictions>
  struct do_swap<para_allocation_checker<T, Allocators, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
        || std::allocator_traits<Allocators>::is_always_equal::value) && ...) };
  };

  template<class T, class... Allocators, class... Predictions>
  struct do_swap<allocation_checker<T, Allocators, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
        || std::allocator_traits<Allocators>::is_always_equal::value) && ...) };
  };
  
  template<class Fn, class... Ts, std::size_t... I>
  decltype(auto) unpack_invoke(const std::tuple<Ts...>& t, Fn&& fn, std::index_sequence<I...>)
  {
    return fn(std::get<I>(t)...);
  }

  template<class Fn, class... Ts>
  decltype(auto) unpack_invoke(const std::tuple<Ts...>& t, Fn&& fn)
  {
    return unpack_invoke(t, std::forward<Fn>(fn), std::make_index_sequence<sizeof...(Ts)>{});
  }

  // make_dual_allocation_checkers

  template<class Container, class Predictions, class... Allocators, class... Args, std::size_t... I>
  [[nodiscard]]
  auto make_dual_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, std::index_sequence<I...>, Args&&... args)
  {
    return std::make_tuple(dual_allocation_checker{std::forward<Args>(args)..., info.template unpack<I>()}...);
  }

  template<class Container, class Predictions, class... Args, class... Allocators>
  [[nodiscard]]
  auto make_dual_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_dual_allocation_checkers(info, std::make_index_sequence<sizeof...(Allocators)>{}, std::forward<Args>(args)...);
  }

  template<class Container, class Allocator, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<dual_allocation_checker<Container, Allocator, Predictions>>
  make_dual_allocation_checkers(const basic_allocation_info<Container, Allocator, Predictions>& info, Args&&... args)
  {
    using checker = dual_allocation_checker<Container, Allocator, Predictions>;
    return {checker{std::forward<Args>(args)..., info}};
  }

  // make_para_allocation_checkers
  
  template<class Container, class Predictions, class... Allocators, class... Args, std::size_t... I>
  [[nodiscard]]
  auto make_para_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, std::index_sequence<I...>, Args&&... args)
  {
    return std::make_tuple(para_allocation_checker{std::forward<Args>(args)..., info.template unpack<I>()}...);
  }

  template<class Container, class Predictions, class... Args, class... Allocators>
  [[nodiscard]]
  auto make_para_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_para_allocation_checkers(info, std::make_index_sequence<sizeof...(Allocators)>{}, std::forward<Args>(args)...);
  }

  template<class Container, class Allocator, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<para_allocation_checker<Container, Allocator, Predictions>>
  make_para_allocation_checkers(const basic_allocation_info<Container, Allocator, Predictions>& info, Args&&... args)
  {
    using checker = para_allocation_checker<Container, Allocator, Predictions>;
    return {checker{std::forward<Args>(args)..., info}};
  }

  //================================ Variadic Allocation Checking ================================//

  template<test_mode Mode, class CheckFn, class Checker, class... Checkers>
  void check_allocation(std::string_view description, test_logger<Mode>& logger, CheckFn check, const Checker& checker, const Checkers&... moreCheckers)
  {
    using Allocator = typename Checker::allocator_type;
    check(add_type_info<Allocator>(description), checker);

    if constexpr (sizeof...(Checkers) > 0)
    {
      check_allocation(description, logger, check, moreCheckers...); 
    }
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_no_allocation(std::string_view description, test_logger<Mode>& logger, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &x, &y](std::string_view message, auto& checker){
        checker.check_no_allocation(message, logger, x, y);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_x_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_x(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_y_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_move_y_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
      
  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_assign_allocation(std::string_view description, test_logger<Mode>& logger, const Container& xContainer, const Container& yContainer, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, auto& checker){
        checker.check_copy_assign_y_to_x(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_move_assign_allocation(std::string_view description, test_logger<Mode>& logger, const Container& xContainer, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer](std::string_view message, auto& checker){
        checker.check_move_assign_y_to_x(message, logger, xContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, test_logger<Mode>& logger, const Container& xContainer, const Container& yContainer, const dual_allocation_checker<Container, Allocator, Prediction>& checker, const dual_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation_after_swap(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, test_logger<Mode>& logger, const Container& yContainer, const para_allocation_checker<Container, Allocator, Prediction>& checker, const para_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation(message, logger, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, test_logger<Mode>& logger, const Container& yContainer, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation(message, logger, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_para_copy_y_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, const para_allocation_checker<Container, Allocator, Prediction>& checker, const para_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_para_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
  
  template<test_mode Mode, class Container, class... Allocators, class... Predictions>
  void check_para_copy_y_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, std::tuple<para_allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&logger,&container](auto&&... checkers){
        check_para_copy_y_allocation(description, logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);
  }

  template<test_mode Mode, class Container, class Prediction, class Allocator, class... Allocators, class... Predictions>
  void check_para_move_y_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, const para_allocation_checker<Container, Allocator, Prediction>& checker, const para_allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_para_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
  
  template<test_mode Mode, class Container, class... Allocators, class... Predictions>
  void check_para_move_y_allocation(std::string_view description, test_logger<Mode>& logger, const Container& container, std::tuple<para_allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&logger,&container](auto&&... checkers){
        check_para_move_y_allocation(description, logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);    
  }

  template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_mutation_after_swap(std::string_view description, test_logger<Mode>& logger, Container& lhs, const Container& rhs, const Container& y, Mutator yMutator, dual_allocation_checker<Container, Allocators, Predictions>... checkers)
  {
    if(check(combine_messages(description, "Mutation after swap pre-condition violated"), logger, lhs == y))
    {    
      yMutator(lhs);
      check_mutation_allocation(combine_messages(description, "mutation after swap"), logger, lhs, rhs, checkers...);

      check(combine_messages(description, "Mutation is not doing anything following copy then swap"), logger, lhs != y);
    }
  }

  struct allocation_actions : pre_condition_actions
  {    
    constexpr static bool has_post_equality_action{true};
    constexpr static bool has_post_nequality_action{true};
    constexpr static bool has_post_move_action{true};
    constexpr static bool has_post_move_assign_action{true};
    constexpr static bool has_post_swap_action{true};

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_equality_action(std::string_view description, test_logger<Mode>& logger, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_no_allocation(combine_messages(description, "no allocation for operator=="), logger, x, y, checkers...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_nequality_action(std::string_view description, test_logger<Mode>& logger, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_no_allocation(combine_messages(description, "no allocation for operator!="), logger, x, y, checkers...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_move_action(std::string_view description, test_logger<Mode>& logger, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_y_allocation(description, logger, y, allocation_checker<Container, Allocators, Predictions>{y, checkers.first_count(), checkers.info()}...);
    }

    template<test_mode Mode, class Container, class... Allocators, class... Predictions>
    static void post_move_action(std::string_view description, test_logger<Mode>& logger, const Container& y, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_y_allocation(description, logger, y, allocation_checker<Container, Allocators, Predictions>{y, checkers.first_count(), checkers.info()}...);
    }

    template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_move_assign_action(std::string_view description, test_logger<Mode>& logger, Container& y, const Container& yClone, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_assign_allocation(description, logger, y, checkers...);
      check_mutation_after_move(description, "assignment", logger, y, yClone, yMutator, allocation_checker<Container, Allocators, Predictions>{y, 0, checkers.info()}...);
    }

    template<test_mode Mode, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_swap_action(std::string_view description, test_logger<Mode>& logger, Container& x, const Container& y, const Container& yClone, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_mutation_after_swap(description, logger, x, y, yClone, yMutator, checkers...);
    }
  };

  // Functions for performing allocation checks which are common to types with both regular and
  // move-only semantics
 
  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  bool check_preconditions(std::string_view description, test_logger<Mode>& logger, const Actions& actions, const Container& x, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_preconditions(description, logger, actions, x, y, dual_allocation_checker<Container, Allocators, Predictions>{x, y, checkers.info()}...);
  }

  template<test_mode Mode, class Actions, class Container, class... Allocators, class... Predictions>
  void check_move_construction(std::string_view description, test_logger<Mode>& logger, const Actions& actions, Container&& z, const Container& y, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_move_construction(description, logger, actions, std::forward<Container>(z), y, allocation_checker<Container, Allocators, Predictions>{z, 0, checkers.info()}...);
  }

  template<test_mode Mode, class Actions, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_move_assign(std::string_view description, test_logger<Mode>& logger, const Actions& actions, Container& u, Container&& v, const Container& y, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_move_assign(description, logger, actions, u, std::forward<Container>(v), y, yMutator, dual_allocation_checker<Container, Allocators, Predictions>{u, v, checkers.info()}...);
  }

  template<test_mode Mode, class Actions, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_swap(std::string_view description, test_logger<Mode>& logger, const Actions& actions, Container&& x, Container& y, const Container& xClone, const Container& yClone, Mutator yMutator, const dual_allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_swap(description, logger, actions, std::forward<Container>(x), y, xClone, yClone, std::move(yMutator), dual_allocation_checker<Container, Allocators, Predictions>{x, y, checkers.info()}...);
  }

  template<test_mode Mode, class Container, class Mutator, class... Checkers>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, test_logger<Mode>& logger, Container& u, const Container& y, Mutator yMutator, Checkers... checkers)
  {
    yMutator(u);
    auto mess{combine_messages("mutation after move", moveType)};
    check_mutation_allocation(combine_messages(description, std::move(mess)), logger, u, checkers...);

    mess = combine_messages("Mutation is not doing anything following move", moveType);
    check(combine_messages(description, std::move(mess)), logger, u != y);    
  }

  template<test_mode Mode, class Container, class Mutator, class... Checkers, std::size_t... I>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, test_logger<Mode>& logger, Container& u, const Container& y, Mutator yMutator, std::tuple<Checkers...> checkers, std::index_sequence<I...>)
  {
    check_mutation_after_move(description, moveType, logger, u, y, std::move(yMutator), std::get<I>(checkers)...);
  }

  template<test_mode Mode, class Container, class Mutator, class... Checkers>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, test_logger<Mode>& logger, Container& u, const Container& y, Mutator yMutator, std::tuple<Checkers...> checkers)
  {
    check_mutation_after_move(description, moveType, logger, u, y, std::move(yMutator), std::move(checkers), std::make_index_sequence<sizeof...(Checkers)>{});
  }
}
