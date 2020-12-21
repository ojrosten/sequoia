////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for allocation checks.
*/

#include "SemanticsCheckersDetails.hpp"
#include "AllocationCheckersTraits.hpp"
#include "FreeCheckers.hpp"

#include <scoped_allocator>

namespace sequoia::testing
{
  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  class basic_allocation_info;

  enum class null_allocation_event { comparison, spectator, serialization };
  
  /*! Type-safe wrapper for allocation predictions, to avoid mixing different allocation events */
  template<auto Event>
  class alloc_prediction
  {
  public:
    constexpr alloc_prediction() = default;

    constexpr alloc_prediction(int unshifted, int delta={}) noexcept
      : m_Unshifted{unshifted}
      , m_Prediction{m_Unshifted + delta}
    {}

    [[nodiscard]]
    constexpr int value() const noexcept
    {
      return m_Prediction;
    }

    [[nodiscard]]
    constexpr int unshifted() const noexcept
    {
      return m_Unshifted;
    }
  private:
    int m_Unshifted{}, m_Prediction{};
  };
}

namespace sequoia::testing::impl
{
  struct allocation_advice
  {
    std::string operator()(int count, int) const;
  };

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Predictions>
  bool check_allocation(std::string_view detail,
                               test_logger<Mode>& logger,
                               const T& container,
                               const basic_allocation_info<T, Getter, Predictions>& info,
                               const int previous,
                               const int prediction)
  {
    const auto current{info.count(container)};

    using allocator = decltype(std::declval<Getter>()(container));
    const auto message{ append_lines(make_type_info<allocator>(), detail) };

    return check_equality(message, logger, current - previous, prediction, tutor{allocation_advice{}});
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Predictions, auto Event>
  bool check_allocation(std::string_view detail,
    test_logger<Mode>& logger,
    const T& container,
    const basic_allocation_info<T, Getter, Predictions>& info,
    const int previous,
    const alloc_prediction<Event> prediction)
  {
    const auto unshifted{prediction.unshifted()};
    const auto delta{prediction.value() - unshifted};
    return check_allocation(detail, logger, container, info, previous + delta, unshifted);
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
  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  class dual_allocation_checker
  {
  public:
    using value_type       = T;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<T, Getter, Predictions>;
    using allocator_type   = typename alloc_info::allocator_type;

    dual_allocation_checker(alloc_info i, const T& x, const T& y)
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
    void check_no_allocation(std::string_view detail, test_logger<Mode>& logger, const T& x, const T& y) const
    {
      const alloc_prediction<null_allocation_event::comparison> prediction{};
      check_allocation(append_lines(detail, "Unexpected allocation detected (x)"), logger, x, info(), first_count(), prediction);
      check_allocation(append_lines(detail, "Unexpected allocation detected (y)"), logger, y, info(), second_count(), prediction);
    }

    template<test_mode Mode>
    void check_copy_assign_y_to_x(test_logger<Mode>& logger, const T& x, const T& y) const
    {
      constexpr bool propagate{std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value};
      if constexpr(propagate)
      {
        const auto yPrediction{info().get_predictions().assign_y_to_x.with_propagation};
        check_allocation("Unexpected allocation detected for propagating copy assignment (x)", logger, x, info(), second_count(), yPrediction);
        check_allocation("Unexpected allocation detected for propagating copy assignment (y)", logger, y, info(), second_count(), yPrediction);
      }
      else
      {
        const auto xPrediction{info().get_predictions().assign_y_to_x.without_propagation};
        check_allocation("Unexpected allocation detected for copy assignment (x)", logger, x, info(), first_count(), xPrediction);

        const alloc_prediction<null_allocation_event::spectator> yPrediction{};
        check_allocation("Unexpected allocation detected for copy assignment (y)", logger, y, info(), second_count(), yPrediction);
      }
    }

    template<test_mode Mode>
    void check_move_assign_y_to_x(test_logger<Mode>& logger, const T& x) const
    {
      constexpr bool propagate{std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value};
      const auto& predictions{info().get_predictions()};
      
      if (m_AllocatorsEqual || propagate)
      {
        const auto xPrediction{predictions.move_assign_allocs()};
        check_allocation("Unexpected allocation detected for propagating move assignment (x)", logger, x, info(), second_count(), xPrediction);
      }
      else
      {
        const auto xPrediction{predictions.copy_like_move_assign_allocs()};
        check_allocation("Unexpected allocation detected for move assignment (x)", logger, x, info(), first_count(), xPrediction);
      }
    }

    template<test_mode Mode>
    void check_mutation_after_swap(test_logger<Mode>& logger, const T& lhs, const T& rhs) const
    {
      auto lhCount{first_count()}, rhCount{second_count()};
      
      if constexpr(std::allocator_traits<allocator_type>::propagate_on_container_swap::value)
      {
        using std::swap;
        swap(lhCount, rhCount);
      }

      const auto lhPrediction{ info().get_predictions().mutation_allocs() };
      check_allocation("Unexpected allocation detected following mutation after swap (y)", logger, lhs, info(), lhCount, lhPrediction);

      const alloc_prediction<null_allocation_event::spectator> rhPrediction{};
      check_allocation("Unexpected allocation detected following mutation after swap (x)", logger, rhs, info(), rhCount, rhPrediction);
    }
  private:
    alloc_info m_Info{};
    int m_FirstCount{}, m_SecondCount{};
    bool m_AllocatorsEqual{};
  };

  //================================ Deduction guide ================================//

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  dual_allocation_checker(basic_allocation_info<T, Getter, Predictions>, const T&, const T&)
    -> dual_allocation_checker<T, Getter, Predictions>;

  /*! \brief Wraps basic_allocation_info, together with the prior allocation count.

      Consider a container, x, on which some potentially allocating operation is performed.
      Prior to this operation, an instantiation of the allocation_checker class template
      acquires the number of allocations already performed. Calling the check method after
      the operation of interest, the container is inspected via the function object stored
      in basic_allocation_info. This allows the change in the number of allocations to be
      computed, which is then compared with the prediction.
   */

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  class allocation_checker
  {
  public:
    using value_type       = T;
    using predictions_type = Predictions;
    using alloc_info       = basic_allocation_info<T, Getter, Predictions>;
    using allocator_type   = typename alloc_info::allocator_type;
    
    allocation_checker(alloc_info i, const int priorCount=0)
      : m_Info{std::move(i)}
      , m_PriorCount{priorCount}
    {}

    allocation_checker(alloc_info i, const T& x)
      : m_Info{std::move(i)}
      , m_PriorCount{m_Info.count(x)}
    {}

    template<test_mode Mode, auto Event>
    bool check(std::string_view detail, test_logger<Mode>& logger, const T& container, const alloc_prediction<Event> prediction) const
    {
      return check_allocation(detail, logger, container, info(), m_PriorCount, prediction);
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

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  allocation_checker(basic_allocation_info<T, Getter, Predictions>, const T&)
    -> allocation_checker<T, Getter, Predictions>;

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  allocation_checker(basic_allocation_info<T, Getter, Predictions>, int)
    -> allocation_checker<T, Getter, Predictions>;

  //================================ Specializations of do_swap ================================//
  
  template<class T, alloc_getter<T>... Getters, class... Predictions>
  struct do_swap<dual_allocation_checker<T, Getters, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<typename dual_allocation_checker<T, Getters, Predictions>::allocator_type>::propagate_on_container_swap::value
        || std::allocator_traits<typename dual_allocation_checker<T, Getters, Predictions>::allocator_type>::is_always_equal::value) && ...)
    };
  };

