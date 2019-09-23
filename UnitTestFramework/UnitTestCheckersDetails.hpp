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
  
  template<class Allocator>
  class allocation_info;
    
  enum class mutation_flavour {after_move_assign, after_swap};
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

  template<class Logger, class Check, class Allocator, class... Allocators>
  void check_allocation(std::string_view description, Logger& logger, Check check, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    check(add_type_info<Allocator>(description), allocationInfo);

    if constexpr (sizeof...(Allocators) > 0)
    {
      check_allocation(description, logger, check, moreInfo...); 
    }
  }

  template<class Logger, class Check, class... AllAllocators, class Allocator, class... Allocators>
  void check_allocation(std::string_view description, Logger& logger, Check check, const std::tuple<AllAllocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    check(add_type_info<Allocator>(description), std::get<sizeof...(AllAllocators)-1-sizeof...(Allocators)>(allocators), allocationInfo);

    if constexpr (sizeof...(Allocators) > 0)
    {
      check_allocation(description, logger, check, allocators, moreInfo...); 
    }
  }
      
  template<class Logger, class Allocator, class... Allocators>
  void check_copy_x_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, auto& info){
        info.check_copy_x(message, logger);
      }
    };

    check_allocation(description, logger, checker, allocationInfo, moreInfo...);
  }

  template<class Logger, class Allocator, class... Allocators>
  void check_copy_y_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, auto& info){
        info.check_copy_y(message, logger);
      }
    };

    check_allocation(description, logger, checker, allocationInfo, moreInfo...);
  }

  template<class Logger, class Allocator, class... Allocators>
  void check_move_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, auto& info){
        info.check_move(message, logger);
      }
    };

    check_allocation(description, logger, checker, allocationInfo, moreInfo...);
  }
      
  template<class Logger, class Allocator, class... Allocators>
  void check_copy_assign_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, auto& info){
        info.check_copy_assign_y_to_x(message, logger);
      }
    };

    check_allocation(description, logger, checker, allocationInfo, moreInfo...);
  }

  template<class Logger, class Allocator, class... Allocators>
  void check_move_assign_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, auto& info){
        info.check_move_assign_y_to_x(message, logger);
      }
    };

    check_allocation(description, logger, checker, allocationInfo, moreInfo...);
  }

  template<mutation_flavour Flavour, class Logger, class Allocator, class... Allocators>
  void check_mutation_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, auto& info){
        info.template check_mutation<Flavour>(message, logger);
      }
    };

    check_allocation(description, logger, checker, allocationInfo, moreInfo...);
  }

  template<class Logger, class Allocator, class... Allocators>
  void check_copy_like_x_allocation(std::string_view description, Logger& logger, const std::tuple<Allocator, Allocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, const auto& alloc, const auto& info){
        check_equality(message, logger, alloc.allocs(), info.predictions().copy_x);
      }
    };

    check_allocation(description, logger, checker, allocators, allocationInfo, moreInfo...);
  }

  template<class Logger, class Allocator, class... Allocators>
  void check_move_like_x_allocation(std::string_view description, Logger& logger, const std::tuple<Allocator, Allocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, const auto& alloc, const auto& info){
        check_equality(message, logger, alloc.allocs(), info.predictions().copy_x);
      }
    };

    check_allocation(description, logger, checker, allocators, allocationInfo, moreInfo...);
  }

  template<class Logger, class Allocator, class... Allocators>
  void check_mutation_allocation(std::string_view description, Logger& logger, const std::tuple<Allocator, Allocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
  {
    auto checker{
      [&logger](std::string_view message, const auto& alloc, const auto& info){
        check_equality(message, logger, alloc.allocs(), info.predictions().mutation + info.predictions().copy_x);
      }
    };

    check_allocation(description, logger, checker, allocators, allocationInfo, moreInfo...);
  }

  struct null_mutator
  {
    template<class T> void operator()(T&) noexcept {};
  };

  template<class Logger, class T, class Mutator, class... Allocators, std::size_t... I>
  void check_allocations(std::index_sequence<I...>, std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<Allocators>... allocationInfo)
  {
    {
      T u{x}, v{y};
      check_copy_x_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);
      check_copy_y_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);

      u = std::move(v);
      check_move_assign_allocation(combine_messages(description, "Move assign allocations"), logger, allocationInfo...);

      yMutator(u);
      check_mutation_allocation<mutation_flavour::after_move_assign>(combine_messages(description, "mutation after move assignment allocations"), logger, allocationInfo...);

      check(combine_messages(description, "Mutation is not doing anything following copy constrution/broken value semantics"), logger, u != y);
    }

    if constexpr(do_swap<Allocators...>())
    {
      T u{x}, v{y};
      check_copy_x_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);
      check_copy_y_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);

      using std::swap;
      swap(u, v);

      yMutator(u);
      check_mutation_allocation<mutation_flavour::after_swap>(combine_messages(description, "mutation after swap allocations"), logger, allocationInfo...);
    }

    {
      auto make{
        [description, &logger, &x](auto&... info){
          std::tuple<Allocators...> allocs{};

          T u{x, std::get<I>(allocs)...};
          check_equality(combine_messages(description, "Copy-like construction"), logger, u, x);
          check_copy_like_x_allocation(combine_messages(description, "Copy-like allocations"), logger, allocs, info...);

          return u;
        }
      };

      std::tuple<Allocators...> allocs{};

      T v{make(allocationInfo...), std::get<I>(allocs)...};
      check_equality(combine_messages(description, "Move-like construction"), logger, v, x);
      check_move_like_x_allocation(combine_messages(description, "Move-like allocations"), logger, allocs, allocationInfo...);

      yMutator(v);
      check_mutation_allocation(combine_messages(description, "mutation allocations after move-like construction"), logger, allocs, allocationInfo...);
    }
  }

  template<class Logger, class T, class Mutator, class... Allocators>
  void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<Allocators>... allocationInfo)
  {
    constexpr bool nullMutator{std::is_same_v<Mutator, null_mutator>};
    constexpr bool checkAllocs{sizeof...(Allocators) > 0};
    static_assert(!checkAllocs || !nullMutator);
        
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x)) return;
    if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x))) return;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y)) return;

    T z{x};
    check_equality(combine_messages(description, "Copy constructor"), logger, z, x);
    check(combine_messages(description, "Equality operator"), logger, z == x);
    if constexpr(checkAllocs)
      check_copy_x_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);
      
    z = y;
    check_equality(combine_messages(description, "Copy assignment"), logger, z, y);
    check(combine_messages(description, "Inequality operator"), logger, z != x);
    if constexpr(checkAllocs)
      check_copy_assign_allocation(combine_messages(description, "Copy assign allocations"), logger, allocationInfo...);

    T w{std::move(z)};
    check_equality(combine_messages(description, "Move constructor"), logger, w, y);
    if constexpr(checkAllocs)
      check_move_allocation(combine_messages(description, "Move allocations"), logger, allocationInfo...);

    if constexpr(checkAllocs)
    {
      check_allocations(std::make_index_sequence<sizeof...(Allocators)>{}, description, logger, x, y, yMutator, allocationInfo...);
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
