////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file AllocationCheckersDetails.hpp
    \brief Implementation details for allocation checks within the unit testing framework.
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

  template<class Container, class Allocator, class Predictions>
  class allocation_checker
  {
  public:
    using alloc_info = basic_allocation_info<Container, Allocator, Predictions>;
    
    allocation_checker(alloc_info i)
      : m_Info{std::move(i)}
    {}

    allocation_checker(const Container& x, const Container& y, alloc_info i)
      : m_Info{std::move(i)}
      , m_FirstCount{m_Info.count(x)}
      , m_SecondCount{m_Info.count(y)}
      , m_AllocatorsEqual{m_Info.allocator(x) == m_Info.allocator(y)}
    {}

    allocation_checker(const Container& x, const int existingCount, alloc_info i)
      : m_Info{std::move(i)}
      , m_FirstCount{m_Info.count(x)}
      , m_SecondCount{existingCount}
    {}

    [[nodiscard]]
    int first_count() const noexcept { return m_FirstCount; }

    [[nodiscard]]
    int second_count() const noexcept { return m_SecondCount; }

    template<class Logger>
    void check_no_allocation(std::string_view description, Logger& logger, const Container& container) const
    {
      check_allocation(description, "", "", logger, container, m_Info, m_FirstCount, 0);
    }

    template<class Logger>
    void check_copy_x(std::string_view description, Logger& logger, const Container& container) const
    {
      check_copy(description, "(x)", logger, container, m_Info, m_SecondCount, m_Info.get_predictions().copy_x);
    }

    template<class Logger>
    void check_copy_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_copy(description, "(y)", logger, container, m_Info, m_SecondCount, m_Info.get_predictions().y.copy);
    }

    template<class Logger>
    void check_para_copy_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_para_copy(description, "(y)", logger, container, m_Info, m_Info.get_predictions().y.para_copy);
    }

    template<class Logger>
    void check_move_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_allocation(description, "Move construction allocation", "(y)", logger, container, m_Info, m_SecondCount, 0);
    }

    template<class Logger>
    void check_para_move_y(std::string_view description, Logger& logger, const Container& container) const
    {
      const auto prediction{m_Info.get_predictions().para_move_allocs()};

      check_para_move(description, "(y)", logger, container, m_Info, prediction);
    }

    template<class Logger>
    void check_copy_assign_y_to_x(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer) const
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      constexpr bool propagate{std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value};
      
      int yPrediction{};
      if constexpr(propagate)
      {
        yPrediction = m_Info.get_predictions().assign_y_to_x.with_propagation;        
        check_allocation(description, "Copy assignment x allocations", "", logger, xContainer, m_Info, m_SecondCount, yPrediction);
      }
      else
      {
        const int xPrediction{m_Info.get_predictions().assign_y_to_x.without_propagation};        
        check_allocation(description, "Copy assignment x allocations", "", logger, xContainer, m_Info, m_FirstCount, xPrediction);
      }

      check_allocation(description, "Copy assignment y allocations", "", logger, yContainer, m_Info, m_SecondCount, yPrediction);
    }

    template<class Logger>
    void check_move_assign_y_to_x(std::string_view description, Logger& logger, const Container& xContainer) const
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      constexpr bool propagate{std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value};

      const bool copyLike{!propagate && !m_AllocatorsEqual};

      const int xPrediction{
        copyLike ? m_Info.get_predictions().assign_without_propagation_allocs() : 0        
      };
      
      if constexpr(propagate)
      {
        check_allocation(description, "Move assignment x allocations", "", logger, xContainer, m_Info, m_SecondCount, xPrediction);
      }
      else
      {
        check_allocation(description, "Move assignment x allocations", "", logger, xContainer, m_Info, m_FirstCount, xPrediction);
      }
    }

    template<class Logger>
    void check_mutation(std::string_view description, Logger& logger, const Container& yContainer) const
    {
      check_allocation(description, "Mutation allocation after move-like construction", "", logger, yContainer, m_Info, m_FirstCount, m_Info.get_predictions().mutation_allocs());
    }

    template<class Logger>
    void check_mutation_after_swap(std::string_view description, Logger& logger, const Container& lhs, const Container& rhs) const
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      const auto prediction{m_Info.get_predictions().mutation_allocs()};
      auto lhCount{m_FirstCount}, rhCount{m_SecondCount};
      
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_swap::value)
      {
        using std::swap;
        swap(lhCount, rhCount);
      }

      check_allocation(description, "y allocations", "", logger, lhs, m_Info, lhCount, prediction);
      check_allocation(description, "x allocations", "", logger, rhs, m_Info, rhCount, 0);
    }     

    [[nodiscard]]
    const alloc_info& info() const noexcept
    {
      return m_Info;
    }
  private:
    alloc_info m_Info;
      
    int m_FirstCount{}, m_SecondCount{};
    bool m_AllocatorsEqual{};

    template<class Logger>
    static void check_allocation(std::string_view description, std::string_view detail, std::string_view suffix, Logger& logger, const Container& container, const alloc_info& info, const int previous, const int prediction)
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      const auto current{info.count(container)};      
      auto message{combine_messages(combine_messages(description, detail), suffix)};

      check_equality(std::move(message), logger, current - previous, prediction);
    }    

    template<class Logger>
    static void check_copy(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const alloc_info& info, const int previous, const int prediction)
    {
      check_allocation(description, "Copy construction allocation", suffix, logger, container, info, previous, prediction);
    }

    template<class Logger>
    void check_para_copy(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const alloc_info& info, const int prediction) const
    {
      check_allocation(description, "Para copy construction allocation", suffix, logger, container, info, m_SecondCount, prediction);
    }

    template<class Logger>
    void check_para_move(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const alloc_info& info, const int prediction) const
    {
      check_allocation(description, "Para move construction allocation", suffix, logger, container, info, m_FirstCount, prediction);
    }

    template<class Logger>
    void check_mutation(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const alloc_info& info, const int prediction) const
    {
      check_allocation(description, "Mutation allocation", suffix, logger, container, info, m_SecondCount, prediction);
    }
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

  // make_allocation_checkers

  template<class Container, class Predictions, class... Allocators, class... Args, std::size_t... I>
  [[nodiscard]]
  auto make_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, std::index_sequence<I...>, Args&&... args)
  {
    return std::make_tuple(allocation_checker{std::forward<Args>(args)..., info.template unpack<I>()}...);
  }

  template<class Container, class Predictions, class... Args, class... Allocators>
  [[nodiscard]]
  auto make_allocation_checkers(const basic_allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_allocation_checkers(info, std::make_index_sequence<sizeof...(Allocators)>{}, std::forward<Args>(args)...);
  }

  template<class Container, class Allocator, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<allocation_checker<Container, Allocator, Predictions>>
  make_allocation_checkers(const basic_allocation_info<Container, Allocator, Predictions>& info, Args&&... args)
  {
    return {{std::forward<Args>(args)..., info}};
  }

  //================================ Variadic Allocation Checking ================================//

  template<class Logger, class CheckFn, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_allocation(std::string_view description, Logger& logger, CheckFn check, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    check(add_type_info<Allocator>(description), checker);

    if constexpr (sizeof...(Allocators) > 0)
    {
      check_allocation(description, logger, check, moreCheckers...); 
    }
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_no_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_no_allocation(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_x_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_x(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_move_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
      
  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_copy_assign_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, auto& checker){
        checker.check_copy_assign_y_to_x(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_move_assign_allocation(std::string_view description, Logger& logger, const Container& xContainer, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer](std::string_view message, auto& checker){
        checker.check_move_assign_y_to_x(message, logger, xContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation_after_swap(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_mutation_allocation(std::string_view description, Logger& logger, const Container& yContainer, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation(message, logger, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class Prediction, class... Allocators, class... Predictions>
  void check_para_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_para_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
  
  template<class Logger, class Container, class... Allocators, class... Predictions>
  void check_para_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&logger,&container](auto&&... checkers){
        check_para_copy_y_allocation(description, logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);
  }

  template<class Logger, class Container, class Prediction, class Allocator, class... Allocators, class... Predictions>
  void check_para_move_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Prediction>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_para_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
  
  template<class Logger, class Container, class... Allocators, class... Predictions>
  void check_para_move_y_allocation(std::string_view description, Logger& logger, const Container& container, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&logger,&container](auto&&... checkers){
        check_para_move_y_allocation(description, logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);    
  }

  template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_mutation_after_swap(std::string_view description, Logger& logger, Container& lhs, const Container& rhs, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators, Predictions>... checkers)
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

    template<class Logger, class Container, class... Allocators, class... Predictions>
    static void post_equality_action(std::string_view description, Logger& logger, const Container& x, const Container&, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_no_allocation(combine_messages(description, "no allocation for operator=="), logger, x, checkers...);
    }

    template<class Logger, class Container, class... Allocators, class... Predictions>
    static void post_nequality_action(std::string_view description, Logger& logger, const Container& x, const Container&, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_no_allocation(combine_messages(description, "no allocation for operator!="), logger, x, checkers...);
    }

    template<class Logger, class Container, class... Allocators, class... Predictions>
    static void post_move_action(std::string_view description, Logger& logger, const Container& y, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_y_allocation(description, logger, y, allocation_checker<Container, Allocators, Predictions>{y, checkers.first_count(), checkers.info()}...);
    }

    template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_move_assign_action(std::string_view description, Logger& logger, Container& y, const Container& yClone, Mutator yMutator, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_move_assign_allocation(description, logger, y, checkers...);
      check_mutation_after_move(description, "assignment", logger, y, yClone, yMutator, allocation_checker<Container, Allocators, Predictions>{y, 0, checkers.info()}...);
    }

    template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
    static void post_swap_action(std::string_view description, Logger& logger, Container& x, const Container& y, const Container& yClone, Mutator yMutator, const allocation_checker<Container, Allocators, Predictions>&... checkers)
    {
      check_mutation_after_swap(description, logger, x, y, yClone, yMutator, checkers...);
    }
  };
 
  template<class Logger, class Actions, class Container, class... Allocators, class... Predictions>
  bool check_preconditions(std::string_view description, Logger& logger, const Actions& actions, const Container& x, const Container& y, const allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    return do_check_preconditions(description, logger, actions, x, y, allocation_checker<Container, Allocators, Predictions>{x, checkers.first_count(), checkers.info()}...);
  }

  template<class Logger, class Actions, class Container, class... Allocators, class... Predictions>
  void check_move_construction(std::string_view description, Logger& logger, const Actions& actions, Container&& z, const Container& y, const allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_move_construction(description, logger, actions, std::forward<Container>(z), y, allocation_checker<Container, Allocators, Predictions>{z, 0, checkers.info()}...);
  }

  template<class Logger, class Actions, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_move_assign(std::string_view description, Logger& logger, const Actions& actions, Container& u, Container&& v, const Container& y, Mutator yMutator, const allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_move_assign(description, logger, actions, u, std::forward<Container>(v), y, yMutator, allocation_checker<Container, Allocators, Predictions>{u, v, checkers.info()}...);
  }

  template<class Logger, class Actions, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_swap(std::string_view description, Logger& logger, const Actions& actions, Container&& x, Container& y, const Container& xClone, const Container& yClone, Mutator yMutator, const allocation_checker<Container, Allocators, Predictions>&... checkers)
  {
    do_check_swap(description, logger, actions, std::forward<Container>(x), y, xClone, yClone, std::move(yMutator), allocation_checker<Container, Allocators, Predictions>{x, y, checkers.info()}...);
  }

  template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, Logger& logger, Container& u, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators, Predictions>... checkers)
  {
    yMutator(u);
    auto mess{combine_messages("mutation after move", moveType)};
    check_mutation_allocation(combine_messages(description, std::move(mess)), logger, u, checkers...);

    mess = combine_messages("Mutation is not doing anything following move", moveType);
    check(combine_messages(description, std::move(mess)), logger, u != y);    
  }

  template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions, std::size_t... I>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, Logger& logger, Container& u, const Container& y, Mutator yMutator, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers, std::index_sequence<I...>)
  {
    check_mutation_after_move(description, moveType, logger, u, y, std::move(yMutator), std::get<I>(checkers)...);
  }

  template<class Logger, class Container, class Mutator, class... Allocators, class... Predictions>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, Logger& logger, Container& u, const Container& y, Mutator yMutator, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    check_mutation_after_move(description, moveType, logger, u, y, std::move(yMutator), std::move(checkers), std::make_index_sequence<sizeof...(Allocators)>{});
  }
}
