////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include <array>

#include <algorithm>

namespace sequoia::experimental
{
  /// taken from stl_algo.h but rendered constexpr
    template<typename _BidirectionalIterator, typename _Distance,
	   typename _Compare>
    constexpr void
    __merge_without_buffer(_BidirectionalIterator __first,
			   _BidirectionalIterator __middle,
			   _BidirectionalIterator __last,
			   _Distance __len1, _Distance __len2,
			   _Compare __comp)
    {
      if (__len1 == 0 || __len2 == 0)
	return;

      if (__len1 + __len2 == 2)
	{
	  if (__comp(__middle, __first))
	    std::iter_swap(__first, __middle);
	  return;
	}

      _BidirectionalIterator __first_cut = __first;
      _BidirectionalIterator __second_cut = __middle;
      _Distance __len11 = 0;
      _Distance __len22 = 0;
      if (__len1 > __len2)
	{
	  __len11 = __len1 / 2;
	  std::advance(__first_cut, __len11);
	  __second_cut
	    = std::__lower_bound(__middle, __last, *__first_cut,
				 __gnu_cxx::__ops::__iter_comp_val(__comp));
	  __len22 = std::distance(__middle, __second_cut);
	}
      else
	{
	  __len22 = __len2 / 2;
	  std::advance(__second_cut, __len22);
	  __first_cut
	    = std::__upper_bound(__first, __middle, *__second_cut,
				 __gnu_cxx::__ops::__val_comp_iter(__comp));
	  __len11 = std::distance(__first, __first_cut);
	}

      _BidirectionalIterator __new_middle
	= std::rotate(__first_cut, __middle, __second_cut);
      experimental::__merge_without_buffer(__first, __first_cut, __new_middle,
				  __len11, __len22, __comp);
      experimental::__merge_without_buffer(__new_middle, __second_cut, __last,
				  __len1 - __len11, __len2 - __len22, __comp);
    }

   /// taken from stl_algo.h but rendered constexpr
   template<typename _RandomAccessIterator, typename _Compare>
   constexpr void
    __inplace_stable_sort(_RandomAccessIterator __first,
			  _RandomAccessIterator __last, _Compare __comp)
    {
      if (__last - __first < 15)
	{
      std::__insertion_sort(__first, __last, __comp);
	  return;
    }
      _RandomAccessIterator __middle = __first + (__last - __first) / 2;
      experimental::__inplace_stable_sort(__first, __middle, __comp);
      experimental::__inplace_stable_sort(__middle, __last, __comp);
      experimental::__merge_without_buffer(__first, __middle, __last,
                                           __middle - __first,
                                           __last - __middle,
                                           __comp);
    }

  /// a constexpr version of stable_sort, based on the gcc implementation
  template<typename _RandomAccessIterator, typename _Compare>
    constexpr void
    stable_sort(_RandomAccessIterator __first, _RandomAccessIterator __last,
		  _Compare __comp)
    {
      if (std::is_constant_evaluated())
      {
        if (__first == __last)
          return;

        experimental::__inplace_stable_sort(__first, __last, __gnu_cxx::__ops::__iter_comp_iter(__comp));
      }
      else
      {
        std::__stable_sort(__first, __last, __gnu_cxx::__ops::__iter_comp_iter(__comp));
      }
    }
  
  /// a constexpr version of inplace_merge, based on the gcc implementation
  template<typename _BidirectionalIterator, typename _Compare>
    constexpr void
    inplace_merge(_BidirectionalIterator __first,
		  _BidirectionalIterator __middle,
		  _BidirectionalIterator __last,
		  _Compare __comp)
    {
      // concept requirements
      __glibcxx_function_requires(_Mutable_BidirectionalIteratorConcept<
	    _BidirectionalIterator>)
      __glibcxx_function_requires(_BinaryPredicateConcept<_Compare,
	    typename iterator_traits<_BidirectionalIterator>::value_type,
	    typename iterator_traits<_BidirectionalIterator>::value_type>)
      __glibcxx_requires_sorted_pred(__first, __middle, __comp);
      __glibcxx_requires_sorted_pred(__middle, __last, __comp);
      __glibcxx_requires_irreflexive_pred(__first, __last, __comp);

      if (std::is_constant_evaluated())
      {
        const auto __len1 = std::distance(__first, __middle);
        const auto __len2 = std::distance(__middle, __last);

        experimental::__merge_without_buffer
          (__first, __middle, __last, __len1, __len2, __gnu_cxx::__ops::__iter_comp_iter(__comp));
      }
      else
      {
        std::__inplace_merge(__first, __middle, __last,
			   __gnu_cxx::__ops::__iter_comp_iter(__comp));
      }
    }

