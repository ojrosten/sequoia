////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IteratorTest.hpp"
#include "IteratorTestingUtilities.hpp"

namespace sequoia::unit_testing
{    
  void iterator_test::run_tests()
  {
    test_iterator();
    test_const_iterator();
    test_reverse_iterator();
    test_const_reverse_iterator();
  }

  void iterator_test::test_iterator()
  {
    using namespace utilities;
    
    using i_type = std::array<int, 3>::iterator;
    using custom_iter_t = iterator<i_type, identity_dereference_policy<i_type>, null_data_policy>;

    static_assert(std::is_same_v<custom_iter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_iter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_iter_t::value_type, int>);
    static_assert(std::is_same_v<custom_iter_t::pointer, int*>);
    static_assert(std::is_same_v<custom_iter_t::reference, int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_iter_t>(a.begin(), a.end(), "Custom iterator from iterator");

    custom_iter_t i{a.begin() + 1};
    
    *i = 5;
    // 3 5 1

    check_equality({3, 5, 1}, a, LINE("Check changing pointee"));
    check_equality(5, *i, LINE(""));
    check_equality(3, i[-1], LINE(""));
    check_equality(5, i[0], LINE(""));
    check_equality(1, i[1], LINE(""));

    i[-1] = 7;
    // 7 5 1
    
    check_equality({7, 5, 1}, a, LINE("Check changing pointee via []"));
    check_equality(5, *i, LINE(""));
    check_equality(7, i[-1], LINE(""));
    check_equality(5, i[0], LINE(""));
    check_equality(1, i[1], LINE(""));
  }

  void iterator_test::test_const_iterator()
  {
    using namespace utilities;
    
    using ci_type = std::array<int, 3>::const_iterator;
    using custom_citer_t = iterator<ci_type, identity_dereference_policy<ci_type>, null_data_policy>;

    static_assert(std::is_same_v<custom_citer_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_citer_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_citer_t::value_type, int>);
    static_assert(std::is_same_v<custom_citer_t::pointer, const int*>);
    static_assert(std::is_same_v<custom_citer_t::reference, const int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_citer_t>(a.cbegin(), a.cend(), "Custom const_iterator from const_iterator");
    basic_checks<custom_citer_t>(a.begin(), a.end(), "Custom const_iterator from iterator");

    /*
    using i_type = std::array<int, 3>::iterator;
    using custom_iter_t = iterator<i_type, identity_dereference_policy<ci_type>, null_data_policy>;

    basic_checks<custom_citer_t>(custom_iter_t{a.begin()}, custom_iter_t{a.end()});
    */
  }
  
  void iterator_test::test_reverse_iterator()
  {
    using namespace utilities;
    
    using i_type = std::array<int, 3>::reverse_iterator;
    using custom_riter_t = iterator<i_type, identity_dereference_policy<i_type>, null_data_policy>;

    static_assert(std::is_same_v<custom_riter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_riter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_riter_t::value_type, int>);
    static_assert(std::is_same_v<custom_riter_t::pointer, int*>);
    static_assert(std::is_same_v<custom_riter_t::reference, int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_riter_t>(a.rbegin(), a.rend(), "Custom reverse_iterator from reverse_iterator");

    custom_riter_t i{a.rbegin()};

    *i = 5;
    // 3 0 5

    check_equality({3, 0, 5}, a, LINE("Check changing pointee"));
    check_equality(5, *i, LINE(""));
    check_equality(5, i[0], LINE(""));
    check_equality(0, i[1], LINE(""));
    check_equality(3, i[2], LINE(""));

    i[2] = 7;
    // 7 0 5
    
    check_equality({7, 0, 5}, a, LINE("Check changing pointee via []"));
    check_equality(5, *i, LINE(""));
    check_equality(5, i[0], LINE(""));
    check_equality(0, i[1], LINE(""));
    check_equality(7, i[2], LINE(""));
  }

  void iterator_test::test_const_reverse_iterator()
  {
    using namespace utilities;
    
    using i_type = std::array<int, 3>::const_reverse_iterator;
    using custom_criter_t = iterator<i_type, identity_dereference_policy<i_type>, null_data_policy>;

    static_assert(std::is_same_v<custom_criter_t::iterator_category, std::random_access_iterator_tag>);
    static_assert(std::is_same_v<custom_criter_t::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<custom_criter_t::value_type, int>);
    static_assert(std::is_same_v<custom_criter_t::pointer, const int*>);
    static_assert(std::is_same_v<custom_criter_t::reference, const int&>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_criter_t>(a.crbegin(), a.crend(), "Custom const_reverse_iterator from const_reverse_iterator");
    basic_checks<custom_criter_t>(a.rbegin(), a.rend(), "Custom const_reverse_iterator from reverse_iterator");

    // TO DO: init with non-const r-iter
  }
  

  template<class CustomIter, class Iter>
  void iterator_test::basic_checks(Iter begin, Iter end, std::string_view message)
  {
    using namespace std;
    
    if(!check_equality<int64_t>(3, distance(begin, end), LINE(std::string{"Contract violated"}.append(message))))
      return;
    
    CustomIter i{begin};
    check_equality(*begin, *i, LINE(message));
    check_equality(begin[0], i[0], LINE(message));
    check_equality(begin[1], i[1], LINE(message));
    check_equality(begin[2], i[2], LINE(message));

    using base_iter = typename CustomIter::base_iterator_type;
    check_equality(i.operator->(), &*base_iter{begin}, LINE("Operator ->"));

    CustomIter j{end};      
    check_regular_semantics(i, j, LINE("Regular semantics; one iterator at end"));
      
    check(i < j, LINE(message));
    check(j > i, LINE(message));
    check(i <= j, LINE(message));
    check(j >= i, LINE(message));      
    check_equality(distance(begin, end), distance(i, j), LINE(std::string{message}.append(" Check non-zero distance")));

    check_equality(begin[1], *++i, LINE(message));
    check_equality(begin[1], *i++, LINE(message));
    check_equality(begin[2], *i, LINE(message));
    check(++i == j, LINE(message));
    check(i <= j, LINE(message));
    check(j >= i, LINE(message));

    check_equality(begin[2], *--i, LINE(message));
    check_equality(begin[2], *i--, LINE(message));
    check_equality(begin[1], *i, LINE(message));

    j = i - 1;
    check_equality(begin[1], *i, LINE(message));
    check_equality(begin[0], *j, LINE(message));
    check_regular_semantics(i, j, LINE(std::string{message}.append(" Regular semantics")));

    i = j + 2;
    check_equality(begin[2], *i, LINE(message));

    i -= 1;
    check_equality(begin[1], *i, LINE(message));

    j += 1;
    check_equality(begin[1], *j, LINE(message));

    check(i == j, LINE(message));
    check_equality<int64_t>(0, distance(i, j), LINE(std::string{message}.append(" Check for distance of zero")));
  }
}
