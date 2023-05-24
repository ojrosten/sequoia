////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"
#include <execution>

namespace experimental
{
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

  constexpr bool contains(std::vector<int> v, int num)
  {
    return experimental::any_of(std::execution::par, v.begin(), v.end(), [num](int i){ return i == num; });
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
  }
}
