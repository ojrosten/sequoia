////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  /** \brief The order `meta::type_comparator` imposes on the spaces which can appear in a tensor
      product.

      That order is what makes \f$A \otimes B\f$ and \f$B \otimes A\f$ the same type, so it is
      the precondition for everything `physical_value_meta_free_test` checks about reduction rather
      than a part of it.
   */
  class space_ordering_meta_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void test_type_comparator();

    void test_type_comparator_ordering_laws();
  };
}
