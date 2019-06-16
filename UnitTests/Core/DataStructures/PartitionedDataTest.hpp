////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedDataTestingUtilities.hpp"

namespace sequoia
{
  namespace unit_testing
  {  
    class partitioned_data_test : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:      
      void run_tests() override;

      void test_storage();
      
      template <class Storage> Storage test_generic_storage();

      void test_static_storage();

      template<class T, class SharingPolicy, bool ThrowOnRangeError>
      void test_contiguous_capacity();

      template<class T, class SharingPolicy, bool ThrowOnRangeError>
      void test_bucketed_capacity();
      
      template<class Traits, template<class> class SharingPolicy, template<class> class ReferencePolicy>
      void test_generic_iterator_properties();
      
      template<template<class> class SharingPolicy> void test_iterators();

      struct traits
      {
        template<class S> using container_type = std::vector<S, std::allocator<S>>;
      };
    };
  }
}
