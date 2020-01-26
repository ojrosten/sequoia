////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCheckersDetails.hpp
    \brief Implementation details for performing checks within the unit testing framework.
*/

#include "UnitTestLogger.hpp"
#include <scoped_allocator>

namespace sequoia::unit_testing
{
  template <class, template<class...> class T, class... Args>
  struct template_class_is_instantiable : std::false_type
  {};

  template <template<class...> class T, class... Args>
  struct template_class_is_instantiable<std::void_t<decltype(T<Args...>{})>, T, Args...>
    : std::true_type
  {};

  template <template<class...> class T, class... Args>
  constexpr bool template_class_is_instantiable_v{template_class_is_instantiable<std::void_t<>, T, Args...>::value};

  template<class T, class... U> std::string make_type_info();
  template<class T, class... U> std::string add_type_info(std::string_view description);

  template<class T> struct detailed_equality_checker;
  template<class T, class... Us> struct equivalence_checker;
  template<class T, class... Us> struct weak_equivalence_checker;

  struct equality_tag{};
  struct equivalence_tag{};
  struct weak_equivalence_tag{};
  
  template<class T> constexpr bool has_equivalence_checker_v{template_class_is_instantiable_v<equivalence_checker, T>};
  template<class T> constexpr bool has_weak_equivalence_checker_v{template_class_is_instantiable_v<weak_equivalence_checker, T>};
  template<class T> constexpr bool has_detailed_equality_checker_v{template_class_is_instantiable_v<detailed_equality_checker, T>};

  template<class Logger, class T, class S, class... U>
  bool check(std::string_view description, Logger& logger, equivalence_tag, const T& value, S&& s, U&&... u);

  template<class Logger, class T, class S, class... U>
  bool check(std::string_view description, Logger& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u);
  
  template<class Logger, class T> bool check_equality(std::string_view description, Logger& logger, const T& value, const T& prediction);

  struct individual_allocation_predictions;
  struct allocation_predictions;
  struct move_only_allocation_predictions;
  
  template<class Container, class Allocator, class Predictions=allocation_predictions>
  class allocation_info;
}

namespace sequoia::unit_testing::impl
{
  template<class EquivChecker, class Logger, class T, class S, class... U>
  bool check(std::string_view description, Logger& logger, const T& value, const S& s, const U&... u)
  {
    using sentinel = typename Logger::sentinel;

    const std::string message{
      add_type_info<S, U...>(
        combine_messages(description, "Comparison performed using:\n\t[" + demangle<EquivChecker>() + "]\n\tWith equivalent types:", "\n"))
    };
      
    sentinel r{logger, message};
    const auto previousFailures{logger.failures()};
    
    EquivChecker::check(message, logger, value, s, u...);
      
    return logger.failures() == previousFailures;
  }

