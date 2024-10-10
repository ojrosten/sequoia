////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BitmaskFreeTest.hpp"

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    enum class mask { none = 0, a = 1, b = 2, c = 4 };

    template<class Stream>
    Stream& operator<<(Stream& stream, mask m)
    {
      return (stream << static_cast<int>(m));
    }
  }
}

NAMESPACE_SEQUOIA_AS_BITMASK
{
  template<>
  struct as_bitmask<sequoia::testing::mask> : std::true_type {};
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path bitmask_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void bitmask_free_test::run_tests()
  {
    using transition_checker_t = transition_checker<mask, check_ordering::no>;
    using mask_graph           = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    mask_graph g{
      { { edge_t{1, "none | a", [](mask m) -> mask { return m | mask::a; }} }, // 0: none
        { edge_t{3, "a | b", [](mask m) -> mask { return m | mask::b; }},
          edge_t{7, "a |= (b |= c)", [](mask m) -> mask {(m |= mask::b) |= mask::c; return m; }},
          edge_t{0, "a ^ a", [](mask m) -> mask { return m ^ mask::a; }},
          edge_t{1, "a & a", [](mask m) -> mask { return m & mask::a; }}
        }, // 1: a
        { }, // 2: b
        { edge_t{1, "a|b & ~b", [](mask m) -> mask { return m & ~mask::b; }},
          edge_t{2, "a|b & ~a", [](mask m) -> mask { return m & ~mask::a; }},
          edge_t{6, "((a|b) ^= a) ^= c", [](mask m) -> mask { (m ^= mask::a) ^= mask::c; return m; }}}, // 3: a|b
        {  }, // 4: c
        {  }, // 5: a|c
        { edge_t{5, "b|c ^ a|b", [](mask m) -> mask { return m ^ (mask::a | mask::b); }}}, // 6: b|c
        { edge_t{4, "(a | b | c) &= (b|c) &= b", [](mask m) -> mask {(m &= (mask::b | mask::c)) &= mask::c; return m; }} } // 7: a|b|c
      },
      {mask::none, mask::a, mask::b, static_cast<mask>(3), mask::c, static_cast<mask>(5), static_cast<mask>(6), static_cast<mask>(7)}
    };

    auto checker{
        [this](std::string_view description, mask obtained, mask prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker_t::check(report(""), g, checker);
  }
}
