#include "UnitTestDiagnostics.hpp"

#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    void false_positive_diagnostics::run_tests()
    {
      
      basic_tests();
      test_container_checks();
      test_relative_performance();
      test_mixed();
      test_standard_semantics();
    }

    void false_positive_diagnostics::basic_tests()
    {
      check(false, LINE(""));
      
      check_equality(5, 4, LINE(""));
      check_equality(6.5, 5.6, LINE(""));

      check_equality(std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8}, LINE(""));
      check_equality(std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 7.8}, LINE(""));

      check_equality(std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{0, 3.4, -9.2f}, LINE(""));
      check_equality(std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 0.0, -9.2f}, LINE(""));
      check_equality(std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -0.0f}, LINE(""));

      check_equality_within_tolerance(3.0, 5.0, 1.0, LINE(""));
      check_equality_within_tolerance(7.0, 5.0, 1.0, LINE(""));

      check_exception_thrown<std::runtime_error>([](){}, LINE("Exception expected but nothing thrown"));
      check_exception_thrown<std::runtime_error>([](){ throw std::logic_error("Error"); }, LINE("Exception thrown but of wrong type"));
    }

    void false_positive_diagnostics::test_container_checks()
    {
      check_equality(std::vector<double>{}, std::vector<double>{1}, LINE("Empty vector check which should fail"));
      check_equality(std::vector<double>{1}, std::vector<double>{2}, LINE("One element vector check which should fail due to wrong value"));
      check_equality(std::vector<double>{1}, std::vector<double>{1,2}, LINE("One element vector check which should fail due to differing sizes"));
      check_equality(std::vector<double>{1,5}, std::vector<double>{1,4}, LINE("Multi-element vector comparison which should fail due to last element"));
      check_equality(std::vector<double>{1,5}, std::vector<double>{0,5}, LINE("Multi-element vector comparison which should fail due to first element"));
      check_equality(std::vector<double>{1,5,3.2}, std::vector<double>{1,5,3.3}, LINE("Multi-element vector comparison which should fail due to middle element"));      
      check_equality(std::vector<double>{1,5,3.2}, std::vector<double>{5,3.2}, LINE("Multi-element vector comparison which should fail due to different sizes"));

      std::vector<float> refs{-4.3, 2.8, 6.2, 7.3}, ans{1.1, -4.3, 2.8, 6.2, 8.4, 7.3};

      check_range(refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend(), LINE("Iterators demarcate differing number of elements"));
      check_range(refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4, LINE("Iterators demarcate differing elements"));
    }

    void false_positive_diagnostics::test_relative_performance()
    {
      auto wait{[](const size_t millisecs) {
          std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
        }
      };

      check_relative_performance([wait]() { return wait(1); }, [wait]() { return wait(1); }, 2.0, 2.0, LINE("Performance Test for which fast task is too slow"));
      check_relative_performance([wait]() { return wait(1); }, [wait]() { return wait(1); }, 2.0, 3.0, LINE("Performance Test for which fast task is too slow"));
      check_relative_performance([wait]() { return wait(1); }, [wait]() { return wait(4); }, 2.0, 2.5, LINE("Performance Test for which fast task is too fast"));      
    }

    void false_positive_diagnostics::test_mixed()
    {
      using t_0 = std::vector<std::pair<int, float>>;
      using t_1 = std::set<double>;
      using t_2 = std::complex<double>;
      using type = std::tuple<t_0, t_1, t_2>;
      
      type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

      {
        type b{t_0{{2, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
        check_equality(a, b, LINE(""));
      }

      {
        type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.4, -9.6, 3.2}, {1.1, 0.2}};
        check_equality(a, b, LINE(""));
      }
    }

    void false_positive_diagnostics::test_standard_semantics()
    {
      check_standard_semantics(broken_equality{1}, broken_equality{2}, LINE(""));
      check_standard_semantics(broken_inequality{1}, broken_inequality{2}, LINE(""));
      check_standard_semantics(broken_copy{1}, broken_copy{2}, LINE(""));
      check_standard_semantics(broken_move{1}, broken_move{2}, LINE(""));
      check_standard_semantics(broken_copy_assignment{1}, broken_copy_assignment{2}, LINE(""));
      check_standard_semantics(broken_move_assignment{1}, broken_move_assignment{2}, LINE(""));

      check_standard_semantics(perfectly_normal_beast{1}, perfectly_normal_beast{1}, LINE(""));
    }

    void false_negative_diagnostics::run_tests()
    {
      basic_tests();
      test_container_checks();
      test_relative_performance();
      test_mixed();
      test_standard_semantics();
    }

    void false_negative_diagnostics::basic_tests()
    {
      check(true, LINE(""));

      check_equality(5, 5, LINE(""));
      check_equality(5.0, 5.0, LINE(""));

      check_equality_within_tolerance(4.5, 5.0, 1.0, LINE(""));
      check_equality_within_tolerance(5.5, 5.0, 1.0, LINE(""));

      check_equality(std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8}, LINE(""));

      check_equality(std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f}, LINE(""));

      check_exception_thrown<std::runtime_error>([](){ throw std::runtime_error("Error");}, LINE(""));

      
    }

    void false_negative_diagnostics::test_container_checks()
    {
      check_equality(std::vector<double>{}, std::vector<double>{}, LINE("Empty vector check which should pass"));
      check_equality(std::vector<double>{2}, std::vector<double>{2}, LINE("One element vector check which should pass"));
      check_equality(std::vector<double>{1,5}, std::vector<double>{1,5}, LINE("Multi-element vector comparison which should pass"));

      std::vector<float> refs{-4.3, 2.8, 6.2, 7.3}, ans{1.1, -4.3, 2.8, 6.2, 8.4, 7.3};
      
      check_range(refs.cbegin(), refs.cbegin()+3, ans.cbegin()+1, ans.cbegin()+4, LINE("Iterators demarcate identical elements"));
    }
    
    void false_negative_diagnostics::test_relative_performance()
    {
      
      auto wait{[](const size_t millisecs) {
          std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
        }
      };

      auto fast{[wait]() {
          return wait(1);
        }
      };      

      check_relative_performance(fast, [wait]() { return wait(2); }, 1.9, 2.1, LINE("Performance Test which should pass"), 5);
      check_relative_performance(fast, [wait]() { return wait(4); }, 3.9, 4.1, LINE("Performance Test which should pass"));
    }

    void false_negative_diagnostics::test_mixed()
    {
      using t_0 = std::vector<std::pair<int, float>>;
      using t_1 = std::set<double>;
      using t_2 = std::complex<double>;
      using type = std::tuple<t_0, t_1, t_2>;
      
      type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
      type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

      check_equality(a, b, LINE(""));

    }

    void false_negative_diagnostics::test_standard_semantics()
    {
      check_standard_semantics(perfectly_normal_beast{1}, perfectly_normal_beast{2}, LINE(""));
    }
  }
}
