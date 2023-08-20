////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "EdgeTest.hpp"

#include "EdgeTestingUtilities.hpp"

#include "sequoia/Core/Object/Handlers.hpp"

#include <complex>
#include <list>
#include <vector>

namespace sequoia
{
  namespace testing
  {
    using namespace maths;
    using namespace object;

    [[nodiscard]]
    std::filesystem::path test_edges::source_file() const
    {
      return std::source_location::current().file_name();
    }

    void test_edges::run_tests()
    {
      test_plain_partial_edge();
      test_partial_edge_indep_weight();
      test_partial_edge_shared_weight();
      test_partial_edge_meta_data();
      test_partial_edge_indep_weight_meta_data();
      test_partial_edge_shared_weight_meta_data();

      test_plain_embedded_partial_edge();
      test_embedded_partial_edge_indep_weight();
      test_embedded_partial_edge_shared_weight();
      test_embedded_partial_edge_meta_data();
      test_embedded_partial_edge_indep_weight_meta_data();
      test_embedded_partial_edge_shared_weight_meta_data();
    }

    void test_edges::test_plain_partial_edge()
    {
      using edge_t = partial_edge<by_value<null_weight>, null_meta_data>;
      static_assert(sizeof(std::size_t) == sizeof(edge_t));

      using compact_edge_t = partial_edge<by_value<null_weight>, null_meta_data, unsigned char>;
      static_assert(sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e0{0};
      check(equivalence, report_line("Construction"), e0, 0);

      edge_t e1{1};
      check(equivalence, report_line("Construction"), e1, 1);

      check_semantics(report_line("Standard semantics"), e0, e1);

      e0.target_node(1);
      check(equality, report_line("Changing target node"), e0, e1);

      edge_t e2{3};
      check(equivalence, report_line(""), e2, 3);

      check_semantics(report_line("Standard semantics"), e2, e1);
    }

    void test_edges::test_partial_edge_shared_weight()
    {
      using edge_t = partial_edge<shared<int>, null_meta_data>;
      static_assert(sizeof(std::shared_ptr<int>) + sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{1, 4};
      check(equivalence, report_line("Construction"), edge, 1, 4);

      edge.weight(5);
      check(equality, report_line("Set weight"), edge, edge_t{1, 5});

      edge.mutate_weight([](auto& i){ i -= 6;});
      check(equality, report_line("Manipulate weight"), edge, edge_t{1, -1});

      edge.target_node(2);
      check(equality, report_line("Change target node"), edge, edge_t{2, -1});

      edge_t edge1{2,-7};
      check(equivalence, report_line("Construction"), edge1, 2, -7);

      check_semantics(report_line("Standard Semantics"), edge1, edge);

      edge_t edge2{6, edge};
      check(equality, report_line("Construction with shared weight"), edge2, edge_t{6, -1});

      edge.target_node(1);
      check(equality, report_line("Change target node of edge with shared weight"), edge, edge_t{1, -1});
      check(equality, report_line(""), edge2, edge_t{6, -1});

      edge2.target_node(5);
      check(equality, report_line("Change target node of edge with shared weight"), edge, edge_t{1, -1});
      check(equality, report_line(""), edge2, edge_t{5, -1});

      edge2.weight(-3);
      check(equality, report_line("Implicit change of shared weight"), edge, edge_t{1, -3});
      check(equality, report_line("Explicit change of shared weight"), edge2, edge_t{5, -3});

      check_semantics(report_line("Standard semantics with shared weight"), edge2, edge);

      edge_t edge3(2, 8);
      check(equivalence, report_line(""), edge3, 2, 8);

      check_semantics(report_line("Standard semantics with one having a shared weight"), edge2, edge);

      std::ranges::swap(edge, edge2);
      std::ranges::swap(edge, edge3);
      check(equality, report_line("Swap edge with one of an edge sharing pair"), edge, edge_t{2, 8});
      check(equality, report_line("Swap edge with one of an edge sharing pair"), edge2, edge_t{1, -3});
      check(equality, report_line("Swap edge with one of an edge sharing pair"), edge3, edge_t{5, -3});

      edge.mutate_weight([](int& a) { ++a; });
      check(equality, report_line("Mutate weight of edge which previously but no longer shares its weight"), edge, edge_t{2, 9});
      check(equality, report_line(""), edge2, edge_t{1, -3});
      check(equality, report_line(""), edge3, edge_t{5, -3});

      edge2.weight(4);
      check(equality, report_line(""), edge, edge_t{2, 9});
      check(equality, report_line("Set weight which is now shared with a different weight"), edge2, edge_t{1, 4});
      check(equality, report_line("Set weight which is now shared with a different weight"), edge3, edge_t{5, 4});

      edge3.mutate_weight([](int& a) { a = -a;});
      check(equality, report_line(""), edge, edge_t{2, 9});
      check(equality, report_line("Mutate shared weight"), edge2, edge_t{1, -4});
      check(equality, report_line("Mutate shared weight"), edge3, edge_t{5, -4});
    }

    void test_edges::test_partial_edge_indep_weight()
    {
      using edge_t = partial_edge<by_value<int>, null_meta_data>;
      static_assert(2 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 7};
      check(equivalence, report_line("Construction"), edge, 2, 7);

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 7});

      edge.weight(-5);
      check(equality, report_line("Set weight"), edge, edge_t{3, -5});

      edge_t edge2(5, edge);
      check(equivalence, report_line("Construction with by_value weight"), edge2, 5, -5);
      check(equality, report_line(""), edge, edge_t{3, -5});

      edge.mutate_weight([](int& a) { a *= 2;} );
      check(equality, report_line("Mutate weight"), edge, edge_t{3, -10});
      check(equality, report_line(""), edge2, edge_t{5, -5});

      check_semantics(report_line("Standard semantics"), edge2, edge);
    }

