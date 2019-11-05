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

namespace sequoia::unit_testing
{
  template<class T, class... U> std::string make_type_info();
  template<class T, class... U> std::string add_type_info(std::string_view description);

  struct equality_tag{};
  struct equivalence_tag{};
  struct weak_equivalence_tag{};

  template<class Logger, class T, class S, class... U>
  bool check(std::string_view description, Logger& logger, equivalence_tag, const T& value, S&& s, U&&... u);

  template<class Logger, class T, class S, class... U>
  bool check(std::string_view description, Logger& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u);
  
  template<class Logger, class T> bool check_equality(std::string_view description, Logger& logger, const T& value, const T& prediction);

  struct individual_allocation_predictions;
  struct allocation_predictions;
  
  template<class Container, class Allocator>
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
    if(check_equality(combine_messages(description, "container size wrong"), logger, distance(first, last), predictedSize))
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
  class allocation_checker
  {
  public:
    using alloc_info = allocation_info<Container, Allocator>;
    
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
    void check_copy_x(std::string_view description, Logger& logger, const Container& container) const
    {
      // Sort out propagation for Copy Construction!
      check_copy(description, "(x)", logger, container, m_Info, m_SecondCount, m_Info.get_predictions().copy_x);
    }

    template<class Logger>
    void check_copy_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_copy(description, "(y)", logger, container, m_Info, m_SecondCount, m_Info.get_predictions().y.copy);
    }

    template<class Logger>
    void check_copy_alloc_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_copy_alloc(description, "(y)", logger, container, m_Info, m_Info.get_predictions().y.copy_alloc);
    }

    template<class Logger>
    void check_move_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_move(description, "(y)", logger, container, m_Info, 0);
    }

    template<class Logger>
    void check_move_alloc_y(std::string_view description, Logger& logger, const Container& container) const
    {
      check_move_alloc(description, "(y)", logger, container, m_Info, m_Info.get_predictions().y.move_alloc);
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
    allocation_info<Container, Allocator> info() const noexcept
    {
      return m_Info;
    }
  private:
    allocation_info<Container, Allocator> m_Info;
      
    int m_FirstCount{}, m_SecondCount{};
    bool m_AllocatorsEqual{};

    template<class Logger>
    static void check_allocation(std::string_view description, std::string_view detail, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int previous, const int prediction)
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      const auto current{info.count(container)};      
      auto message{combine_messages(combine_messages(description, detail), suffix)};

      check_equality(std::move(message), logger, current - previous, prediction);
    }

    template<class Logger>
    static void check_copy(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int previous, const int prediction)
    {
      check_allocation(description, "Copy construction allocation", suffix, logger, container, info, previous, prediction);
    }

    template<class Logger>
    void check_move(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int prediction) const
    {
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
      {
        check_allocation(description, "Move construction allocation", suffix, logger, container, info, m_SecondCount, prediction);
      }
      else
      {
        check_allocation(description, "Move construction allocation", suffix, logger, container, info, m_FirstCount, prediction);
      }
    }

    template<class Logger>
    void check_copy_alloc(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int prediction) const
    {
      check_allocation(description, "Copy-like construction allocation", suffix, logger, container, info, m_SecondCount, prediction);
    }

    template<class Logger>
    void check_move_alloc(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int prediction) const
    {
      check_allocation(description, "Move-like construction allocation", suffix, logger, container, info, m_FirstCount, prediction);
    }

    template<class Logger>
    void check_mutation(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int prediction) const
    {
      check_allocation(description, "Mutation allocation", suffix, logger, container, info, m_SecondCount, prediction);
    }
  };

   //================================ Variadic Allocation Checking ================================//

  template<class Logger, class CheckFn, class Container, class Allocator, class... Allocators>
  void check_allocation(std::string_view description, Logger& logger, CheckFn check, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    check(add_type_info<Allocator>(description), checker);

    if constexpr (sizeof...(Allocators) > 0)
    {
      check_allocation(description, logger, check, moreCheckers...); 
    }
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
  void check_copy_alloc_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_copy_alloc_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_move_alloc_y_allocation(std::string_view description, Logger& logger, const Container& container, const allocation_checker<Container, Allocator>& checker, const allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, auto& checker){
        checker.check_move_alloc_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
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
  void check_mutation_after_move_assign(std::string_view description, Logger& logger, T& u, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... checkers)
  {
    yMutator(u);
    check_mutation_allocation(combine_messages(description, "mutation after move assignment allocations"), logger, u, checkers...);

    check(combine_messages(description, "Mutation is not doing anything following copy then move assign"), logger, u != y);    
  }
  
  template<class Logger, class T, class Mutator, class... Allocators>
  void check_mutation_after_swap(std::string_view description, Logger& logger, T& u, const T& v, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... checkers)
  {
    yMutator(u);
    check_mutation_allocation(combine_messages(description, "mutation after swap allocations"), logger, u, v, checkers...);

    check(combine_messages(description, "Mutation is not doing anything following copy then swap"), logger, u != y);    
  }

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_allocations(std::string_view description, Logger& logger, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators>... checkers)
  {
    auto make{
      [description, &logger, &y](auto&... checkers){
        Container u{y, checkers.info().make_allocator()...};
        check_equality(combine_messages(description, "Copy-like construction"), logger, u, y);
        check_copy_alloc_y_allocation(description, logger, u, checkers...);

        return u;
      }
    };

    Container v{make(checkers...), checkers.info().make_allocator()...};

    check_equality(combine_messages(description, "Move-like construction"), logger, v, y);    
    check_move_alloc_y_allocation(description, logger, v, checkers...);

    yMutator(v);
    check_mutation_allocation(combine_messages(description, "mutation allocations after move-like construction"), logger, v, checkers...);
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

      check_mutation_after_move_assign(description, logger, u, y, yMutator, allocation_checker<Container, Allocators>{u, 0, checkers.info()}...);
    }

    if constexpr(do_swap<Allocators...>())
    {
      Container u{x}, v{y};

      check_swap_allocations(description, logger, u, v, y, yMutator, allocation_checker<Container, Allocators>{u, v, checkers.info()}...);
    }
    
    //check_allocations(description, logger, y, yMutator, allocation_checker<Container, Allocators>{y, 0, checkers.info()}...);
  }

  template<class Logger, class T, class Mutator, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... checkers)
  {
    constexpr bool nullMutator{std::is_same_v<Mutator, null_mutator>};
    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    static_assert(!checkAllocs || !nullMutator);
        
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x)) return;
    if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x))) return;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y)) return;
    
    T z{x};
    check_equality(combine_messages(description, "Copy constructor (x)"), logger, z, x);
    check(combine_messages(description, "Equality operator"), logger, z == x);
    if constexpr(checkAllocs)
    {
      check_copy_x_allocation(description, logger, z, allocation_checker<T, Allocators>{z, checkers.first_count(), checkers.info()}...);
    }

    // z = y
    check_copy_assign(description, logger, z, y, allocation_checker<T, Allocators>{z, y, checkers.info()}...);
    check(combine_messages(description, "Inequality operator"), logger, z != x);

    // w = std::move(z)
    check_move_construction(description, logger, std::move(z), y, allocation_checker<T, Allocators>{z, 0, checkers.info()}...);

    if constexpr(checkAllocs)
    {
      check_allocations(description, logger, x, y, yMutator, allocation_checker<T, Allocators>{x, y, checkers.info()}...);
    }

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
  }  
}