  template<class... Allocators>
  constexpr bool do_swap() noexcept
  {
    return ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
             || std::allocator_traits<Allocators>::is_always_equal::value) && ... );
  }

  template<class Logger, class Tag, class Iter, class PredictionIter>
  bool check_range(std::string_view description, Logger& logger, Tag tag, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    typename Logger::sentinel r{logger, description};
    bool equal{true};

    using std::distance;
    const auto predictedSize{distance(predictionFirst, predictionLast)};
    if(check_equality(combine_messages(description, "Container size wrong", "\n"), logger, distance(first, last), predictedSize))
    {
      auto predictionIter{predictionFirst};
      auto iter{first};
      for(; predictionIter != predictionLast; ++predictionIter, ++iter)
      {
        std::string dist{std::to_string(std::distance(predictionFirst, predictionIter)).append("\n")};
        if(!check(combine_messages(description, "element ").append(std::move(dist)), logger, tag, *iter, *predictionIter)) equal = false;
      }
    }
    else
    {
      equal = false;
    }
      
    return equal;
  }

  //================================ Allocation Checking ================================//

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

    getter make_getter() const
    {
      return m_AllocatorGetter;
    }
  private:

    getter m_AllocatorGetter;
  };

  template<class Container, class Allocator, class Predictions=allocation_predictions>
  class allocation_checker
  {
  public:
    using alloc_info = allocation_info<Container, Allocator, Predictions>;
    
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
      const auto& predictions{m_Info.get_predictions()};

      int prediction{};
      if constexpr (std::is_same_v<Predictions, move_only_allocation_predictions>)
      {
        prediction = predictions.para_move;
      }
      else
      {
        prediction = predictions.y.para_move;
      }

      check_para_move(description, "(y)", logger, container, m_Info, prediction);
    }

    template<class Logger>
    void check_copy_assign_y_to_x(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer) const
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};
      
      int yPrediction{};
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
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

      const int xPrediction{copyLike ? m_Info.get_predictions().assign_y_to_x.without_propagation : 0};
      
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
      check_allocation(description, "Mutation allocation after move-like construction", "", logger, yContainer, m_Info, m_FirstCount, m_Info.get_predictions().y.mutation);
    }

    template<class Logger>
    void check_mutation_after_swap(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer) const
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      const auto prediction{m_Info.get_predictions().y.mutation};
      auto uCount{m_SecondCount}, vCount{m_FirstCount};
      
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_swap::value)
      {
        using std::swap;
        swap(uCount, vCount);        
      }

      check_allocation(description, "x allocations", "", logger, xContainer, m_Info, uCount, prediction);
      check_allocation(description, "y allocations", "", logger, yContainer, m_Info, vCount, 0);
    }     

    [[nodiscard]]
    const allocation_info<Container, Allocator>& info() const noexcept
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

  template<class Container, class Predictions, class... Allocators, class... Args, std::size_t... I>
  [[nodiscard]]
  auto make_allocation_checkers(const allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, std::index_sequence<I...>, Args&&... args)
  {
    return std::make_tuple(allocation_checker{std::forward<Args>(args)..., info.template unpack<I>()}...);
  }

  template<class Container, class Predictions, class... Args, class... Allocators>
  [[nodiscard]]
  auto make_allocation_checkers(const allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>& info, Args&&... args)
  {
    return make_allocation_checkers(info, std::make_index_sequence<sizeof...(Allocators)>{}, std::forward<Args>(args)...);
  }

  template<class Container, class Allocator, class Predictions, class... Args>
  [[nodiscard]]
  std::tuple<allocation_checker<Container, Allocator, Predictions>>
  make_allocation_checkers(const allocation_info<Container, Allocator, Predictions>& info, Args&&... args)
  {
    return {{std::forward<Args>(args)..., info}};
  }

  //================================ Variadic Allocation Checking ================================//

  template<class Logger, class CheckFn, class Container, class Predictions, class Allocator, class... Allocators>
  void check_allocation(std::string_view description, Logger& logger, CheckFn check, const allocation_checker<Container, Allocator, Predictions>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    check(add_type_info<Allocator>(description), checker);

    if constexpr (sizeof...(Allocators) > 0)
    {
      check_allocation(description, logger, check, moreCheckers...); 
    }
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_no_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_no_allocation(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_x_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_x(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_move_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
      
  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_assign_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, auto& checker){
        checker.check_copy_assign_y_to_x(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_move_assign_allocation(std::string_view description, Logger& logger, const Container& xContainer, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer](std::string_view message, auto& checker){
        checker.check_move_assign_y_to_x(message, logger, xContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_mutation_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation_after_swap(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_mutation_allocation(std::string_view description, Logger& logger, const Container& yContainer, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &yContainer](std::string_view message, auto& checker){
        checker.check_mutation(message, logger, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_para_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_para_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
  
  template<class Logger, class Container, class... Allocators>
  void check_para_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, std::tuple<allocation_checker<Container, Allocators>...> checkers)
  {
    auto fn{[description,&logger,&container](auto&&... checkers){
        check_para_copy_y_allocation(description, logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);
  }

  template<class Logger, class Container, class Predictions, class Allocator, class... Allocators>
  void check_para_move_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator, Predictions>& checker, const allocation_checker<Container, Allocators, Predictions>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_para_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
  
  template<class Logger, class Container, class Predictions, class... Allocators>
  void check_para_move_y_allocation(std::string_view description, Logger& logger, const Container& container, std::tuple<allocation_checker<Container, Allocators, Predictions>...> checkers)
  {
    auto fn{[description,&logger,&container](auto&&... checkers){
        check_para_move_y_allocation(description, logger, container, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    unpack_invoke(checkers, fn);    
  }

  struct null_mutator
  {
    template<class T> void operator()(T&) noexcept {};
  };
 
  template<class Logger, class T, class... Allocators>
  void check_copy_assign(std::string_view description, Logger& logger, T& z, const T& y, allocation_checker<T, Allocators>... checkers)
  {
    z = y;
    check_equality(combine_messages(description, "Copy assignment (from y)"), logger, z, y);

    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    if constexpr(checkAllocs)
      check_copy_assign_allocation(description, logger, z, y, checkers...);
  }

  template<class Logger, class T, class... Allocators>
  void check_move_construction(std::string_view description, Logger& logger, T&& z, const T& y, allocation_checker<T, Allocators>... checkers)
  {
    const T w{std::move(z)};
    check_equality(combine_messages(description, "Move constructor"), logger, w, y);

    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    if constexpr(checkAllocs)
      check_move_y_allocation(description, logger, w, allocation_checker<T, Allocators>{w, checkers.first_count(), checkers.info()}...);
  }

  template<class Logger, class T, class... Allocators>
  void check_move_assign(std::string_view description, Logger& logger, T& u, T&& v, const T& y, allocation_checker<T, Allocators>... checkers)
  {
    u = std::move(v);
    check_equality(combine_messages(description, "Move assignment (from y)"), logger, u, y);

    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    if constexpr(checkAllocs)
      check_move_assign_allocation(description, logger, u, checkers...);
  }

  template<class Logger, class T, class Mutator, class... Allocators>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, Logger& logger, T& u, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... checkers)
  {
    yMutator(u);
    auto mess{combine_messages("mutation after move", moveType)};
    check_mutation_allocation(combine_messages(description, std::move(mess)), logger, u, checkers...);

    mess = combine_messages("Mutation is not doing anything following move", moveType);
    check(combine_messages(description, std::move(mess)), logger, u != y);    
  }

  template<class Logger, class Container, class Mutator, class... Allocators, std::size_t... I>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, Logger& logger, Container& u, const Container& y, Mutator yMutator, std::tuple<allocation_checker<Container, Allocators>...> checkers, std::index_sequence<I...>)
  {
    check_mutation_after_move(description, moveType, logger, u, y, std::move(yMutator), std::get<I>(checkers)...);
  }

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_mutation_after_move(std::string_view description, std::string_view moveType, Logger& logger, Container& u, const Container& y, Mutator yMutator, std::tuple<allocation_checker<Container, Allocators>...> checkers)
  {
    check_mutation_after_move(description, moveType, logger, u, y, std::move(yMutator), std::move(checkers), std::make_index_sequence<sizeof...(Allocators)>{});
  }
  
  template<class Logger, class T, class Mutator, class... Allocators>
  void check_mutation_after_swap(std::string_view description, Logger& logger, T& u, const T& v, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... checkers)
  {
    yMutator(u);
    check_mutation_allocation(combine_messages(description, "mutation after swap"), logger, u, v, checkers...);

    check(combine_messages(description, "Mutation is not doing anything following copy then swap"), logger, u != y);    
  }

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_para_constructor_allocations(std::string_view description, Logger& logger, const Container& y, Mutator yMutator, const allocation_info<Container, Allocators>&... info)
  {    
    if constexpr(sizeof...(Allocators) > 0)
    {
      auto make{
        [description, &logger, &y](auto&... info){
          Container u{y, info.make_allocator()...};
          check_equality(combine_messages(description, "Copy-like construction"), logger, u, y);
          check_para_copy_y_allocation(description, logger, u, std::tuple_cat(make_allocation_checkers(info)...));

          return u;
        }
      };

      Container v{make(info...), info.make_allocator()...};

      check_equality(combine_messages(description, "Move-like construction"), logger, v, y);    
      check_para_move_y_allocation(description, logger, v, std::tuple_cat(make_allocation_checkers(info)...));
      check_mutation_after_move(description, "allocation assignment", logger, v, y, yMutator, std::tuple_cat(make_allocation_checkers(info, v, 0)...));
    }
  }

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_swap_allocations(std::string_view description, Logger& logger, Container& u, Container& v, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators>... checkers)
  {
    using std::swap;
    swap(u, v);

    check_mutation_after_swap(description, logger, u, v, y, yMutator, allocation_checker<Container, Allocators>{u, v, checkers.info()}...);
  }

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_allocations(std::string_view description, Logger& logger, const Container& x, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators>... checkers)
  {
    {      
      Container u{x}, v{y};
      check_copy_x_allocation(description, logger, u, allocation_checker<Container, Allocators>{u, checkers.first_count(), checkers.info()}...);
      check_copy_y_allocation(description, logger, v, allocation_checker<Container, Allocators>{v, checkers.second_count(), checkers.info()}...);

      // u = std::move(v) (= y)
      check_move_assign(description, logger, u, std::move(v), y, allocation_checker<Container, Allocators>{u, y, checkers.info()}...);

      check_mutation_after_move(description, "assignment", logger, u, y, yMutator, allocation_checker<Container, Allocators>{u, 0, checkers.info()}...);
    }

    if constexpr(do_swap<Allocators...>())
    {
      Container u{x}, v{y};

      check_swap_allocations(description, logger, u, v, y, yMutator, allocation_checker<Container, Allocators>{u, v, checkers.info()}...);
    }
  }

  template<class Logger, class T, class... Allocators>
  bool check_regular_preconditions(std::string_view description, Logger& logger, const T& x, const T& y, allocation_checker<T, Allocators>... checkers)
  {
    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x))      return false;
    if constexpr (checkAllocs)
    {
      check_no_allocation(combine_messages(description, "no allocation for operator=="), logger, x, checkers...);
    }
    
    if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x))) return false;
    if constexpr (checkAllocs)
    {
      check_no_allocation(combine_messages(description, "no allocation for operator!="), logger, x, checkers...);
    }

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y)) return false;

    return true;
  }
  
  template<class Logger, class T, class Mutator, class... Allocators>
  bool check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... checkers)
  {
    constexpr bool nullMutator{std::is_same_v<Mutator, null_mutator>};
    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    static_assert(!checkAllocs || !nullMutator);
        
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    if(!check_regular_preconditions(description, logger, x, y, allocation_checker<T, Allocators>{x, checkers.first_count(), checkers.info()}...))
      return false;
        
    T z{x};
    if constexpr(checkAllocs)
    {
      check_copy_x_allocation(description, logger, z, allocation_checker<T, Allocators>{z, checkers.first_count(), checkers.info()}...);
    }    
    check_equality(combine_messages(description, "Copy constructor (x)"), logger, z, x);
    check(combine_messages(description, "Equality operator"), logger, z == x);

    // z = y
    check_copy_assign(description, logger, z, y, allocation_checker<T, Allocators>{z, y, checkers.info()}...);
    check(combine_messages(description, "Inequality operator"), logger, z != x);

    // w = std::move(z)
    check_move_construction(description, logger, std::move(z), y, allocation_checker<T, Allocators>{z, 0, checkers.info()}...);

    z = T{x};
    check_equality(combine_messages(description, "Move assignment"), logger, z, x);

    if constexpr (impl::do_swap<Allocators...>())
    {
      using std::swap;
      T w{y};
      swap(z, w);
      check_equality(combine_messages(description, "Swap"), logger, z, y);
      check_equality(combine_messages(description, "Swap"), logger, w, x);
    }

    if constexpr(!nullMutator)
    {
      T v{y};
      yMutator(v);

      check(combine_messages(description, "Mutation is not doing anything following copy constrution/broken value semantics"), logger, v != y);

      v = y;
      check_equality(combine_messages(description, "Copy assignment"), logger, v, y);

      yMutator(v);

      check(combine_messages(description, "Mutation is not doing anything following copy assignment/ broken value semantics"), logger, v != y);
    }

    if constexpr(checkAllocs)
    {
      check_allocations(description, logger, x, y, yMutator, allocation_checker<T, Allocators>{x, y, checkers.info()}...);
    }

    return true;
  }

  template<class Logger, class T, class Mutator, class... Allocators, std::size_t... I>
  bool check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, std::tuple<allocation_checker<T, Allocators>...> checkers, std::index_sequence<I...>)
  {
    return impl::check_regular_semantics(description, logger, x, y, std::move(yMutator), std::get<I>(checkers)...);
  }

  template<class Logger, class T, class Mutator, class... Allocators>
  bool check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, std::tuple<allocation_checker<T, Allocators>...> checkers)
  {
    auto fn{
      [description,&logger,&x,&y,m=std::move(yMutator)](auto&&... checkers){
        return impl::check_regular_semantics(description, logger, x, y, m, std::forward<decltype(checkers)>(checkers)...);
      }
    };

    return unpack_invoke(checkers, fn);
  }
}
