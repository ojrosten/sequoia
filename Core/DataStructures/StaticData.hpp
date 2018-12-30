////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticData.hpp
    \brief Utilities to allow for both std::vector and std::array to be smoothly
    utilized as template template classes especially in contiguous_partitioned_data.
 */

#include <vector>
#include <array>
#include <limits>

namespace sequoia::data_structures::impl
{

  template<std::size_t Npartitions, std::size_t Nelements, class IndexType=std::size_t> struct static_contiguous_data
  {
    static_assert(std::numeric_limits<IndexType>::max() >= Npartitions);
      
    template<class T> struct data
    {
      [[nodiscard]] static constexpr std::size_t num_elements() noexcept { return Nelements; }
      [[nodiscard]] static constexpr std::size_t num_partitions() noexcept { return Npartitions; }
      
      using storage_type = std::array<T, Nelements>;
      using auxiliary_storage = std::array<IndexType, Npartitions>;
      using value_type = T;
      using iterator               = typename storage_type::iterator;
      using const_iterator         = typename storage_type::const_iterator;
      using reverse_iterator       = typename storage_type::reverse_iterator;
      using const_reverse_iterator = typename storage_type::const_reverse_iterator;
    };
  };

  template<class T, class = std::void_t<>>
  struct is_static_data : std::false_type
  {};

  template<class T>
  struct is_static_data<T, std::void_t<decltype(T::num_elements())>> : std::true_type
  {};

  template<class T> constexpr bool is_static_data_v{is_static_data<T>::value};

  template<class C, bool=is_static_data_v<C>> struct storage_helper
  {
    using storage_type = typename C::storage_type;
    using auxiliary_storage_type = typename C::auxiliary_storage;
  };

  template<class C> struct storage_helper<C, false>
  {    
    using storage_type = C;
    using auxiliary_storage_type = std::vector<typename storage_type::size_type>;
  };
}
