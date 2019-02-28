////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedData.hpp"
#include "TestPartitionedDataHelper.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    template<class T> struct type_to_string<data_structures::bucketed_storage<T>>
    {
        static std::string str() { return "BUCKETED STORAGE; UNSHARED DATA"; }
    };

    template<class T> struct type_to_string<data_structures::contiguous_storage<T>>
    {
        static std::string str() { return "CONTIGUOUS STORAGE; UNSHARED DATA"; }
    };

    template<class T> struct type_to_string<data_structures::bucketed_storage<T, data_sharing::shared<T>>>
    {
        static std::string str() { return "BUCKETED STORAGE; SHARED DATA"; }
    };

    template<class T> struct type_to_string<data_structures::contiguous_storage<T, data_sharing::shared<T>>>
    {
        static std::string str() { return "CONTIGUOUS STORAGE; SHARED DATA"; }
    };
  
    class partitioned_data_test : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      struct no_default_constructor
      {
        constexpr no_default_constructor(const int _x) : x{_x} {}

        friend bool operator==(const no_default_constructor& lhs, const no_default_constructor& rhs)
        {
          return lhs.x == rhs.x;
        }

        friend bool operator!=(const no_default_constructor& lhs, const no_default_constructor& rhs)
        {
          return !(lhs == rhs);
        }

        template<class Stream> friend Stream& operator<<(Stream& stream, const no_default_constructor& ndc)
        {
          stream << ndc.x;
          return stream;
        }
        
        int x;
      };
      
      void run_tests() override;

      template<class S, class T>
      bool check_partitions(S& storage, std::initializer_list<std::initializer_list<T>> answers);
      
      // storge passed by non-const reference, so that non-const iterators can be tested!
      template<class S, class T>
      bool check_partitions(S& storage, const std::vector<std::vector<T>>& answers);

      template<class S, class T>
      bool check_constexpr_partitions(S& storage, std::initializer_list<std::initializer_list<T>> answers);

      template<class S, class T>
      bool check_constexpr_partitions(const S& storage, const std::vector<std::vector<T>>& answers);
    
      template<template<class...> class Storage, class T>
      void test_init_lists(std::initializer_list<std::initializer_list<T>> list);
      
      void test_storage();
      
      template <class Storage> Storage test_generic_storage();

      void test_static_storage();

      template<class T, class SharingPolicy, bool ThrowOnRangeError>
      void test_contiguous_capacity();

      template<class T, class SharingPolicy, bool ThrowOnRangeError>
      void test_bucketed_capacity();
      
      template<template<class...> class C, class SharingPolicy, template<class> class ReferencePolicy, class Arg>
      void test_generic_iterator_properties(const Arg& v);

      template<template<class...> class C, class SharingPolicy, template<class> class ReferencePolicy, class Arg>
      void test_generic_iterator_deref(const Arg& v);
      
      template<template<class> class SharingPolicy> void test_iterators();
    };
  }
}
