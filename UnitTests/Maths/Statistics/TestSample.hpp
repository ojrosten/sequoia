#pragma once

#include "UnitTestUtils.hpp"
#include "Sample.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class test_sample : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      void run_tests();

      template<class T> void test_single_data_set()
      {
        using namespace statistics;

        sample<T> sdata;
        check_equality<std::size_t>(0, sdata.size());
        check(std::isnan(sdata.mean()));
        check(std::isnan(sdata.variance()));
        check(std::isnan(sdata.standard_deviation()));
        check(std::isnan(sdata.minimum()));
        check(std::isnan(sdata.maximum()));

        sdata.add_datum(1.0);

        check_equality<std::size_t>(1, sdata.size());
        check_equality<T>(1, sdata.mean());
        check_equality<T>(0, sdata.variance());
        check_equality<T>(0, sdata.standard_deviation());
        check_equality<T>(1, sdata.minimum());
        check_equality<T>(1, sdata.maximum());

        sdata.add_datum(3.0);
        check_equality<std::size_t>(2, sdata.size());
        check_equality<T>(1, sdata.variance());
        check_equality<T>(2, sdata.mean());
        check_equality<T>(1, sdata.standard_deviation());
        check_equality<T>(1, sdata.minimum());
        check_equality<T>(3, sdata.maximum());
        check_equality<T>(2, sdata.sample_variance());
        check_equality<T>(sqrt(2), sdata.sample_standard_deviation());
        check_equality_within_tolerance<T>(2,sdata.template sample_standard_deviation<bias::gaussian_approx_modifier>(),std::numeric_limits<T>::epsilon()*100);
      }
    };
  }
}
