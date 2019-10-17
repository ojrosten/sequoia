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
    
  enum class mutation_flavour {after_move_assign, after_swap, after_construction};
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
    allocation_checker(allocation_info<Container, Allocator> info)
      : m_Info{std::move(info)}
    {}

    allocation_checker(const Container& x, const Container& y, allocation_info<Container, Allocator> info)
      : m_Info{std::move(info)}
      , m_xCurrentCount{m_Info.count(x)}
      , m_yCurrentCount{m_Info.count(y)}
    {}

    template<class Logger>
    void check_copy_x(std::string_view description, Logger& logger, const Container& container)
    {
      m_xCurrentCount = check_copy(description, "(x)", logger, container, m_Info, m_xCurrentCount, m_Info.get_predictions().copy_x);
    }

    template<class Logger>
    void check_copy_y(std::string_view description, Logger& logger, const Container& container)
    {
      m_yCurrentCount = check_copy(description, "(y)", logger, container, m_Info, m_yCurrentCount, m_Info.get_predictions().y.copy);
    }

    template<class Logger>
    void check_copy_like_y(std::string_view description, Logger& logger, const Container& container)
    {
      check_copy_like(description, "(y)", logger, container, m_Info, m_Info.get_predictions().y.copy_like);
    }

    template<class Logger>
    void check_move_y(std::string_view description, Logger& logger, const Container& container)
    {
      check_move(description, "(y)", logger, container, m_Info, 0);
    }

    template<class Logger>
    void check_move_like_y(std::string_view description, Logger& logger, const Container& container)
    {
      check_move_like(description, "(y)", logger, container, m_Info, m_Info.get_predictions().y.move_like);
    }

    template<class Logger>
    void check_copy_assign_y_to_x(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer)
    {        
      int xPrediction{}, yPrediction{};
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
      {
        yPrediction = m_Info.get_predictions().assign_y_to_x.with_propagation;
      }
      else
      {
        xPrediction = m_Info.get_predictions().assign_y_to_x.without_propagation;
      }

      check_combined_allocation(description, "Copy assignment x allocations", "Copy assignment y allocations", logger, xContainer, yContainer, xPrediction, yPrediction);
    }

    template<class Logger>
    void check_move_assign_y_to_x(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer)
    {
      const bool copyLike{!std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
          && (m_Info.count(xContainer) != m_Info.count(yContainer))};
        
      const int xPrediction{copyLike ? m_Info.get_predictions().assign_y_to_x.without_propagation : 0}, yPrediction{};

      check_combined_allocation(description, "Move assignment x allocations", "Move assignment y allocations", logger, xContainer, yContainer, xPrediction, yPrediction);
    }

    template<class Logger>
    void check_mutation(std::string_view description, Logger& logger, const Container& yContainer)
    {
      m_xCurrentCount = check_allocation(description, "Mutation allocation after move-like construction", "", logger, yContainer, m_Info, m_xCurrentCount, m_Info.get_predictions().y.mutation);
    }

    template<mutation_flavour Flavour, class Logger>
    void check_mutation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer)
    {
      int xPrediction{}, yPrediction{};
      constexpr bool pred{[](){
          switch(Flavour)
            {
            case mutation_flavour::after_move_assign:
              return std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value;
            case mutation_flavour::after_swap:
              return std::allocator_traits<Allocator>::propagate_on_container_swap::value;
            case mutation_flavour::after_construction:
              return true;
            }
        }()
      };
        
      if constexpr(pred)
      {
        yPrediction = m_Info.get_predictions().y.mutation;
      }
      else
      {
        xPrediction = m_Info.get_predictions().y.mutation;
      }

      check_combined_allocation(description, "Mutation x allocations", "Mutation y allocations", logger, xContainer, yContainer, xPrediction, yPrediction);
    }     

    [[nodiscard]]
    allocation_info<Container, Allocator> get_allocation_info() const
    {
      return m_Info;
    }
  private:
    allocation_info<Container, Allocator> m_Info;
      
    int m_xCurrentCount{}, m_yCurrentCount{};

    template<class Logger>
    [[nodiscard]]
    static int check_allocation(std::string_view description, std::string_view detail, std::string_view suffix, Logger& logger, const Container& container, const allocation_info<Container, Allocator>& info, const int previous, const int prediction)
    {
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

      const auto current{info.count(container)};      
      auto message{combine_messages(combine_messages(description, detail), suffix)};

      check_equality(std::move(message), logger, current - previous, prediction);

      return current;
    }

    template<class Logger>
    void check_combined_allocation(std::string_view description, std::string_view xMessage, std::string_view yMessage, Logger& logger, const Container& xContainer, const Container& yContainer, const int xPrediction, const int yPrediction)
    {      
      typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};
      
      m_xCurrentCount = check_allocation(description, xMessage, "", logger, xContainer, m_Info, m_xCurrentCount, xPrediction);
      m_yCurrentCount = check_allocation(description, yMessage, "", logger, yContainer, m_Info, m_yCurrentCount, yPrediction);
    }

    template<class Logger>
    static int check_copy(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, allocation_info<Container, Allocator>& info, const int current, const int prediction)
    {
      return check_allocation(description, "Copy construction allocation", suffix, logger, container, info, current, prediction);
    }

    template<class Logger>
    void check_move(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, allocation_info<Container, Allocator>& info, const int prediction)
    {
      if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
      {
        m_yCurrentCount = check_allocation(description, "Move construction allocation", suffix, logger, container, info, m_yCurrentCount, prediction);
      }
      else
      {
        m_xCurrentCount = check_allocation(description, "Move construction allocation", suffix, logger, container, info, m_xCurrentCount, prediction);
      }
    }

    template<class Logger>
    void check_copy_like(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, allocation_info<Container, Allocator>& info, const int prediction)
    {
      m_yCurrentCount = check_allocation(description, "Copy-like construction allocation", suffix, logger, container, info, m_yCurrentCount, prediction);
    }

    template<class Logger>
    void check_move_like(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, allocation_info<Container, Allocator>& info, const int prediction)
    {
      m_xCurrentCount = check_allocation(description, "Move-like construction allocation", suffix, logger, container, info, m_xCurrentCount, prediction);
    }

    template<class Logger>
    void check_mutation(std::string_view description, std::string_view suffix, Logger& logger, const Container& container, allocation_info<Container, Allocator>& info, const int prediction)
    {
      m_yCurrentCount = check_allocation(description, "Mutation allocation", suffix, logger, container, info, m_yCurrentCount, prediction);
    }
  };

   //================================ Variadic Allocation Checking ================================//

  template<class Logger, class CheckFn, class Container, class Allocator, class... Allocators>
  void check_allocation(std::string_view description, Logger& logger, CheckFn check, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    check(add_type_info<Allocator>(description), checker);

    if constexpr (sizeof...(Allocators) > 0)
    {
      check_allocation(description, logger, check, moreCheckers...); 
    }
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_x_allocation(std::string_view description, Logger& logger, const Container& container, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_copy_x(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_y_allocation(std::string_view description, Logger& logger, const Container& container, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_copy_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_move_y_allocation(std::string_view description, Logger& logger, const Container& container, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_move_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }
      
  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_assign_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_copy_assign_y_to_x(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_move_assign_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_move_assign_y_to_x(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<mutation_flavour Flavour, class Logger, class Container, class Allocator, class... Allocators>
  void check_mutation_allocation(std::string_view description, Logger& logger, const Container& xContainer, const Container& yContainer, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &xContainer, &yContainer](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.template check_mutation<Flavour>(message, logger, xContainer, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_mutation_allocation(std::string_view description, Logger& logger, const Container& yContainer, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &yContainer](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_mutation(message, logger, yContainer);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_copy_like_y_allocation(std::string_view description, Logger& logger, const Container& container, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_copy_like_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  template<class Logger, class Container, class Allocator, class... Allocators>
  void check_move_like_y_allocation(std::string_view description, Logger& logger, const Container& container, allocation_checker<Container, Allocator>& checker, allocation_checker<Container, Allocators>&... moreCheckers)
  {
    auto checkFn{
      [&logger, &container](std::string_view message, allocation_checker<Container, Allocator>& checker){
        checker.check_move_like_y(message, logger, container);
      }
    };

    check_allocation(description, logger, checkFn, checker, moreCheckers...);
  }

  struct null_mutator
  {
    template<class T> void operator()(T&) noexcept {};
  };

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_like_allocations(std::string_view description, Logger& logger, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators>... checkers)
  {
    auto make{
      [description, &logger, &y](auto&... checkers){
        Container u{y, checkers.get_allocation_info().make_allocator()...};
        check_equality(combine_messages(description, "Copy-like construction"), logger, u, y);
        check_copy_like_y_allocation(description, logger, u, checkers...);

        return u;
      }
    };

    Container v{make(checkers...), checkers.get_allocation_info().make_allocator()...};
    // Remember to check if allocators equal
    check_equality(combine_messages(description, "Move-like construction"), logger, v, y);    
    check_move_like_y_allocation(description, logger, v, checkers...);

    yMutator(v);
    check_mutation_allocation(combine_messages(description, "mutation allocations after move-like construction"), logger, v, checkers...);
  }

  template<class Logger, class Container, class Mutator, class... Allocators>
  void check_allocations(std::string_view description, Logger& logger, const Container& x, const Container& y, Mutator yMutator, allocation_checker<Container, Allocators>&... checkers)
  {
    {      
      Container u{x}, v{y};
      check_copy_x_allocation(description, logger, x, checkers...);
      check_copy_y_allocation(description, logger, y, checkers...);

      u = std::move(v);
      check_move_assign_allocation(description, logger, x, y, checkers...);

      yMutator(u);
      check_mutation_allocation<mutation_flavour::after_move_assign>(combine_messages(description, "mutation after move assignment allocations"), logger, x, y, checkers...);

      check(combine_messages(description, "Mutation is not doing anything following copy constrution/broken value semantics"), logger, u != y);
    }

    if constexpr(do_swap<Allocators...>())
    {
      Container u{x}, v{y};
      check_copy_x_allocation(description, logger, u, checkers...);
      check_copy_y_allocation(description, logger, v, checkers...);

      using std::swap;
      swap(u, v);

      yMutator(u);
      check_mutation_allocation<mutation_flavour::after_swap>(combine_messages(description, "mutation after swap allocations"), logger, v, u, checkers...);
    }
    
    check_like_allocations(description, logger, y, yMutator, allocation_checker<Container, Allocators>{checkers.get_allocation_info()}...);
  }

  template<class Logger, class T, class Mutator, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_checker<T, Allocators>... allocationCheckers)
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
      check_copy_x_allocation(description, logger, z, allocationCheckers...);
      
    z = y;
    check_equality(combine_messages(description, "Copy assignment (from y)"), logger, z, y);
    check(combine_messages(description, "Inequality operator"), logger, z != x);
    if constexpr(checkAllocs)
      check_copy_assign_allocation(description, logger, x, y, allocationCheckers...);

    T w{std::move(z)};
    check_equality(combine_messages(description, "Move constructor"), logger, w, y);
    if constexpr(checkAllocs)
      check_move_y_allocation(description, logger, w, allocationCheckers...);

    if constexpr(checkAllocs)
    {
      check_allocations(description, logger, x, y, yMutator, allocationCheckers...);
    }

    z = T{x};
    check_equality(combine_messages(description, "Move assignment"), logger, z, x);

    if constexpr (impl::do_swap<Allocators...>())
    {
      using std::swap;
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
