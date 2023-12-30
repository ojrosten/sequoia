////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#pragma once

// Bug report: https://developercommunity.visualstudio.com/t/constexpr-value-corrupted-at-runtime/10466594
#ifdef _MSC_VER
#define DODGY_MSVC_CONSTEXPR const
#else
#define DODGY_MSVC_CONSTEXPR constexpr
#endif

// Bug report: https://developercommunity.visualstudio.com/t/Wrapped-std::array-holding-type-whose-co/10465475
#ifdef _MSC_VER
#define ANOTHER_DODGY_MSVC_CONSTEXPR const
#else
#define ANOTHER_DODGY_MSVC_CONSTEXPR constexpr
#endif

#ifdef _MSC_VER
#define YET_ANOTHER_DODGY_MSVC_CONSTEXPR const
#else
#define YET_ANOTHER_DODGY_MSVC_CONSTEXPR constexpr
#endif
