#include "TestIterator.hpp"

#include "Iterator.hpp"

#include <array>

namespace sequoia
{
  namespace unit_testing
  {
    void test_iterator::run_tests()
    {
      test_vector_int();
      test_vector_vector();
      test_array();
    }
    
    void test_iterator::test_vector_int()  
    {
      std::vector<int> v{3,0,1};

      using namespace utilities;
      using i_type = std::vector<int>::iterator;
      using iter = iterator<i_type, identity_dereference_policy<i_type>, null_data_policy>;

      iter i{v.begin()};
      check_equality(3, *i, LINE(""));
      check_equality(3, i[0], LINE(""));
      check_equality(0, i[1], LINE(""));
      check_equality(1, i[2], LINE(""));

      iter j{v.end()};
      check(i != j, LINE(""));
      check(i < j, LINE(""));
      check(j > i, LINE(""));
      check(i <= j, LINE(""));
      check(j >= i, LINE(""));      
      check_equality<int64_t>(3, distance(i, j), LINE(""));

      check_equality(0, *++i, LINE(""));
      check_equality(0, *i++, LINE(""));
      check_equality(1, *i, LINE(""));
      check(++i == j, LINE(""));
      check(i <= j, LINE(""));
      check(j >= i, LINE(""));

      check_equality(1, *--i, LINE(""));
      check_equality(1, *i--, LINE(""));
      check_equality(0, *i, LINE(""));

      j = i - 1;
      check_equality(0, *i, LINE(""));
      check_equality(3, *j, LINE(""));

      i = j + 2;
      check_equality(1, *i, LINE(""));

      i -= 1;
      check_equality(0, *i, LINE(""));

      j += 1;
      check_equality(0, *j, LINE(""));

      check(i == j, LINE(""));
      check_equality<int64_t>(0, distance(i, j), LINE("Check for distance of zero"));

      *i = 5;
      check_equality(5, *i, LINE("Check changing pointee"));
      check_equality(5, *i, LINE("Check indirect change of pointee"));

      static_assert(std::is_same_v<typename i_type::reference, int&>);
      static_assert(std::is_same_v<iter::reference, int&>);

      using ci_type = std::vector<int>::const_iterator;      
      using citer = iterator<ci_type, identity_dereference_policy<ci_type>, null_data_policy>;

      citer ci{v.cbegin()}, cj{v.cend()};
      check_equality(3, *ci, LINE("Check initial value of citerator"));
      check_equality<int64_t>(3, distance(ci, cj), LINE(""));

      citer cFromI{i};
      check_equality(5, *cFromI, LINE(""));

      using ri_type = std::vector<int>::reverse_iterator;
      using riter = iterator<ri_type, identity_dereference_policy<ri_type>, null_data_policy>;

      riter ri{v.rbegin()}, rj{v.rend()};
      check_equality(1, *ri, LINE("Check initial value of riterator"));
      check_equality<int64_t>(3, distance(ri, rj), LINE(""));

      using cri_type = std::vector<int>::const_reverse_iterator;
      using criter = iterator<cri_type, identity_dereference_policy<cri_type>, null_data_policy>;

      criter cri{v.crbegin()}, crj{v.crend()};
      check_equality(1, *cri, LINE("Check initial value of criterator"));
      check_equality<int64_t>(3, distance(cri, crj), LINE(""));

      criter crFromR{ri};
      check_equality(1, *crFromR, LINE(""));
    }

    void test_iterator::test_vector_vector()
    {
      using namespace utilities;
      using c_type = std::vector<std::vector<int>>;
      using i_type = c_type::iterator;
      using iter = iterator<i_type, identity_dereference_policy<i_type>, null_data_policy>;

      c_type v{{3,20}, {1}, {5, 6, -3}};

      iter i{v.begin()};
      check_equality(3, *i->begin(), LINE("Check -> operator"));
    }
    
    void test_iterator::test_array()
    {
      using namespace utilities;
      using c_type = std::array<int, 5>;
      using i_type = c_type::const_iterator;
      using iter = iterator<i_type, identity_dereference_policy<i_type>, null_data_policy>;

      constexpr c_type a{5, 8, 2, 9, 4};
      iter i{a.cbegin()};

      check_equality(5, *i, LINE("Check * for array iterator"));
    }
  }
}