  /// extracted from __stable_partition in stl_algo.h
  template<typename _ForwardIterator, typename _Predicate,
	   typename _Distance>
    constexpr _ForwardIterator
    __inplace_stable_partition(_ForwardIterator __first,
				_ForwardIterator __last,
				_Predicate __pred, _Distance __len)
    {      
      if (__len == 1)
	return __first;

      _ForwardIterator __middle = __first;
      std::advance(__middle, __len / 2);
      _ForwardIterator __left_split =
	experimental::__inplace_stable_partition(__first, __middle, __pred, __len / 2);

      // Advance past true-predicate values to satisfy this
      // function's preconditions.
      _Distance __right_len = __len - __len / 2;
      _ForwardIterator __right_split =
	std::__find_if_not_n(__middle, __right_len, __pred);

      if (__right_len)
	__right_split =
	  experimental::__inplace_stable_partition(__right_split, __last, __pred, __right_len);

      return std::rotate(__left_split, __middle, __right_split);
    }


  /// a constexpr version of stable_partition, based on the gcc implementation
  template<typename _ForwardIterator, typename _Predicate>
    constexpr _ForwardIterator
    stable_partition(_ForwardIterator __first, _ForwardIterator __last,
		     _Predicate __pred)
    {
      // concept requirements
      __glibcxx_function_requires(_Mutable_ForwardIteratorConcept<
				  _ForwardIterator>)
      __glibcxx_function_requires(_UnaryPredicateConcept<_Predicate,
	    typename iterator_traits<_ForwardIterator>::value_type>)
      __glibcxx_requires_valid_range(__first, __last);

      if(std::is_constant_evaluated())
      {
        return experimental::__inplace_stable_partition(__first, __last,
                                                        __gnu_cxx::__ops::__pred_iter(__pred),
                                                        std::distance(__first, __last));
      }
      else
      {
        return std::__stable_partition(__first, __last,
				              __gnu_cxx::__ops::__pred_iter(__pred));
      }
    }
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    test_stable_sort();
    test_inplace_merge();
    test_stable_partition();
  }

  void experimental_test::test_stable_sort()
  {
    {
      using array_t = std::array<int, 3>;
    
      constexpr auto a{
        [](){
          array_t a{42, 1, 42};
          experimental::stable_sort(a.begin(), a.end(), [](int lhs, int rhs){
              return lhs < rhs;
            });

          return a;
        }()
      };

      check(equality,
            LINE("Sorting a three element array, where identical elements are indistinguishable, and so stability is irrelevant"),
            a,
            array_t{1, 42, 42});
    }

    {
      using pair_t = std::pair<int, int>;
      using array_t = std::array<pair_t, 4>;
      
      constexpr auto a{
        [](){
          array_t a{pair_t{42,42}, pair_t{42, 1}, pair_t{42, 4}, pair_t{1, 100}};
          experimental::stable_sort(a.begin(), a.end(), [](const pair_t& lhs, const pair_t& rhs){
              return lhs.first < rhs.first;
            });

          return a;
        }()
      };

      check(equality,
            LINE("Sorting a four element array, where stability makes a difference"),
            a,
            array_t{pair_t{1, 100}, pair_t{42,42}, pair_t{42, 1}, pair_t{42, 4}});
    }
  }

  void experimental_test::test_inplace_merge()
  {
    {
      using array_t = std::array<int, 4>;

      constexpr auto a{
        [](){
          array_t a{1, 4, 2, 5};
          experimental::inplace_merge(a.begin(), a.begin() + 2, a.end(),
                                      [](int lhs, int rhs){ return lhs < rhs; });

          return a;
        }()
      };

      check(equality,
            LINE("Inplace merging of a four element array, implicitly partitioned into two sorted sub-arrays"),
            a,
            array_t{1, 2, 4, 5});
    }
  }

  void experimental_test::test_stable_partition()
  {
    /*{
      using array_t = std::array<int, 4>;

      constexpr auto a{
        [](){
          array_t a{1, 4, 2, 5};
          experimental::stable_partition(a.begin(), a.end(), [](int element){ return element < 3; });

          return a;
        }()
      };

      check(equality,
            LINE("stable partition of a four element array"),
            a,
            array_t{1, 2, 4, 5});
            }*/
  }
}