    void test_edges::test_partial_edge_meta_data()
    {
      using edge_t = partial_edge<by_value<null_weight>, float, std::size_t>;
      static_assert(2 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 0.5f};
      check(equivalence, report_line("Construction"), edge, 2, 0.5f);

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 0.5f});

      edge.meta_data(0.7f);
      check(equality, report_line("Change meta data"), edge, edge_t{3, 0.7f});

      constexpr edge_t edge1{5, 0.8f};
      check_semantics(report_line("Standard semantics"), edge1, edge);

      constexpr edge_t edge2{7, edge1};
      check(equality, report_line(""), edge2, edge_t{7, 0.8f});

      constexpr edge_t edge3{7};
      check(equivalence, report_line("Construction"), edge3, 7, 0.0f);
    }

    void test_edges::test_partial_edge_indep_weight_meta_data()
    {
      using edge_t = partial_edge<by_value<double>, float, std::size_t>;
      static_assert(3 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 1.0, 0.5f};
      check(equivalence, report_line("Construction"), edge, 2, 1.0, 0.5f);

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 1.0, 0.5f});

      edge.weight(1.2);
      check(equality, report_line("Change weight"), edge, edge_t{3, 1.2, 0.5f});

      edge.meta_data(0.7f);
      check(equality, report_line("Change meta data"), edge, edge_t{3, 1.2, 0.7f});

      constexpr edge_t edge1{5, 1.0, 0.8f};
      check_semantics(report_line("Standard semantics"), edge1, edge);

      constexpr edge_t edge2{7, edge1};
      check(equality, report_line(""), edge2, edge_t{7, 1.0, 0.8f});

      constexpr edge_t edge3{7};
      check(equivalence, report_line("Construction"), edge3, 7, 0.0, 0.0f);
    }

    void test_edges::test_partial_edge_shared_weight_meta_data()
    {
      using edge_t = partial_edge<shared<double>, float, std::size_t>;
      static_assert(sizeof(std::shared_ptr<double>) + 2*sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 1.0, 0.5f};
      check(equivalence, report_line("Construction"), edge, 2, 1.0, 0.5f);

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 1.0, 0.5f});

      edge.weight(1.2);
      check(equality, report_line("Change weight"), edge, edge_t{3, 1.2, 0.5f});

      edge.meta_data(0.7f);
      check(equality, report_line("Change meta data"), edge, edge_t{3, 1.2, 0.7f});

      const edge_t edge1{5, 1.0, 0.8f};
      check_semantics(report_line("Standard semantics"), edge1, edge);

      const edge_t edge2{7, edge1};
      check(equality, report_line(""), edge2, edge_t{7, 1.0, 0.8f});

      const edge_t edge3{7};
      check(equivalence, report_line("Construction"), edge3, 7, 0.0, 0.0f);
    }


    void test_edges::test_plain_embedded_partial_edge()
    {
      using edge_t = embedded_partial_edge<by_value<null_weight>, null_meta_data>;
      static_assert(2*sizeof(std::size_t) == sizeof(edge_t));

      using compact_edge_t
        = embedded_partial_edge<by_value<null_weight>, null_meta_data, unsigned char>;
      static_assert(2*sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e1{0, 4};
      check(equivalence, report_line("Construction"), e1, 0, 4);

      e1.target_node(1);
      check(equality, report_line("Change target"), e1, edge_t{1, 4});

      e1.complementary_index(5);
      check(equality, report_line("Change complementary index"), e1, edge_t{1, 5});

      edge_t e2{10, 10};
      check(equivalence, report_line("Construction"), e2, 10, 10);

      check_semantics(report_line("Standard semantics"), e2, e1);
    }

    void test_edges::test_embedded_partial_edge_indep_weight()
    {
      using edge_t = embedded_partial_edge<by_value<double>, null_meta_data>;
      static_assert(2*sizeof(std::size_t) + sizeof(double) == sizeof(edge_t));

      constexpr edge_t edge1{1, 2, 5.0};
      check(equivalence, report_line("Construction"), edge1, 1, 2, 5.0);

      edge_t edge2{3, 7, edge1};
      check(equality, report_line("Construction with by_value weight"), edge2, edge_t{3, 7, 5.0});

      edge2.target_node(13);
      check(equality, report_line("Change target node"), edge2, edge_t{13, 7, 5.0});

      edge2.complementary_index(2);
      check(equality, report_line("Change complementary index"), edge2, edge_t{13, 2, 5.0});

      edge2.weight(5.6);
      check(equality, report_line("Change weight"), edge2, edge_t{13, 2, 5.6});

      check_semantics(report_line("Standard semantics"), edge2, edge1);
    }

    void test_edges::test_embedded_partial_edge_shared_weight()
    {
      using edge_t = embedded_partial_edge<shared<double>, null_meta_data>;
      static_assert(sizeof(std::shared_ptr<double>) + 2*sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge1{1, 2, 5.0};
      check(equivalence, report_line("Construction"), edge1, 1, 2, 5.0);

      edge_t edge2{3, 7, edge1};
      check(equality, report_line("Construction with shared weight"), edge2, edge_t{3, 7, 5.0});

      edge2.target_node(13);
      check(equality, report_line("Change target node"), edge2, edge_t{13, 7, 5.0});

      edge2.complementary_index(2);
      check(equality, report_line("Change complementary index"), edge2, edge_t{13, 2, 5.0});

      edge2.weight(5.6);
      check(equality, report_line("Change weight"), edge2, edge_t{13, 2, 5.6});
      check(equality, report_line("Induced change in shared weight"), edge1, edge_t{1, 2, 5.6});

      check_semantics(report_line("Standard semantics"), edge2, edge1);
    }

    void test_edges::test_embedded_partial_edge_meta_data()
    {
      using edge_t = embedded_partial_edge<by_value<null_weight>, float, std::size_t>;
      static_assert(3 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 1, 0.5f};
      check(equivalence, report_line("Construction"), edge, 2, 1, 0.5f);

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 1, 0.5f});

      edge.complementary_index(4);
      check(equality, report_line("Change complementary index"), edge, edge_t{3, 4, 0.5f});

      edge.meta_data(0.7f);
      check(equality, report_line("Set weight"), edge, edge_t{3, 4, 0.7f});

      constexpr edge_t edge1{5, 8, 0.8f};
      check_semantics(report_line("Standard semantics"), edge1, edge);

      constexpr edge_t edge2{7, 4, edge1};
      check(equality, report_line(""), edge2, edge_t{7, 4, 0.8f});

      constexpr edge_t edge3{8, 2};
      check(equivalence, report_line("Construction"), edge3, 8, 2, 0.0f);
    }

    void test_edges::test_embedded_partial_edge_indep_weight_meta_data()
    {
      using edge_t = embedded_partial_edge<by_value<double>, float, std::size_t>;
      static_assert(4 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 5, 1.0, 0.5f};
      check(equivalence, report_line("Construction"), edge, 2, 5, 1.0, 0.5f);

      edge.complementary_index(6);
      check(equality, report_line("Change complementary index"), edge, edge_t{2, 6, 1.0, 0.5f});

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 6, 1.0, 0.5f});

      edge.weight(1.2);
      check(equality, report_line("Change weight"), edge, edge_t{3, 6, 1.2, 0.5f});

      edge.meta_data(0.7f);
      check(equality, report_line("Change meta data"), edge, edge_t{3, 6, 1.2, 0.7f});

      constexpr edge_t edge1{5, 7, 1.0, 0.8f};
      check_semantics(report_line("Standard semantics"), edge1, edge);

      constexpr edge_t edge2{7, 4, edge1};
      check(equality, report_line(""), edge2, edge_t{7, 4, 1.0, 0.8f});

      constexpr edge_t edge3{8, 2};
      check(equivalence, report_line("Construction"), edge3, 8, 2, 0.0, 0.0f);
    }

    void test_edges::test_embedded_partial_edge_shared_weight_meta_data()
    {
      using edge_t = embedded_partial_edge<shared<double>, float, std::size_t>;
      static_assert(sizeof(std::shared_ptr<double>) + 3*sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 5, 1.0, 0.5f};
      check(equivalence, report_line("Construction"), edge, 2, 5, 1.0, 0.5f);

      edge.complementary_index(6);
      check(equality, report_line("Change complementary index"), edge, edge_t{2, 6, 1.0, 0.5f});

      edge.target_node(3);
      check(equality, report_line("Change target node"), edge, edge_t{3, 6, 1.0, 0.5f});

      edge.weight(1.2);
      check(equality, report_line("Change weight"), edge, edge_t{3, 6, 1.2, 0.5f});

      edge.meta_data(0.7f);
      check(equality, report_line("Change meta data"), edge, edge_t{3, 6, 1.2, 0.7f});

      const edge_t edge1{5, 7, 1.0, 0.8f};
      check_semantics(report_line("Standard semantics"), edge1, edge);

      const edge_t edge2{7, 4, edge1};
      check(equality, report_line(""), edge2, edge_t{7, 4, 1.0, 0.8f});

      const edge_t edge3{8, 2};
      check(equivalence, report_line("Construction"), edge3, 8, 2, 0.0, 0.0f);
    }
  }
}