  template<class T, alloc_getter<T>... Getters, class... Predictions>
  struct do_swap<allocation_checker<T, Getters, Predictions>...>
  {
    constexpr static bool value{
      ((   std::allocator_traits<typename allocation_checker<T, Getters, Predictions>::allocator_type>::propagate_on_container_swap::value
        || std::allocator_traits<typename allocation_checker<T, Getters, Predictions>::allocator_type>::is_always_equal::value) && ...)
    };
  };

  //================================ make_scoped_allocation_checkers ================================//

  template
  <
    template<class, class, class> class Checker,
    movable_comparable T,
    class Predictions,
    alloc_getter<T> Getter,
    class... Args,
    std::size_t... I
  >
  [[nodiscard]]
  auto make_scoped_allocation_checkers(const basic_allocation_info<T, Getter, Predictions>& info, std::index_sequence<I...>, Args&&... args)
  {
    return std::make_tuple(Checker{info.template unpack<I>(), std::forward<Args>(args)...}...);
  }

  template
  <
    template<class, class, class> class Checker,
    movable_comparable T,
    class Predictions,
    alloc_getter<T> Getter,
    class... Args
  >
  [[nodiscard]]
  auto make_scoped_allocation_checkers(const basic_allocation_info<T, Getter, Predictions>& info, Args&&... args)
  {
    constexpr std::size_t N{basic_allocation_info<T, Getter, Predictions>::size};
    return make_scoped_allocation_checkers<Checker>(info, std::make_index_sequence<N>{}, std::forward<Args>(args)...);
  }

