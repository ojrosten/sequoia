////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Useful specializations for the class template value_tester.

    The specializations are for various types defined in `std`. Internally,
    `check(equality,...)` / `check(equivalence,...)` are used meaning that there will be automatic,
    recursive dispatch to other specializations of `value_tester`, if appropriate. For example,
    consider two instances of `std::pair<T, U>`, `x` and `y`. The utilities in these
    headers mean the call

    <pre>
    check(equality, "descripion", logger, x, y);
    </pre>

    will automatically call

    <pre>
    check(equality, "automatically enhanced desciption", logger, x.first, y,first)
    </pre>

    and similarly for the second element. In turn, this nested `check(equality, ...)` will use
    a specialization of the `value_tester` for `T`, should it exist. As
    usual, if the specialization for `T` does not exist, but `T` may be interpreted as
    a range holding a type `V`, then everything will simply work, provided either that
    there exists a specialization of the `value_tester` for `V` or `V` is serializable.

    However, all of this begs the question as to what happens in the above example if
    one or both of `U` and `T` do not support `equality` checking, but rather only
    offer `equivalence` or `weak_equivalence`. If both types have the same characteristics,
    then the top level call can be made using the appropriate tag. However, if they are
    different then instead clients should use

    <pre>
    check(with_best_available, "descripion", logger, x, y);
    </pre>

    This uses static reflection to choose the strongest check available for each of the
    nested types.

    The specializations themselves live in the headers included below, one per family of
    types, so that a test pays only for what it uses. This header pulls in all of them and
    is the right include when that is what you want.
 */

#include "sequoia/TestFramework/ChronoCheckers.hpp"
#include "sequoia/TestFramework/FunctionCheckers.hpp"
#include "sequoia/TestFramework/PathCheckers.hpp"
#include "sequoia/TestFramework/PointerCheckers.hpp"
#include "sequoia/TestFramework/ProductTypeCheckers.hpp"
#include "sequoia/TestFramework/SmartPointerCheckers.hpp"
#include "sequoia/TestFramework/StringCheckers.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"
