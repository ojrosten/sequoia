////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "EdgeTest.hpp"

#include "EdgeTestingUtilities.hpp"

#include "Ownership.hpp"
#include "ProtectiveWrapper.hpp"

#include <complex>
#include <list>
#include <vector>

namespace sequoia
{
  namespace testing
  {

    [[nodiscard]]
    std::string_view test_edges::source_file() const noexcept
    {
      return __FILE__;
    }

    void test_edges::run_tests()
    {
      test_plain_partial_edge();      
      test_partial_edge_indep_weight(); 
      test_partial_edge_shared_weight();

      test_plain_embedded_partial_edge();
      test_embedded_partial_edge_indep_weight();
      test_embedded_partial_edge_shared_weight();
      
      test_plain_edge();      
      test_weighted_edge();

      test_plain_embedded_edge();
      test_embedded_edge_indep_weight();
      test_embedded_edge_shared_weight();      
    }

    void test_edges::test_plain_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      static_assert(sizeof(std::size_t) == sizeof(edge_t));
      
      using compact_edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e1{0};
      check_equality(LINE("Construction"), e1, edge_t{0});

      e1.target_node(1);
      check_equality(LINE("Changing target node"), e1, edge_t{1});
  
      edge_t e2{3};
      check_equality(LINE(""), e2, edge_t{3});
      
      check_semantics(LINE("Standard semantics"), e2, e1);
    }
    
    void test_edges::test_partial_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<int, shared, utilities::protective_wrapper<int>>;

      edge_t edge{1, 4};
      check_equality(LINE("Construction"), edge, edge_t{1, 4});

      edge.weight(5);
      check_equality(LINE("Set weight"), edge, edge_t{1, 5});

      edge.mutate_weight([](auto& i){ i -= 6;});
      check_equality(LINE("Manipulate weight"), edge, edge_t{1, -1});

      edge.target_node(2);
      check_equality(LINE("Change target node"), edge, edge_t{2, -1});

      edge_t edge1{2,-7};
      check_equality(LINE("Construction"), edge1, edge_t{2, -7});

      check_semantics(LINE("Standard Semantics"), edge1, edge);      

      edge_t edge2{6, edge};
      check_equality(LINE("Construction with shared weight"), edge2, edge_t{6, -1});

      edge.target_node(1);
      check_equality(LINE("Change target node of edge with shared weight"), edge, edge_t{1, -1});
      check_equality(LINE(""), edge2, edge_t{6, -1});

      edge2.target_node(5);
      check_equality(LINE("Change target node of edge with shared weight"), edge, edge_t{1, -1});
      check_equality(LINE(""), edge2, edge_t{5, -1});      
      
      edge2.weight(-3);
      check_equality(LINE("Implicit change of shared weight"), edge, edge_t{1, -3});
      check_equality(LINE("Explicit change of shared weight"), edge2, edge_t{5, -3});

      check_semantics(LINE("Standard semantics with shared weight"), edge2, edge);

      edge_t edge3(2, 8);
      check_equality(LINE(""), edge3, edge_t{2, 8});

      check_semantics(LINE("Standard semantics with one having a shared weight"), edge2, edge);
 
      std::swap(edge, edge2);
      std::swap(edge, edge3);
      check_equality(LINE("Swap edge with one of an edge sharing pair"), edge, edge_t{2, 8});
      check_equality(LINE("Swap edge with one of an edge sharing pair"), edge2, edge_t{1, -3});
      check_equality(LINE("Swap edge with one of an edge sharing pair"), edge3, edge_t{5, -3});

      edge.mutate_weight([](int& a) { ++a; });
      check_equality(LINE("Mutate weight of edge which previously but no longer shares its weight"), edge, edge_t{2, 9});
      check_equality(LINE(""), edge2, edge_t{1, -3});
      check_equality(LINE(""), edge3, edge_t{5, -3});