  //================================ make_dual_allocation_checkers ================================//

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<dual_allocation_checker<T, Getter, Predictions>>
  make_dual_allocation_checkers(const basic_allocation_info<T, Getter, Predictions>& info, Args&&... args)
  {
    using checker = dual_allocation_checker<T, Getter, Predictions>;
    return {checker{info, std::forward<Args>(args)...}};
  }

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions, class... Args>
    requires scoped_alloc<std::invoke_result_t<Getter, T>>
  [[nodiscard]]
  auto make_dual_allocation_checkers(const basic_allocation_info<T, Getter, Predictions>& info, Args&&... args)
  {
    return make_scoped_allocation_checkers<dual_allocation_checker>(info, std::forward<Args>(args)...);
  }

  //================================ make_allocation_checkers ================================//

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<allocation_checker<T, Getter, Predictions>>
  make_allocation_checkers(const basic_allocation_info<T, Getter, Predictions>& info, Args&&... args)
  {
    using checker = allocation_checker<T, Getter, Predictions>;
    return {checker{info, std::forward<Args>(args)...}};
  }

  template<movable_comparable T, alloc_getter<T> Getter, class Predictions, class... Args>
    requires scoped_alloc<std::invoke_result_t<Getter, T>>
  [[nodiscard]]
  auto make_allocation_checkers(const basic_allocation_info<T, Getter, Predictions>& info, Args&&... args)
  {
    return make_scoped_allocation_checkers<allocation_checker>(info, std::forward<Args>(args)...);
  }

  //================================ Variadic Allocation Checking ================================//

  template<test_mode Mode, class CheckFn, class Checker, class... Checkers>
  void check_allocation([[maybe_unused]] test_logger<Mode>& logger, CheckFn check, const Checker& checker, [[maybe_unused]] const Checkers&... moreCheckers)
  {
    check(checker);

    if constexpr (sizeof...(Checkers) > 0)
    {
      check_allocation(logger, check, moreCheckers...); 
    }
  }

