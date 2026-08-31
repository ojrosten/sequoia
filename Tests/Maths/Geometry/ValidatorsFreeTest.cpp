////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file
    \brief What the validators *do*, as opposed to what they are.

    SpacesMetaFreeTest asks the validators trait questions and BoundsFreeTest
    pins the messages `throwing_validator` throws. Neither exercises a validator
    which accepts, and until this test `identity_validator` had no run-time
    coverage at all - which matters, because it is the validator every
    unconstrained space uses.
 */

#include "ValidatorsFreeTest.hpp"
#include "ValidatorTestingUtilities.hpp"

#include "sequoia/Maths/Geometry/Spaces.hpp"

#include <array>

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path validators_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void validators_free_test::run_tests()
  {
    test_identity_validator();
    test_throwing_validator_accepts();
    test_partial_validators();
  }

  /** identity_validator ignores its bounds entirely; the point is that it does so
      for each of its three overloads, and that the bounds it ignores may be ones
      the value plainly violates. */
  void validators_free_test::test_identity_validator()
  {
    constexpr identity_validator v{};
    constexpr coordinate_bounds<double> unitInterval{0.0, 1.0};

    check(equality, "A value within the bounds is returned unchanged", v(unitInterval, 0.5), 0.5);
    check(equality, "A value outside them is returned just as unchanged", v(unitInterval, 42.0), 42.0);
    check(equality, "So is one outside a half-line", v(half_line_bounds<double>, -1.0), -1.0);

    const std::array vals{2.0, -3.0};
    check(equality, "An array is passed through whole", v(unitInterval, vals), vals);
    check(equality,
          "As it is when each element has bounds of its own",
          v(std::array{unitInterval, unitInterval}, vals),
          vals);
  }

  /** BoundsFreeTest pins what throwing_validator rejects. This pins what it accepts,
      including the boundary values, which a half-open reading would get wrong. */
  void validators_free_test::test_throwing_validator_accepts()
  {
    constexpr throwing_validator v{};
    constexpr coordinate_bounds<double> unitInterval{0.0, 1.0};

    check(equality, "An interior value survives", v(unitInterval, 0.5), 0.5);
    check(equality, "The lower bound is inside the domain", v(unitInterval, 0.0), 0.0);
    check(equality, "So is the upper", v(unitInterval, 1.0), 1.0);

    const std::array vals{0.0, 0.25, 1.0};
    check(equality, "An array wholly inside survives", v(unitInterval, vals), vals);

    const std::array pair{0.5, -0.5};
    check(equality,
          "As does one where each element satisfies its own bounds",
          v(std::array{unitInterval, coordinate_bounds<double>{-1.0, 0.0}}, pair),
          pair);
  }

  /** The fixtures exist to separate the disjuncts of `validator_for`, which the
      production validators cannot do because they satisfy all of them. Their
      behaviour is worth pinning too, because it is what makes
      `defines_identity_validator_v` a declaration rather than an inference:
      single_value_validator checks nothing, exactly as identity_validator does,
      and still answers false. */
  void validators_free_test::test_partial_validators()
  {
    check(equality,
          "single_value_validator is transparent in fact, whatever it declares",
          single_value_validator{}(no_bounds<double>, 42.0),
          42.0);
    check(equality,
          "It ignores bounds the value violates, just as identity_validator does",
          single_value_validator{}(coordinate_bounds<double>{0.0, 1.0}, -7.0),
          -7.0);
    check(equality, "unusable_validator is callable, on the one thing it accepts",
          unusable_validator{}(3), 3);
  }
}
