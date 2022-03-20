////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "EdgeTestingDiagnostics.hpp"

#include "sequoia/Core/ObjectHandling/Handlers.hpp"
#include "sequoia/Core/ObjectHandling/UniformWrapper.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_edge_false_positives::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_edge_false_positives::run_tests()
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


  void test_edge_false_positives::test_plain_partial_edge()
  {
    using namespace maths;
    using namespace object;
    using edge_t = partial_edge<independent<object::uniform_wrapper<null_weight>>>;

    check(equality, LINE("Differing target indices"), edge_t{0}, edge_t{1});

    using compact_edge_t = partial_edge<independent<object::uniform_wrapper<null_weight>>, unsigned char>;

    check(equality, LINE("Differing target indices"), compact_edge_t{10}, compact_edge_t{255});
  }

  void test_edge_false_positives::test_partial_edge_indep_weight()
  {
    using namespace maths;
    using namespace object;

    using edge_t = partial_edge<independent<object::uniform_wrapper<int>>>;

    check(equality, LINE("Differing targets, identical weights"), edge_t{0,0}, edge_t{1,0});
    check(equality, LINE("Differing targets, identical weights"), edge_t{0,5}, edge_t{1,5});
    check(equality, LINE("Identical targets, differeing weights"), edge_t{0,0}, edge_t{0,1});
    check(equality, LINE("Identical targets, differeing weights"), edge_t{0,5}, edge_t{0,6});
    check(equality, LINE("Differing targets and weights"), edge_t{0,0}, edge_t{1,1});
    check(equality, LINE("Differing targets and weights"), edge_t{0,1}, edge_t{2,3});
  }

  void test_edge_false_positives::test_partial_edge_shared_weight()
  {
    using namespace maths;
    using namespace object;

    using edge_t = partial_edge<shared<object::uniform_wrapper<int>>>;

    check(equality, LINE("Differing targets, identical weights"), edge_t{0,0}, edge_t{1,0});
    check(equality, LINE("Differing targets, identical weights"), edge_t{0,5}, edge_t{1,5});
    check(equality, LINE("Identical targets, differeing weights"), edge_t{0,0}, edge_t{0,1});
    check(equality, LINE("Identical targets, differeing weights"), edge_t{0,5}, edge_t{0,6});
    check(equality, LINE("Differing targets and weights"), edge_t{0,0}, edge_t{1,1});
    check(equality, LINE("Differing targets and weights"), edge_t{0,1}, edge_t{2,3});
  }

  void test_edge_false_positives::test_plain_embedded_partial_edge()
  {
    using namespace maths;
    using namespace object;

    using edge_t = partial_edge<shared<object::uniform_wrapper<int>>>;

    check(equality, LINE("Differing targets, identical complementary indices"), edge_t{0,0}, edge_t{1,0});
    check(equality, LINE("Differing targets, identical complementary indices"), edge_t{0,5}, edge_t{1,5});
    check(equality, LINE("Identical targets, differeing complementary indices"), edge_t{0,0}, edge_t{0,1});
    check(equality, LINE("Identical targets, differeing complementary indices"), edge_t{0,5}, edge_t{0,6});
    check(equality, LINE("Differing targets and complementary indices"), edge_t{0,0}, edge_t{1,1});
    check(equality, LINE("Differing targets and complementary indices"), edge_t{0,1}, edge_t{1,0});
  }

  void test_edge_false_positives::test_embedded_partial_edge_indep_weight()
  {
    using namespace maths;
    using namespace object;

    using edge_t = embedded_partial_edge<independent<object::uniform_wrapper<double>>>;

    check(equality, LINE("Differing targets, identical complementary indices and weights"), edge_t{0,0,0.0}, edge_t{1,0,0.0});
    check(equality, LINE("Differing targets, identical complementary indices and weights"), edge_t{1,10,0.0}, edge_t{0,10,0.0});
    check(equality, LINE("Differing targets, identical complementary indices and weights"), edge_t{0,20,5.0}, edge_t{1,20,5.0});

    check(equality, LINE("Differing complementary indices, identical targets and weights"), edge_t{0,0,0.0}, edge_t{0,1,0.0});
    check(equality, LINE("Differing complementary indices, identical targets and weights"), edge_t{3,0,0.0}, edge_t{3,1,0.0});
    check(equality, LINE("Differing complementary indices, identical targets and weights"), edge_t{4,0,-7.0}, edge_t{4,1,-7.0});

    check(equality, LINE("Differing weights, identical targets and complementary indices"), edge_t{0,0,0.0}, edge_t{0,0,1.0});
    check(equality, LINE("Differing weights, identical targets and complementary indices"), edge_t{3,0,0.0}, edge_t{3,0,1.0});
    check(equality, LINE("Differing weights, identical targets and complementary indices"), edge_t{3,4,0.0}, edge_t{3,4,1.0});

    check(equality, LINE("Differing targets and complementary indices, identical weights"), edge_t{0,1,0.0}, edge_t{1,0,0.0});
    check(equality, LINE("Differing targets and weights, identical complementary indices"), edge_t{0,1,6.0}, edge_t{2,1,5.0});
    check(equality, LINE("Differing complementary indices and weights, identical targets"), edge_t{0,1,4.0}, edge_t{0,3,5.0});

    check(equality, LINE("Differing targets, complementary indices and weights"), edge_t{0,1,2.0}, edge_t{1,0,5.0});
  }

  void test_edge_false_positives::test_embedded_partial_edge_shared_weight()
  {
    using namespace maths;
    using namespace object;

     using edge_t = embedded_partial_edge<independent<object::uniform_wrapper<double>>>;

    check(equality, LINE("Differing targets, identical complementary indices and weights"), edge_t{0,0,0.0}, edge_t{1,0,0.0});
    check(equality, LINE("Differing targets, identical complementary indices and weights"), edge_t{0,10,0.0}, edge_t{1,10,0.0});
    check(equality, LINE("Differing targets, identical complementary indices and weights"), edge_t{0,20,5.0}, edge_t{1,20,5.0});

    check(equality, LINE("Differing complementary indices, identical targets and weights"), edge_t{0,0,0.0}, edge_t{0,1,0.0});
    check(equality, LINE("Differing complementary indices, identical targets and weights"), edge_t{3,0,0.0}, edge_t{3,1,0.0});
    check(equality, LINE("Differing complementary indices, identical targets and weights"), edge_t{4,0,-7.0}, edge_t{4,1,-7.0});

    check(equality, LINE("Differing weights, identical targets and complementary indices"), edge_t{0,0,0.0}, edge_t{0,0,1.0});
    check(equality, LINE("Differing weights, identical targets and complementary indices"), edge_t{3,0,0.0}, edge_t{3,0,1.0});
    check(equality, LINE("Differing weights, identical targets and complementary indices"), edge_t{3,4,0.0}, edge_t{3,4,1.0});

    check(equality, LINE("Differing targets and complementary indices, identical weights"), edge_t{0,1,0.0}, edge_t{1,0,0.0});
    check(equality, LINE("Differing targets and weights, identical complementary indices"), edge_t{0,1,6.0}, edge_t{2,1,5.0});
    check(equality, LINE("Differing complementary indices and weights, identical targets"), edge_t{0,1,4.0}, edge_t{0,3,5.0});

    check(equality, LINE("Differing targets, complementary indices and weights"), edge_t{0,1,2.0}, edge_t{1,0,5.0});
  }

  void test_edge_false_positives::test_plain_edge()
  {
    using namespace maths;
    using namespace object;

    using edge_t = edge<independent<object::uniform_wrapper<null_weight>>>;

    check(equality, LINE("Differing targets, identical soures"), edge_t{0,0}, edge_t{0,1});
    check(equality, LINE("Differing targets, identical soures"), edge_t{4,1}, edge_t{4,0});

    check(equality, LINE("Differing soures, identical targets"), edge_t{0,0}, edge_t{1,0});
    check(equality, LINE("Differing soures, identical targets"), edge_t{1,8}, edge_t{0,8});

    check(equality, LINE("Differing soures and targets"), edge_t{0,1}, edge_t{1,0});

    check(equality, LINE("Differing inversion flag"), edge_t{0, inversion_constant<true>{}}, edge_t{0, inversion_constant<false>{}});
  }

  void test_edge_false_positives::test_weighted_edge()
  {
    using namespace maths;
    using namespace object;

    using edge_t = edge<independent<object::uniform_wrapper<double>>>;

    check(equality, LINE("Differing targets, identical soures and weight"), edge_t{0,0,0.0}, edge_t{0,1,0.0});
    check(equality, LINE("Differing targets, identical soure and weights"), edge_t{0,10,0.0}, edge_t{1,10,0.0});
    check(equality, LINE("Differing targets, identical soure and weights"), edge_t{0,20,5.0}, edge_t{1,20,5.0});
    check(equality, LINE("Differing inversion flag"), edge_t{0, inversion_constant<true>{}, 0.0}, edge_t{0, inversion_constant<false>{}, 0.0});

    check(equality, LINE("Differing soures, identical targets and weights"), edge_t{0,0,0.0}, edge_t{0,1,0.0});
    check(equality, LINE("Differing soures, identical targets and weights"), edge_t{3,0,0.0}, edge_t{3,1,0.0});
    check(equality, LINE("Differing soures, identical targets and weights"), edge_t{4,0,-7.0}, edge_t{4,1,-7.0});

    check(equality, LINE("Differing weights, identical targets and soures"), edge_t{0,0,0.0}, edge_t{0,0,1.0});
    check(equality, LINE("Differing weights, identical targets and soures"), edge_t{3,0,0.0}, edge_t{3,0,1.0});
    check(equality, LINE("Differing weights, identical targets and soures"), edge_t{3,4,0.0}, edge_t{3,4,1.0});

    check(equality, LINE("Differing targets and soures, identical weights"), edge_t{0,1,0.0}, edge_t{1,0,0.0});
    check(equality, LINE("Differing targets and weights, identical soures"), edge_t{0,1,6.0}, edge_t{2,1,5.0});
    check(equality, LINE("Differing soures and weights, identical targets"), edge_t{0,1,4.0}, edge_t{0,3,5.0});

    check(equality, LINE("Differing targets, soures and weights"), edge_t{0,1,2.0}, edge_t{1,0,5.0});
  }

  void test_edge_false_positives::test_plain_embedded_edge()
  {
    using namespace maths;
    using namespace object;

    using edge_t = embedded_edge<independent<object::uniform_wrapper<null_weight>>>;

    check(equality, LINE("Differing targets, identical soures and complementary indices"), edge_t{0,0,0}, edge_t{0,1,0});
    check(equality, LINE("Differing targets, identical soure and complementary indices"), edge_t{0,10,0}, edge_t{1,10,0});
    check(equality, LINE("Differing targets, identical soure and complementary indices"), edge_t{0,0,20}, edge_t{1,0,20});
    check(equality, LINE("Differing inversion flag"), edge_t{0, inversion_constant<true>{}, 0}, edge_t{0, inversion_constant<false>{}, 0});

    check(equality, LINE("Differing soures, identical targets and complementary indices"), edge_t{0,0,0}, edge_t{0,1,0});
    check(equality, LINE("Differing soures, identical targets and complementary indices"), edge_t{3,0,0}, edge_t{3,1,0});
    check(equality, LINE("Differing soures, identical targets and complementary indices"), edge_t{4,0,7}, edge_t{4,1,7});

    check(equality, LINE("Differing complementary indicess, identical targets and soures"), edge_t{0,0,0}, edge_t{0,0,1});
    check(equality, LINE("Differing complementary indicess, identical targets and soures"), edge_t{3,0,0}, edge_t{3,0,1});
    check(equality, LINE("Differing complementary indicess, identical targets and soures"), edge_t{3,4,0}, edge_t{3,4,1});

    check(equality, LINE("Differing targets and soures, identical complementary indices"), edge_t{0,1,0}, edge_t{1,0,0});
    check(equality, LINE("Differing targets and complementary indices, identical soures"), edge_t{0,1,6}, edge_t{2,1,5});
    check(equality, LINE("Differing soures and complementary indices, identical targets"), edge_t{0,1,4}, edge_t{0,3,5});

    check(equality, LINE("Differing targets, soures and complementary indicess"), edge_t{0,1,2}, edge_t{2,0,1});
  }

  void test_edge_false_positives::test_embedded_edge_indep_weight()
  {
    using namespace maths;
    using namespace object;

    using edge_t = embedded_edge<independent<object::uniform_wrapper<double>>>;

    check(equality, LINE("Differing targets, identical soures, complementary indices and weights"), edge_t{0,0,0,0.0}, edge_t{0,1,0,0.0});
    check(equality, LINE("Differing soures, identical targets, complementary indices and weights"), edge_t{1,0,0,0.0}, edge_t{0,0,0,0.0});
    check(equality, LINE("Differing complementary indices, identical soures, targets and weights"), edge_t{0,0,0,0.0}, edge_t{0,0,1,0.0});
    check(equality, LINE("Differing weights, identical soures, targets, complementary indices"), edge_t{0,0,0,0.0}, edge_t{0,0,0,1.0});
    check(equality, LINE("Differing inversion flag"), edge_t{0, inversion_constant<true>{}, 0, 0.0}, edge_t{0, inversion_constant<false>{}, 0, 0.0});

  }

  void test_edge_false_positives::test_embedded_edge_shared_weight()
  {
    using namespace maths;
    using namespace object;

    using edge_t = embedded_edge<shared<object::uniform_wrapper<double>>>;

    check(equality, LINE("Differing targets, identical soures, complementary indices and weights"), edge_t{0,0,0,0.0}, edge_t{0,1,0,0.0});
    check(equality, LINE("Differing soures, identical targets, complementary indices and weights"), edge_t{1,0,0,0.0}, edge_t{0,0,0,0.0});
    check(equality, LINE("Differing complementary indices, identical soures, targets and weights"), edge_t{0,0,0,0.0}, edge_t{0,0,1,0.0});
    check(equality, LINE("Differing weights, identical soures, targets, complementary indices"), edge_t{0,0,0,0.0}, edge_t{0,0,0,1.0});
    check(equality, LINE("Differing inversion flag"), edge_t{0, inversion_constant<true>{}, 0, 0.0}, edge_t{0, inversion_constant<false>{}, 0, 0.0});
  }
}
