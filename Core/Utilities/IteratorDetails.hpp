////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "TypeTraits.hpp"

/*! \file IteratorDetails.hpp
    \brief Implementation details for iterator.
 */

namespace sequoia::utilities
{
  namespace impl
  {
    template<class DereferencePolicy, class=std::void_t<>>
    struct provides_reference_type : std::false_type
    {};

    template<class DereferencePolicy>
    struct provides_reference_type<DereferencePolicy, std::void_t<typename DereferencePolicy::reference>>
      : std::true_type
    {};
    
    template<class DereferencePolicy>
    constexpr bool provides_reference_type_v{provides_reference_type<DereferencePolicy>::value};

    
    template<class DereferencePolicy, class=std::void_t<>>
    struct provides_proxy_type : std::false_type
    {};

    template<class DereferencePolicy>
    struct provides_proxy_type<DereferencePolicy, std::void_t<typename DereferencePolicy::proxy>>
      : std::true_type
    {};
    
    template<class DereferencePolicy>
    constexpr bool provides_proxy_type_v{provides_proxy_type<DereferencePolicy>::value};

    template<class DereferencePolicy>
    constexpr bool is_valid_v{provides_reference_type_v<DereferencePolicy>};

    template<class DerefPolicy1, class DerefPolicy2>
    constexpr bool are_compatible_v{
         (is_valid_v<DerefPolicy1> && is_valid_v<DerefPolicy2>)
      && (   (provides_proxy_type_v<DerefPolicy1> && provides_proxy_type_v<DerefPolicy2>)
          || (!provides_proxy_type_v<DerefPolicy1> && !provides_proxy_type_v<DerefPolicy2>))
    };       
    
    template<
      class DereferencePolicy,
      bool=provides_proxy_type_v<DereferencePolicy>
    >
    struct type_generator
    {
      using type = typename DereferencePolicy::proxy;
    };

    template<class DereferencePolicy>
    struct type_generator<DereferencePolicy, false>
    {
      using reference = typename DereferencePolicy::reference;
      using type = std::conditional_t<is_const_reference_v<reference>, reference, const reference>;
    };

    template<class DereferencePolicy>
    using type_generator_t = typename type_generator<DereferencePolicy>::type;

    template<class DereferencePolicy>
    constexpr bool provides_mutable_reference_v{
           std::is_reference_v<type_generator_t<DereferencePolicy>>
        && !is_const_reference_v<type_generator_t<DereferencePolicy>>
    };
  }
}
