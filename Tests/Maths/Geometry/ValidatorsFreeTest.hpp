////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief What the validators *do*, as opposed to what they are.

    SpacesMetaFreeTest asks the validators trait questions and BoundsFreeTest
    pins the messages `throwing_validator` throws. Neither exercises a validator
    which accepts, and until this test `identity_validator` had no run-time
    coverage at all - which matters, because it is the validator every
    unconstrained space uses.
 */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class validators_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void test_identity_validator();

    void test_throwing_validator_accepts();

    void test_partial_validators();
  };
}