  //================================ checks using dual_allocation_checker ================================//

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_no_allocation(std::string_view detail, test_logger<Mode>& logger, const T& x, const T& y, const dual_allocation_checker<T, Getter, Prediction>& checker, const dual_allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
        [detail, &logger, &x, &y](const auto& checker){
        checker.check_no_allocation(detail, logger, x, y);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_copy_assign_allocation(test_logger<Mode>& logger, const T& x, const T& y, const dual_allocation_checker<T, Getter, Prediction>& checker, const dual_allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &x, &y](const auto& checker){
        checker.check_copy_assign_y_to_x(logger, x, y);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_move_assign_allocation(test_logger<Mode>& logger, const T& x, const dual_allocation_checker<T, Getter, Prediction>& checker, const dual_allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &x](const auto& checker){
        checker.check_move_assign_y_to_x(logger, x);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_mutation_after_swap(test_logger<Mode>& logger, const T& x, const T& y, const dual_allocation_checker<T, Getter, Prediction>& checker, const dual_allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &x, &y](const auto& checker){
        checker.check_mutation_after_swap(logger, x, y);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, invocable<T&> Mutator, alloc_getter<T>... Getters, class... Predictions>
  void check_mutation_after_swap(test_logger<Mode>& logger, T& lhs, const T& rhs, const T& y, Mutator yMutator, dual_allocation_checker<T, Getters, Predictions>... checkers)
  {
    if(check("Mutation after swap pre-condition violated", logger, lhs == y))
    {    
      yMutator(lhs);
      check_mutation_after_swap(logger, lhs, rhs, checkers...);

      check("Mutation is not doing anything following copy then swap", logger, lhs != y);
    }
  }

  //================================ checks using allocation_checker ================================//

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_copy_x_allocation(test_logger<Mode>& logger, const T& container, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](const auto& checker){
        const auto prediction{checker.info().get_predictions().copy_x};
        checker.check("Unexpected allocation detected for copy construction (x)", logger, container, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_copy_y_allocation(test_logger<Mode>& logger, const T& container, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](const auto& checker){
        const auto prediction{checker.info().get_predictions().y.copy};
        checker.check("Unexpected allocation detected for copy construction (y)", logger, container, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_move_y_allocation(test_logger<Mode>& logger, const T& container, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](const auto& checker){
        const auto prediction{checker.info().get_predictions().move_allocs()};
        checker.check("Unexpected allocation detected for move construction (y)", logger, container, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_mutation_allocation(std::string_view priorOp, test_logger<Mode>& logger, const T& y, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [priorOp, &logger, &y](const auto& checker){
        const auto prediction{checker.info().get_predictions().mutation_allocs()};
        const auto mess{
          std::string{"Unexpected allocation detected following mutation after "}.append(priorOp)
        };

        checker.check(mess, logger, y, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_para_copy_y_allocation(test_logger<Mode>& logger, const T& container, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](const auto& checker){
        const auto prediction{checker.info().get_predictions().y.para_copy};
        checker.check("Unexpected allocation detected for para-copy construction (y)", logger, container, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }
  
  template<test_mode Mode, movable_comparable T, alloc_getter<T>... Getters, class... Predictions>
  void check_para_copy_y_allocation(test_logger<Mode>& logger, const T& container, std::tuple<allocation_checker<T, Getters, Predictions>...> checkers)
  {
    auto fn{[&logger,&container](auto&&... checkers){
        check_para_copy_y_allocation(logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, checkers);
  }

  template<test_mode Mode, movable_comparable T, class Prediction, alloc_getter<T> Getter, alloc_getter<T>... Getters, class... Predictions>
  void check_para_move_y_allocation(test_logger<Mode>& logger, const T& container, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](const auto& checker){
        const auto prediction{checker.info().get_predictions().para_move_allocs()};
        checker.check("Unexpected allocation detected for para-move construction (y)", logger, container, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }
  
  template<test_mode Mode, movable_comparable T, alloc_getter<T>... Getters, class... Predictions>
  void check_para_move_y_allocation(test_logger<Mode>& logger, const T& container, std::tuple<allocation_checker<T, Getters, Predictions>...> checkers)
  {
    auto fn{[&logger, &container](auto&&... checkers){
        check_para_move_y_allocation(logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    std::apply(fn, checkers);
  }

  template<test_mode Mode, movable_comparable T, alloc_getter<T> Getter, class Prediction, alloc_getter<T>... Getters, class... Predictions>
  void check_serialization_allocation(test_logger<Mode>& logger, const T& container, const allocation_checker<T, Getter, Prediction>& checker, const allocation_checker<T, Getters, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](const auto& checker){
        const alloc_prediction<null_allocation_event::serialization> prediction{};
        checker.check("Unexpected allocation detected for serialization (y)", logger, container, prediction);
      }
    };

    check_allocation(logger, checkFn, checker, moreCheckers...);
  }

  
  /*! \brief actions common to both move-only and regular types. */
  template<movable_comparable T>
  struct allocation_actions : precondition_actions<T>
  {
  private:
    template<class... Checkers>
    constexpr static bool do_check_swap()
    {
      return ((   std::allocator_traits<typename Checkers::allocator_type>::propagate_on_container_move_assignment::value
                  && std::allocator_traits<typename Checkers::allocator_type>::propagate_on_container_swap::value) && ... );
    }

  public:
    constexpr static bool has_post_comparison_action{true};
    constexpr static bool has_post_move_action{true};
    constexpr static bool has_post_move_assign_action{true};
    constexpr static bool has_post_swap_action{true};
    constexpr static bool has_post_serialization_action{true};

    using precondition_actions<T>::precondition_actions;

    template<test_mode Mode, alloc_getter<T>... Getters, class... Predictions>
    static bool post_comparison_action(test_logger<Mode>& logger, std::string_view op, const T& x, const T& y, const dual_allocation_checker<T, Getters, Predictions>&... checkers)
    {
      sentinel<Mode> s{logger, ""};

      check_no_allocation(std::string{"Unexpected allocation detected for operator"}.append(op), logger, x, y, checkers...);
      
      return !s.failure_detected();
    }

    template<test_mode Mode, alloc_getter<T>... Getters, class... Predictions>
    static void post_move_action(test_logger<Mode>& logger, const T& y, const allocation_checker<T, Getters, Predictions>&... checkers)
    {
      check_move_y_allocation(logger, y, checkers...);
    }

    template<test_mode Mode, invocable<T&> Mutator, alloc_getter<T>... Getters, class... Predictions>
    static void post_move_assign_action(test_logger<Mode>& logger, T& y, const T& yClone, Mutator yMutator, const dual_allocation_checker<T, Getters, Predictions>&... checkers)
    {
      check_move_assign_allocation(logger, y, checkers...);
      check_mutation_after_move("assignment", logger, y, yClone, std::move(yMutator), allocation_checker{checkers.info(), y}...);
    }

    template<test_mode Mode, alloc_getter<T>... Getters, class... Predictions>
    static void post_swap_action([[maybe_unused]] test_logger<Mode>& logger,
                                 [[maybe_unused]] const T& x,
                                 [[maybe_unused]] const T& y,
                                 [[maybe_unused]] const T&,
                                 [[maybe_unused]] const dual_allocation_checker<T, Getters, Predictions>&... checkers)
    {
      if constexpr (do_check_swap<dual_allocation_checker<T, Getters, Predictions>...>())
      {
        check_no_allocation("Unexpected allocation detected for swap", logger, y, x, checkers...);
      }
    }

    template<test_mode Mode, alloc_getter<T>... Getters, class... Predictions>
    static void post_serialization_action(test_logger<Mode>& logger, const T& y, const allocation_checker<T, Getters, Predictions>&... checkers)
    {
      check_serialization_allocation(logger, y, checkers...);
    }
  };

  /*! \name OverloadSet

      Functions for performing allocation checks which are common to types with both regular and
      move-only semantics. These functions provide an extra level of indirection in order that
      the current number of allocations may be acquired before proceeding
   */

  template<test_mode Mode, class Actions, movable_comparable T, alloc_getter<T>... Getters, class... Predictions>
  std::optional<T> check_move_construction(test_logger<Mode>& logger, const Actions& actions, T&& z, const T& y, const dual_allocation_checker<T, Getters, Predictions>&... checkers)
  {
    return do_check_move_construction(logger, actions, std::forward<T>(z), y, allocation_checker{checkers.info(), z}...);
  }

  template<test_mode Mode, class Actions, movable_comparable T, invocable<T&> Mutator, alloc_getter<T>... Getters, class... Predictions>
  void check_move_assign(test_logger<Mode>& logger, const Actions& actions, T& u, T&& v, const T& y, Mutator yMutator, const dual_allocation_checker<T, Getters, Predictions>&... checkers)
  {
    do_check_move_assign(logger, actions, u, std::forward<T>(v), y, std::move(yMutator), dual_allocation_checker{checkers.info(), u, v}...);
  }

  template<test_mode Mode, movable_comparable T, invocable<T&> Mutator, class... Checkers>
  void check_mutation_after_move(std::string_view moveType, test_logger<Mode>& logger, T& u, const T& y, Mutator yMutator, Checkers... checkers)
  {
    yMutator(u);
    check_mutation_allocation(moveType, logger, u, checkers...);

    const auto mess{
      std::string{"Mutation is not doing anything following move "}.append(moveType)
    };
    check(mess, logger, u != y);    
  }

  template<test_mode Mode, movable_comparable T, invocable<T&> Mutator, class... Checkers, std::size_t... I>
  void check_mutation_after_move(std::string_view moveType, test_logger<Mode>& logger, T& u, const T& y, Mutator yMutator, std::tuple<Checkers...> checkers, std::index_sequence<I...>)
  {
    check_mutation_after_move(moveType, logger, u, y, std::move(yMutator), std::get<I>(checkers)...);
  }

  template<test_mode Mode, movable_comparable T, invocable<T&> Mutator, class... Checkers>
  void check_mutation_after_move(std::string_view moveType, test_logger<Mode>& logger, T& u, const T& y, Mutator yMutator, std::tuple<Checkers...> checkers)
  {
    check_mutation_after_move(moveType, logger, u, y, std::move(yMutator), std::move(checkers), std::make_index_sequence<sizeof...(Checkers)>{});
  }

  template<test_mode Mode, class Actions, movable_comparable T, alloc_getter<T>... Getters, class... Predictions>
  bool check_serialization(test_logger<Mode>& logger, const Actions& actions, T&& u, const T& y, const dual_allocation_checker<T, Getters, Predictions>&... checkers)
  {
    return do_check_serialization(logger, actions, std::forward<T>(u), y, allocation_checker{checkers.info(), y}...);
  }
}
