////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"
#include <execution>
#include <iterator>

namespace experimental
{
  // From pstl/glue_algorithm_impl.h
  template <class _ExecutionPolicy, class _ForwardIterator, class _Predicate>
  __pstl::__internal::__enable_if_execution_policy<_ExecutionPolicy, bool>
  constexpr any_of(_ExecutionPolicy&& __exec, _ForwardIterator __first, _ForwardIterator __last, _Predicate __pred)
  {
    if consteval
    {
      return std::any_of(__first, __last, __pred);
    }
    else
    {
      return __pstl::__internal::__pattern_any_of(
        std::forward<_ExecutionPolicy>(__exec), __first, __last, __pred,
        __pstl::__internal::__is_vectorization_preferred<_ExecutionPolicy, _ForwardIterator>(__exec),
        __pstl::__internal::__is_parallelization_preferred<_ExecutionPolicy, _ForwardIterator>(__exec));
    }
  }

  // From pstl/glue_numeric_impl.h
  template <class _ExecutionPolicy, class _ForwardIterator>
  constexpr __pstl::__internal::__enable_if_execution_policy<_ExecutionPolicy,
                                                   typename std::iterator_traits<_ForwardIterator>::value_type>
  reduce(_ExecutionPolicy&& __exec, _ForwardIterator __first, _ForwardIterator __last)
  {
    if consteval
    {
      return std::reduce(__first, __last);
    }
    else
    {
      typedef typename std::iterator_traits<_ForwardIterator>::value_type _ValueType;
      return transform_reduce(std::forward<_ExecutionPolicy>(__exec), __first, __last, _ValueType{},
                              std::plus<_ValueType>(), __pstl::__internal::__no_op());
    }
  }

  constexpr bool contains(std::vector<int> v, int num)
  {
    return experimental::any_of(std::execution::par, v.begin(), v.end(), [num](int i){ return i == num; });
  }

  template<class ExecPolicy>
  constexpr int sum(ExecPolicy&& execPol, std::vector<int> v)
  {
    return experimental::reduce(std::forward<ExecPolicy>(execPol), v.begin(), v.end());
  }
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path experimental_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void experimental_test::run_tests()
  {
    static_assert(experimental::contains({1,2,3,4,5,6}, 4));
    static_assert(!experimental::contains({1,2,3,4,5,6}, 42));

    static_assert(experimental::sum(std::execution::seq,       {1,2,3})       == 6);
    static_assert(experimental::sum(std::execution::par,       {1,2,3,4})     == 10);
    static_assert(experimental::sum(std::execution::par_unseq, {1,2,3,4,5})   == 15);
    static_assert(experimental::sum(std::execution::unseq,     {1,2,3,4,5,6}) == 21);
    check(equality, report_line("Check runtime result"), experimental::sum(std::execution::par, {1,2,3,4,5}), 15);

    check_relative_performance(
      report_line("Check acceleration still works with a speed up of at least 2"),
      [](){ experimental::sum(std::execution::par, std::vector<int>(1000000,1)); },
      [](){ experimental::sum(std::execution::seq, std::vector<int>(1000000,1)); },
      2.0,
      10.0
    );
  }
}