      edge2.weight(4);
      check_equality(LINE(""), edge, edge_t{2, 9});
      check_equality(LINE("Set weight which is now shared with a different weight"), edge2, edge_t{1, 4});
      check_equality(LINE("Set weight which is now shared with a different weight"), edge3, edge_t{5, 4});

      edge3.mutate_weight([](int& a) { a = -a;});
      check_equality(LINE(""), edge, edge_t{2, 9});
      check_equality(LINE("Mutate shared weight"), edge2, edge_t{1, -4});
      check_equality(LINE("Mutate shared weight"), edge3, edge_t{5, -4});
    }

    void test_edges::test_partial_edge_indep_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<int, independent, utilities::protective_wrapper<int>>;
      static_assert(2 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 7};
      check_equality(LINE("Construction"), edge, edge_t{2, 7});

      edge.target_node(3);
      check_equality(LINE("Change target node"), edge, edge_t{3, 7});

      edge.weight(-5);
      check_equality(LINE("Set weight"), edge, edge_t{3, -5});

      edge_t edge2(5, edge);
      check_equality(LINE("Construction with independent weight"), edge2, edge_t{5, -5});
      check_equality(LINE(""), edge, edge_t{3, -5});

      edge.mutate_weight([](int& a) { a *= 2;} );
      check_equality(LINE("Mutate weight"), edge, edge_t{3, -10});
      check_equality(LINE(""), edge2, edge_t{5, -5});

      check_semantics(LINE("Standard semantics"), edge2, edge);
    }

    void test_edges::test_plain_embedded_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;     
      
      using edge_t = embedded_partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      static_assert(2*sizeof(std::size_t) == sizeof(edge_t));

      using compact_edge_t
        = embedded_partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(2*sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e1{0, 4};
      check_equality(LINE("Construction"), e1, edge_t{0, 4});

      e1.target_node(1);
      check_equality(LINE("Change target"), e1, edge_t{1, 4});

      e1.complementary_index(5);
      check_equality(LINE("Change complementary index"), e1, edge_t{1, 5});

      edge_t e2{10, 10};
      check_equality(LINE("Construction"), e2, edge_t{10, 10});

      check_semantics(LINE("Standard semantics"), e2, e1);
    }
    
    void test_edges::test_embedded_partial_edge_indep_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_partial_edge<double, independent, utilities::protective_wrapper<double>>;
      static_assert(2*sizeof(std::size_t) + sizeof(double) == sizeof(edge_t));
      
      constexpr edge_t edge1{1, 2, 5.0};
      check_equality(LINE("Construction"), edge1, edge_t{1, 2, 5.0});

      edge_t edge2{3, 7, edge1};
      check_equality(LINE("Construction with independent weight"), edge2, edge_t{3, 7, 5.0});

      edge2.target_node(13);
      check_equality(LINE("Change target node"), edge2, edge_t{13, 7, 5.0});

      edge2.complementary_index(2);
      check_equality(LINE("Change complementary index"), edge2, edge_t{13, 2, 5.0});

      edge2.weight(5.6);
      check_equality(LINE("Change weight"), edge2, edge_t{13, 2, 5.6});

      check_semantics(LINE("Standard semantics"), edge2, edge1); 
    }

    void test_edges::test_embedded_partial_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_partial_edge<double, shared, utilities::protective_wrapper<double>>;
      
      edge_t edge1{1, 2, 5.0};
      check_equality(LINE("Construction"), edge1, edge_t{1, 2, 5.0});

      edge_t edge2{3, 7, edge1};
      check_equality(LINE("Construction with shared weight"), edge2, edge_t{3, 7, 5.0});

      edge2.target_node(13);
      check_equality(LINE("Change target node"), edge2, edge_t{13, 7, 5.0});

      edge2.complementary_index(2);
      check_equality(LINE("Change complementary index"), edge2, edge_t{13, 2, 5.0});

      edge2.weight(5.6);
      check_equality(LINE("Change weight"), edge2, edge_t{13, 2, 5.6});
      check_equality(LINE("Induced change in shared weight"), edge1, edge_t{1, 2, 5.6});

      check_semantics(LINE("Standard semantics"), edge2, edge1);
    }


    void test_edges::test_plain_edge()
    {
      using namespace maths;

      using edge_t = edge<null_weight, utilities::protective_wrapper<null_weight>>;
      static_assert(2*sizeof(std::size_t) == sizeof(edge_t));

      using compact_edge_t = edge<null_weight, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(2 * sizeof(unsigned char) == sizeof(compact_edge_t));
      
      edge_t  
        e1(2, 3),
        e2(4, 6);
      
      check_equality(LINE("Construction"), e1, edge_t{2, 3});
      check_equality(LINE("Construction"), e2, edge_t{4, 6});

      e2.target_node(1);
      check_equality(LINE("Change target"), e2, edge_t{4, 1});

      e2.source_node(3);
      check_equality(LINE("Change source"), e2, edge_t{3, 1});      
      
      check_semantics(LINE("Standard semantics"), e2, e1);
      
      edge_t e3{4, inversion_constant<false>{}}, e4{5, inversion_constant<true>{}};
      check_equality(LINE("Construction"), e3, edge_t{4, inversion_constant<false>{}});
      check_equality(LINE("Construction inverted edge"), e4, edge_t{5, inversion_constant<true>{}});

      check_semantics(LINE("Standard semantics"), e4, e3);

      // Changing source / target node for inverted edge implicitly changes source
      e4.target_node(9);
      check_equality(LINE(""), e4, edge_t{9, inversion_constant<true>{}});

      e4.source_node(4);
      check_equality(LINE(""), e4, edge_t{4, inversion_constant<true>{}});
    }    

    void test_edges::test_weighted_edge()
    {
      using namespace maths;

      {
        using edge_t = edge<double, utilities::protective_wrapper<double>>;
        static_assert(sizeof(edge_t) == sizeof(double) + 2*sizeof(std::size_t));

        edge_t
          e1(0, 1, 1.2),
          e2(1, 0, -3.1);

        check_equality(LINE("Construction"), e1, edge_t{0, 1, 1.2});
        check_equality(LINE("Construction"), e2, edge_t{1, 0, -3.1});
        
        e1.weight(2.3);
        check_equality(LINE("Change weight"), e1, edge_t{0, 1, 2.3});

        e1.target_node(10);
        check_equality(LINE("Change target"), e1, edge_t{0, 10, 2.3});

        e1.source_node(5);
        check_equality(LINE("Change target"), e1, edge_t{5, 10, 2.3});

        check_semantics(LINE("Standard semantics"), e2, e1);
      }

      {
        using std::complex;
        using edge_t = edge<complex<float>, utilities::protective_wrapper<complex<float>>>;
        static_assert(sizeof(edge_t) == sizeof(std::complex<float>) + 2*sizeof(std::size_t));
      
        edge_t
          e1(3, inversion_constant<true>{}, 1.2f),
          e2(4, inversion_constant<false>{}, -1.3f, -1.4f);

        check_equality(LINE("Construction"), e1, edge_t{3, inversion_constant<true>{}, 1.2f});
        check_equality(LINE("Construction"), e2, edge_t{4, inversion_constant<false>{}, -1.3f, -1.4f});

        check_semantics(LINE(""), e2, e1);
      }

      {
        using std::vector;
        using edge_t = edge<vector<int>, utilities::protective_wrapper<vector<int>>>;

        edge_t
          e1(0, 0, 5, 1),
          e2(1, 1);

        check_equality(LINE("Construction"), e1, edge_t{0, 0, 5, 1});
        check_equality(LINE("Construction"), e2, edge_t{1,1});

        e1.weight(vector<int>{3, 2});
        check_equality(LINE("Change weight"), e1, edge_t{0, 0, vector<int>{3, 2}});

        e1.source_node(2);
        check_equality(LINE("Change source, no induced change in target"), e1, edge_t{2, 0, vector<int>{3, 2}});
        
        check_semantics(LINE("Standard semantics"), e2, e1);
      }
    }

    void test_edges::test_plain_embedded_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      check_equality(LINE(""), sizeof(edge_t), 3*sizeof(std::size_t));

      using compact_edge_t = embedded_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(3*sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e{3, 4, 1};
      check_equality(LINE("Construction"), e, edge_t{3, 4, 1});

      e.source_node(4);
      check_equality(LINE("Change source"), e, edge_t{4, 4, 1});

      e.target_node(5);
      check_equality(LINE("Change target"), e, edge_t{4, 5, 1});

      e.complementary_index(0);
      check_equality(LINE("Change complementary index"), e, edge_t{4, 5, 0});

      edge_t e1{7, inversion_constant<true>{}, 9};
      check_equality(LINE("Construction"), e1, edge_t{7, inversion_constant<true>{}, 9});

      e1.source_node(6);
      check_equality(LINE("Change source"), e1, edge_t{6, inversion_constant<true>{}, 9});

      e1.target_node(8);
      check_equality(LINE("Induced change to source"), e1, edge_t{8, inversion_constant<true>{}, 9});

      check_semantics(LINE("Standard semantics"), e1, e);

    }
    
    void test_edges::test_embedded_edge_indep_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_edge<double, independent, utilities::protective_wrapper<double>>;
      check_equality(LINE(""), sizeof(edge_t), 3*sizeof(std::size_t) + sizeof(double));

      constexpr edge_t e{3, 4, 1, 4.2};
      check_equality(LINE(""), e, edge_t{3, 4, 1, 4.2});

      edge_t e2{4, inversion_constant<true>{}, 1, 1.1};
      check_equality(LINE(""), e2, edge_t{4, inversion_constant<true>{}, 1, 1.1});

      e2.source_node(8);
      check_equality(LINE("Change source"), e2, edge_t{8, inversion_constant<true>{}, 1, 1.1});

      e2.target_node(7);
      check_equality(LINE("Induced change source"), e2, edge_t{7, inversion_constant<true>{}, 1, 1.1});

      e2.complementary_index(4);
      check_equality(LINE("Change complementary index"), e2, edge_t{7, inversion_constant<true>{}, 4, 1.1});

      e2.weight(-2.5);
      check_equality(LINE("Change weight"), e2, edge_t{7, inversion_constant<true>{}, 4, -2.5});


      check_semantics(LINE("Standard semantics"), e2, e);

    }

    void test_edges::test_embedded_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_edge<double, shared, utilities::protective_wrapper<double>>;

        edge_t e{10, 11, 0, -1.2};
        check_equality(LINE("Construction"), e, edge_t{10, 11, 0, -1.2});

        e.source_node(9);
        check_equality(LINE("Change source node"), e, edge_t{9, 11, 0, -1.2});

        e.weight(5.2);
        check_equality(LINE("Change weight"), e, edge_t{9, 11, 0, 5.2});

        e.complementary_index(3);
        check_equality(LINE("Change complementary index"), e, edge_t{9, 11, 3, 5.2});

        e.target_node(0);
        check_equality(LINE("Change target node"), e, edge_t{9, 0, 3, 5.2});

        edge_t e2{6, inversion_constant<true>{}, 4, 0.0};
        check_equality(LINE("Construction"), e2, edge_t{6, inversion_constant<true>{}, 4, 0.0});

        e2.source_node(5);
        check_equality(LINE("Change source node, inducing change in target"), e2, edge_t{5, inversion_constant<true>{}, 4, 0.0});
        check_semantics(LINE("Standard semantics"), e2, e);

        edge_t e3{8, inversion_constant<false>{}, 3, e};
        check_equality(LINE("Construction"), e3, edge_t{8, 8, 3, 5.2});

        e3.weight(0.0);
        check_equality(LINE("Induced change to shared weight"), e, edge_t{9, 0, 3, 0.0});
        check_equality(LINE("Change to shared weight"), e3, edge_t{8, 8, 3, 0.0});

        check_semantics(LINE("Standard semantics"), e3, e);
        check_semantics(LINE("Standard semantics"), e2, e3);
    }
  }
}
