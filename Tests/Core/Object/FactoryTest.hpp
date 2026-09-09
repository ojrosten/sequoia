////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "FactoryTestingUtilities.hpp"

namespace sequoia::testing
{
  class factory_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    /** Checks bulk creation against the same prediction the individual `make` checks use, so the
        two cannot drift apart. Written once and instantiated for each factory the test builds.
     */

    template<class Factory, std::size_t N, class... Args>
    void check_bulk_creation(std::string_view description,
                             const Factory& f,
                             const std::array<std::pair<std::string, typename Factory::vessel>, N>& prediction,
                             const Args&... args);
  };
}
