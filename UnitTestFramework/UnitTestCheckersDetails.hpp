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
  
  template<class Logger, class T> bool check_equality(std::string_view description, Logger& logger, const T& value, const T& prediction);
}

namespace sequoia::unit_testing::impl
{
  template<class...> struct do_swap;

  template<> struct do_swap<>
  {
    constexpr static bool value{true};
  };

  struct null_mutator
  {
    template<class T> constexpr void operator()(T&) noexcept {};
  };

  struct pre_condition_actions
  {
    template<class Logger, class T, class... Allocators>
    static bool check_preconditions(std::string_view description, Logger& logger, const T& x, const T& y)
    {
      return check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y);
    }
  };
  
  struct default_actions : pre_condition_actions
  {
    constexpr static bool has_post_equality_action{};
    constexpr static bool has_post_nequality_action{};
    constexpr static bool has_post_copy_action{};
    constexpr static bool has_post_copy_assign_action{};
    constexpr static bool has_post_move_action{};
    constexpr static bool has_post_move_assign_action{};
    constexpr static bool has_additional_action{};
  };
 

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
        if(!check(combine_messages(description, "element ", "\n").append(std::move(dist)), logger, tag, *iter, *predictionIter)) equal = false;
      }
    }
    else
    {
      equal = false;
    }
      
    return equal;
  }

  template<class Logger, class Actions, class T, class... Args>
  bool do_check_preconditions(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y, const Args&... args)
  {
    if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x))
      return false;

    if constexpr (Actions::has_post_equality_action)
    {
      actions.post_equality_action(combine_messages(description, "no allocation for operator=="), logger, x, y, args...);
    }
    
    if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x)))
      return false;

    if constexpr (Actions::has_post_equality_action)
    {
      actions.post_nequality_action(combine_messages(description, "no allocation for operator!="), logger, x, y, args...);
    }

    return actions.check_preconditions(description, logger, x, y);
  }

  template<class Logger, class Actions, class T>
  bool check_preconditions(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y)
  {
    return do_check_preconditions(description, logger, actions, x, y);
  }


  template<class Logger, class Actions, class T, class... Args>
  void do_check_copy_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, const T& y, const Args&... args)
  {
    z = y;
    check_equality(combine_messages(description, "Copy assignment (from y)"), logger, z, y);

    if constexpr(Actions::has_post_copy_assign_action)
    {
      actions.post_copy_assign_action(description, logger, z, y, args...);
    }
  }
  
  template<class Logger, class Actions, class T>
  void check_copy_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, const T& y)
  {
    do_check_copy_assign(description, logger, actions, z, y);   
  }

  template<class Logger, class Actions, class T, class... Args>
  void do_check_move_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, T&& y, const T& yClone, const Args&... args)
  {
    z = std::move(y);
    check_equality(combine_messages(description, "Move assignment (from y)"), logger, z, yClone);

    if constexpr(Actions::has_post_move_assign_action)
    {
      actions.post_move_assign_action(description, logger, z, args...);
    }
  }
  
  template<class Logger, class Actions, class T>
  void check_move_assign(std::string_view description, Logger& logger, const Actions& actions, T& z, T&& y, const T& yClone)
  {
    do_check_move_assign(description, logger, actions, z, std::forward<T>(y), yClone);   
  }

  template<class Logger, class Actions, class T, class... Args>
  void do_check_move_construction(std::string_view description, Logger& logger, const Actions& actions, T&& z, const T& y, const Args&... args)
  {
    const T w{std::move(z)};
    check_equality(combine_messages(description, "Move constructor"), logger, w, y);

    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(description, logger, w, args...);
    }
  }

  template<class Logger, class Actions, class T>
  void check_move_construction(std::string_view description, Logger& logger, const Actions& actions, T&& z, const T& y)
  {
    do_check_move_construction(description, logger, actions, std::forward<T>(z), y);
  }
  
  template<class Logger, class Actions, class T, class Mutator, class... Args>
  bool check_regular_semantics(std::string_view description, Logger& logger, const Actions& actions, const T& x, const T& y, Mutator yMutator, const Args&... args)
  {        
    typename Logger::sentinel s{logger, add_type_info<T>(description)};
    
    // Preconditions
    if(!check_preconditions(description, logger, actions, x, y, args...))
      return false;
        
    T z{x};
    if constexpr(Actions::has_post_copy_action)
    {
      actions.post_copy_action(description, logger, z, args...);
    }
    check_equality(combine_messages(description, "Copy constructor (x)"), logger, z, x);
    check(combine_messages(description, "Equality operator"), logger, z == x);

    // z = y
    check_copy_assign(description, logger, actions, z, y, args...);
    check(combine_messages(description, "Inequality operator"), logger, z != x);

    // move construction
    check_move_construction(description, logger, actions, std::move(z), y, args...);

    // move assign
    z = T{x};
    check_equality(combine_messages(description, "Move assignment"), logger, z, x);

    if constexpr (do_swap<Args...>::value)
    {
      using std::swap;
      T w{y};
      swap(z, w);
      check_equality(combine_messages(description, "Swap"), logger, z, y);
      check_equality(combine_messages(description, "Swap"), logger, w, x);
    }

    if constexpr(!std::is_same_v<std::decay_t<Mutator>, null_mutator>)
    {
      T v{y};
      yMutator(v);

      check(combine_messages(description, "Mutation is not doing anything following copy constrution/broken value semantics"), logger, v != y);

      v = y;
      check_equality(combine_messages(description, "Copy assignment"), logger, v, y);

      yMutator(v);

      check(combine_messages(description, "Mutation is not doing anything following copy assignment/ broken value semantics"), logger, v != y);
    }

    if constexpr(Actions::has_additional_action)
    {
      actions.additional_action(description, logger, x, y, yMutator, args...);
    }

    return true;
  }

  template<class Logger, class Actions, class T, class... Args>
  bool check_regular_semantics(std::string_view description, Logger& logger, const Actions& actions, T&& x, T&& y, const T& xClone, const T& yClone, const Args&... args)
  {
    typename Logger::sentinel s{logger, add_type_info<T>(description)};

    // Preconditions
    if(!check_preconditions(description, logger, actions, x, y, args...))
      return false;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and xClone are assumed to be equal"), logger, x == xClone)) return false;

    if(!check(combine_messages(description, "Precondition - for checking regular semantics, y and yClone are assumed to be equal"), logger, y == yClone)) return false;

    T z{std::move(x)};
    if constexpr(Actions::has_post_move_action)
    {
      actions.post_move_action(description, logger, z, args...);
    }
    check_equality(combine_messages(description, "Move constructor"), logger, z, xClone);
        
    x = std::move(y);
    check_equality(combine_messages(description, "Move assignment"), logger, x, yClone);

    if constexpr (do_swap<Args...>::value)
    {
      using std::swap;
      swap(z, x);
      check_equality(combine_messages(description, "Swap"), logger, x, xClone);
      check_equality(combine_messages(description, "Swap"), logger, z, yClone);

      y = std::move(z);
    }
    else
    {
      y = std::move(x);
    }

    return check_equality(combine_messages(description, "Post condition: y restored"), logger, y, yClone);
  }
}
