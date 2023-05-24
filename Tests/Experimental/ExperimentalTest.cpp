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

  constexpr bool foo()
  {
    std::vector<int> v{1,2,3,4,5,6};
    return experimental::any_of(std::execution::par, v.begin(), v.end(), [](int i){ return i == 4; });
  }
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path experimental_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void experimental_test::run_tests()
  {
    constexpr bool b{experimental::foo()};
    check(report_line(""), b);
  }
}
