////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IteratorTest.hpp"
#include "IteratorTestingUtilities.hpp"

#include <array>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view iterator_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void iterator_test::run_tests()
  {
    test_iterator();
    test_const_iterator();
    test_reverse_iterator();
    test_const_reverse_iterator();

    test_const_scaling_iterator();
    test_const_reverse_scaling_iterator();
  }

  void iterator_test::test_iterator()
  {
    using namespace utilities;
    
    using i_type = std::array<int, 3>::iterator;
    using custom_iter_t = iterator<i_type, identity_dereference_policy<i_type, null_data_policy>>;

    static_assert(std::is_same_v<custom_iter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_iter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_iter_t::value_type, int>);
    static_assert(std::is_same_v<custom_iter_t::pointer, int*>);
    static_assert(std::is_same_v<custom_iter_t::reference, int&>);
    static_assert(std::is_same_v<custom_iter_t::const_dereference_type, const int&>);
    
    
    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_iter_t>(a.begin(), a.end(), &*a.begin(), "Custom iterator from iterator");

    custom_iter_t i{a.begin() + 1};
    
    *i = 5;
    // 3 5 1

    check_equality(LINE("Check changing pointee"), a, {3, 5, 1});
    check_equality(LINE(""), *i, 5);
    check_equality(LINE(""), i[-1], 3);
    check_equality(LINE(""), i[0], 5);
    check_equality(LINE(""), i[1], 1);

    i[-1] = 7;
    // 7 5 1
    
    check_equality(LINE("Check changing pointee via []"), a, {7, 5, 1});
    check_equality(LINE(""), *i, 5);
    check_equality(LINE(""), i[-1], 7);
    check_equality(LINE(""), i[0], 5);
    check_equality(LINE(""), i[1], 1);

    std::sort(custom_iter_t{a.begin()}, custom_iter_t{a.end()});
    // 1 5 7
    
    check_equality(LINE(""), i[-1], 1);
    check_equality(LINE(""), i[0], 5);
    check_equality(LINE(""), i[1], 7);
  }

  void iterator_test::test_const_iterator()
  {
    using namespace utilities;
    
    using ci_type = std::array<int, 3>::const_iterator;
    using custom_citer_t = iterator<ci_type, identity_dereference_policy<ci_type, null_data_policy>>;

    static_assert(std::is_same_v<custom_citer_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_citer_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_citer_t::value_type, int>);
    static_assert(std::is_same_v<custom_citer_t::pointer, const int*>);
    static_assert(std::is_same_v<custom_citer_t::reference, const int&>);
    static_assert(std::is_same_v<custom_citer_t::const_dereference_type, const int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_citer_t>(a.cbegin(), a.cend(), &*a.cbegin(), "Custom const_iterator from const_iterator");
    basic_checks<custom_citer_t>(a.begin(), a.end(), &*a.cbegin(), "Custom const_iterator from iterator");

    using i_type = std::array<int, 3>::iterator;
    using custom_iter_t = iterator<i_type, identity_dereference_policy<ci_type, null_data_policy>>;

    basic_checks<custom_citer_t>(custom_iter_t{a.begin()}, custom_iter_t{a.end()}, &*a.cbegin(), "Custom const_iterator from custom iterator");
  }
  
  void iterator_test::test_reverse_iterator()
  {
    using namespace utilities;
    
    using ri_type = std::array<int, 3>::reverse_iterator;
    using custom_riter_t = iterator<ri_type, identity_dereference_policy<ri_type, null_data_policy>>;

    static_assert(std::is_same_v<custom_riter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_riter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_riter_t::value_type, int>);
    static_assert(std::is_same_v<custom_riter_t::pointer, int*>);
    static_assert(std::is_same_v<custom_riter_t::reference, int&>);
    static_assert(std::is_same_v<custom_riter_t::const_dereference_type, const int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_riter_t>(a.rbegin(), a.rend(), &*a.rbegin(), "Custom reverse_iterator from reverse_iterator");

    custom_riter_t i{a.rbegin()};

    *i = 5;
    // 3 0 5

    check_equality(LINE("Check changing pointee"), a, {3, 0, 5});
    check_equality(LINE(""), *i, 5);
    check_equality(LINE(""), i[0], 5);
    check_equality(LINE(""), i[1], 0);
    check_equality(LINE(""), i[2], 3);

    i[2] = 7;
    // 7 0 5
    
    check_equality(LINE("Check changing pointee via []"), a, {7, 0, 5});
    check_equality(LINE(""), *i, 5);
    check_equality(LINE(""), i[0], 5);
    check_equality(LINE(""), i[1], 0);
    check_equality(LINE(""), i[2], 7);
  }

  void iterator_test::test_const_reverse_iterator()
  {
    using namespace utilities;
    
    using cri_type = std::array<int, 3>::const_reverse_iterator;
    using custom_criter_t = iterator<cri_type, identity_dereference_policy<cri_type, null_data_policy>>;

    static_assert(std::is_same_v<custom_criter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_criter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_criter_t::value_type, int>);
    static_assert(std::is_same_v<custom_criter_t::pointer, const int*>);
    static_assert(std::is_same_v<custom_criter_t::reference, const int&>);
    static_assert(std::is_same_v<custom_criter_t::const_dereference_type, const int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_criter_t>(a.crbegin(), a.crend(), &*a.crbegin(), "Custom const_reverse_iterator from const_reverse_iterator");
    basic_checks<custom_criter_t>(a.rbegin(), a.rend(), &*a.crbegin(), "Custom const_reverse_iterator from reverse_iterator");

    using ri_type = std::array<int, 3>::reverse_iterator;
    using custom_riter_t = iterator<ri_type, identity_dereference_policy<ri_type, null_data_policy>>;

    basic_checks<custom_criter_t>(custom_riter_t{a.rbegin()}, custom_riter_t{a.rend()}, &*a.crbegin(), "Custom const_reverse_iterator from custom reverse_iterator");
  }
  
  void iterator_test::test_const_scaling_iterator()
  {
    using namespace utilities;
    
    using ci_type = std::array<int, 3>::const_iterator;
    using custom_citer_t = iterator<ci_type, scaling_dereference_policy<ci_type>>;

    static_assert(std::is_same_v<custom_citer_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_citer_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_citer_t::value_type, int>);
    static_assert(std::is_same_v<custom_citer_t::pointer, const int*>);
    static_assert(std::is_same_v<custom_citer_t::proxy, int>);
    static_assert(std::is_same_v<custom_citer_t::const_dereference_type, int>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_citer_t>(a.cbegin(), a.cend(), &*a.cbegin(), "Custom scaling iterator from const_iterator", 3);

    basic_checks<custom_citer_t>(a.begin(), a.end(), &*a.cbegin(), "Custom scaling iterator from iterator", -1);

    custom_citer_t i{a.cend(), 1}, j{a.cend(), 2};
    check(LINE("Custom iterators should compare equal if they point to the same thing, irrespective of any other state"), i == j);
  }

  void iterator_test::test_const_reverse_scaling_iterator()
  {
    using namespace utilities;
    
    using cri_type = std::array<int, 3>::const_reverse_iterator;
    using custom_criter_t = iterator<cri_type, scaling_dereference_policy<cri_type>>;

    static_assert(std::is_same_v<custom_criter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_criter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_criter_t::value_type, int>);
    static_assert(std::is_same_v<custom_criter_t::pointer, const int*>);
    static_assert(std::is_same_v<custom_criter_t::proxy, int>);
    static_assert(std::is_same_v<custom_criter_t::const_dereference_type, int>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_criter_t>(a.crbegin(), a.crend(), &*a.crbegin(), "Custom reverse scaling iterator from const_reverse_iterator", -1);

    basic_checks<custom_criter_t>(a.rbegin(), a.rend(), &*a.crbegin(), "Custom reverse scaling iterator from reverse_iterator", 3);

    custom_criter_t i{a.crend(), 1}, j{a.crend(), 2};
    check(LINE("Custom reverse iterators should compare equal if they point to the same thing, irrespective of any other state"), i == j);
  }
  
  template<class CustomIter, class Iter, class... Args, class Pointer>
  void iterator_test::basic_checks(Iter begin, Iter end, Pointer pBegin, std::string_view message, Args... args)
  {
    using namespace std;

    using value_type = typename std::iterator_traits<Iter>::value_type;
    using deref_pol = typename CustomIter::dereference_policy;

    if(!check_equality(LINE(append_indented(message, "Contract violated")), distance(begin, end), long{3}))
      return;
    
    CustomIter i{begin, args...};

    value_type scale{1};
    if constexpr(is_scaling_v<deref_pol>)
    {
      scale = i.scale();
    }
    
    check_equality(LINE(message), *i, *begin * scale);
    check_equality(LINE(message), i[0], begin[0] * scale);
    check_equality(LINE(message), i[1], begin[1] * scale);
    check_equality(LINE(message), i[2], begin[2] * scale);

    check_equality(LINE(append_indented(message, "Operator ->")), i.operator->(), pBegin);

    CustomIter j{end, args...};      
    check_semantics(LINE(append_indented(message, "Regular semantics; one iterator at end")), i, j);
      
    check(LINE(message), i < j);
    check(LINE(message), j > i);
    check(LINE(message), i <= j);
    check(LINE(message), j >= i);
    check_equality(LINE(append_indented(message, "Check non-zero distance")), distance(i, j), distance(begin, end));

    check_equality(LINE(message), *++i, begin[1] * scale);
    check_equality(LINE(message), *i++, begin[1] * scale);
    check_equality(LINE(message), *i, begin[2] * scale);
    check(LINE(message), ++i == j);
    check(LINE(message), i <= j);
    check(LINE(message), j >= i);

    check_equality(LINE(message), *--i, begin[2] * scale);
    check_equality(LINE(message), *i--, begin[2] * scale);
    check_equality(LINE(message), *i, begin[1] * scale);

    j = i - 1;
    check_equality(LINE(message), *i, begin[1] * scale);
    check_equality(LINE(message), *j, begin[0] * scale);
    check_semantics(LINE(append_indented(message, "Regular semantics")), i, j);

    i = j + 2;
    check_equality(LINE(message), *i, begin[2] * scale);

    i -= 1;
    check_equality(LINE(message), *i, begin[1] * scale);

    j += 1;
    check_equality(LINE(message), *j, begin[1] * scale);

    check(LINE(message), i == j);
    check_equality<int64_t>(LINE(append_indented(message, "Check for distance of zero")), distance(i, j), 0);
  }
}
