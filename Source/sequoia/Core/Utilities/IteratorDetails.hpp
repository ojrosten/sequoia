////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "TypeTraits.hpp"

/*! \file
    \brief Implementation details for iterator.
 */

namespace sequoia::utilities
{
  template<class Policy>
  concept dereference_policy = requires() {
    typename Policy::value_type;
    typename Policy::reference;
    typename Policy::pointer;
  };

  namespace impl
  {
    template<class Policy>
    concept has_proxy_type = requires() {
      typename Policy::proxy;
    };
                                         
    template<dereference_policy DerefPolicy1, dereference_policy DerefPolicy2>
    inline constexpr bool are_compatible_v{
         (   (has_proxy_type<DerefPolicy1> && has_proxy_type<DerefPolicy2>)
          || (!has_proxy_type<DerefPolicy1> && !has_proxy_type<DerefPolicy2>))
    };       
    
    template<dereference_policy DereferencePolicy>
    struct type_generator
    {
      using reference = typename DereferencePolicy::reference;
      using type = std::conditional_t<std::is_lvalue_reference_v<reference>, std::remove_reference_t<reference> const&, reference>;
    };

    template<dereference_policy DereferencePolicy>
      requires has_proxy_type<DereferencePolicy>
    struct type_generator<DereferencePolicy>
    {
      using type = typename DereferencePolicy::proxy;
    };


    template<class DereferencePolicy>
    using type_generator_t = typename type_generator<DereferencePolicy>::type;

    template<class DereferencePolicy>
    inline constexpr bool provides_mutable_reference_v{
           !has_proxy_type<DereferencePolicy>
        &&  std::is_reference_v<typename DereferencePolicy::reference>
        && !is_const_reference_v<typename DereferencePolicy::reference>
    };
  }
}
