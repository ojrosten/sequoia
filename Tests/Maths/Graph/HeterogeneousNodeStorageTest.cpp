////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "HeterogeneousNodeStorageTest.hpp"

#include "sequoia/Maths/Graph/HeterogeneousNodeStorage.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<class... Ts>
    class storage_tester : public maths::graph_impl::heterogeneous_node_storage<Ts...>
    {
    public:
      template<class... Args>
      constexpr explicit storage_tester(Args&&... args)
        : maths::graph_impl::heterogeneous_node_storage<Ts...>(std::forward<Args>(args)...)
      {
      }
    };

    constexpr storage_tester<float, int> make_storage()
    {
      storage_tester<float, int> s{};

      s.node_weight<0>(2.0f);
      s.node_weight<int>(4);

      s.mutate_node_weight<float>([](float& f) { f += 1; });
      s.mutate_node_weight<int>([](int& i) { i -= 2; });

      return s;
    }
  }

  [[nodiscard]]
  std::string_view test_heterogeneous_node_storage::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_heterogeneous_node_storage::run_tests()
  {
    storage_tester<int, double> s{3, 0.8};

    check(equality, report_line(""), s.node_weight<0>(), 3);
    check(equality, report_line(""), s.node_weight<int>(), 3);

    check(equality, report_line(""), s.node_weight<1>(), 0.8);
    check(equality, report_line(""), s.node_weight<double>(), 0.8);

    s.node_weight<0>(-4);
    check(equality, report_line(""), s.node_weight<0>(), -4);
    check(equality, report_line(""), s.node_weight<int>(), -4);

    s.node_weight<int>(10);
    check(equality, report_line(""), s.node_weight<0>(), 10);
    check(equality, report_line(""), s.node_weight<int>(), 10);

    s.node_weight<1>(3.2);
    check(equality, report_line(""), s.node_weight<1>(), 3.2);
    check(equality, report_line(""), s.node_weight<double>(), 3.2);

    s.node_weight<double>(10.4);
    check(equality, report_line(""), s.node_weight<1>(), 10.4);
    check(equality, report_line(""), s.node_weight<double>(), 10.4);

    s.mutate_node_weight<0>([](int& i) { return i+=5; });
    check(equality, report_line(""), s.node_weight<0>(), 15);
    check(equality, report_line(""), s.node_weight<int>(), 15);

    s.mutate_node_weight<int>([](int& i) { return i/=5; });
    check(equality, report_line(""), s.node_weight<0>(), 3);
    check(equality, report_line(""), s.node_weight<int>(), 3);

    s.mutate_node_weight<1>([](double& d) { return d-=4.4; });
    check(equality, report_line(""), s.node_weight<1>(), 6.0);
    check(equality, report_line(""), s.node_weight<double>(), 6.0);

    s.mutate_node_weight<double>([](double& d) { return d/=6; });
    check(equality, report_line(""), s.node_weight<1>(), 1.0);
    check(equality, report_line(""), s.node_weight<double>(), 1.0);

    constexpr storage_tester<float, int> t{make_storage()};

    check(equality, report_line(""), t.node_weight<0>(), 3.0f);
    check(equality, report_line(""), t.node_weight<float>(), 3.0f);

    check(equality, report_line(""), t.node_weight<1>(), 2);
    check(equality, report_line(""), t.node_weight<int>(), 2);
  }
}
