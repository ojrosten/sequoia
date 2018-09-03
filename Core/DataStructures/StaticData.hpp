#pragma once

#include <vector>
#include <array>
#include <limits>

namespace sequoia
{
  namespace data_structures
  {
    class static_data_base
    {
    public:
      constexpr static_data_base() {}
    protected:
      ~static_data_base() {}
    };

    template<std::size_t Npartitions, std::size_t Nelements, class IndexType=std::size_t> struct static_contiguous_data
    {
      static_assert(std::numeric_limits<IndexType>::max() >= Npartitions);
      
      template<class T> struct data : public static_data_base
      {
        static constexpr std::size_t num_elements() { return Nelements; }
        static constexpr std::size_t num_partitions() { return Npartitions; }
        using storage_type = std::array<T, Nelements>;
        using auxiliary_storage = std::array<IndexType, Npartitions>;
        using value_type = T;
        using iterator = typename storage_type::iterator;
        using const_iterator = typename storage_type::const_iterator;
        using reverse_iterator = typename storage_type::reverse_iterator;
        using const_reverse_iterator = typename storage_type::const_reverse_iterator;
      };
    };

    template<class C, bool b=std::is_base_of<static_data_base, C>::value> struct storage_helper
    {
      using storage_type = C;
      using auxiliary_storage_type = std::vector<std::size_t>;
    };

    template<class C> struct storage_helper<C, true>
    {
      using storage_type = typename C::storage_type;
      using auxiliary_storage_type = typename C::auxiliary_storage;
    };
  }
}
