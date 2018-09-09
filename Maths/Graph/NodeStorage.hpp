#pragma once

#include "Iterator.hpp"
#include "StaticData.hpp"


#include <type_traits>

namespace sequoia
{
  namespace maths
  {
    namespace graph_impl
    {
      //===================================Storage for the list of nodes===================================//

      template<class Iterator>
      struct proxy_dereference_policy
      {        
        using proxy_reference = typename std::iterator_traits<Iterator>::reference;
        using proxy_pointer = typename std::iterator_traits<Iterator>::pointer;

        using reference = decltype(std::declval<proxy_reference>().get());
        using pointer = std::add_pointer_t<reference>;

        static constexpr reference get(proxy_reference ref) noexcept { return ref.get(); }
      };
                                  
      template<class WeightProxy, template<class...> class Container, bool ThrowOnRangeError, bool=std::is_empty_v<typename WeightProxy::value_type>>
      class node_storage
      {
      private:
        using Storage = typename data_structures::storage_helper<Container<WeightProxy>>::storage_type;
      public:        
        using weight_type = typename WeightProxy::value_type;
        using weight_proxy_type = WeightProxy;
        using size_type = typename Storage::size_type;

        using const_iterator = utilities::iterator<typename Storage::const_iterator, proxy_dereference_policy<typename Storage::const_iterator>, utilities::null_data_policy>;
        using const_reverse_iterator = utilities::iterator<typename Storage::const_reverse_iterator, proxy_dereference_policy<typename Storage::const_reverse_iterator>, utilities::null_data_policy>;

        constexpr node_storage() {}

        constexpr node_storage(const size_type n) : node_storage(StaticType{}, n) {}

        constexpr node_storage(std::initializer_list<weight_type> weights) : node_storage{StaticType{}, weights} {}
        
        constexpr auto size() const { return m_NodeWeights.size(); }

        constexpr const_iterator cbegin_node_weights() const noexcept { return const_iterator{m_NodeWeights.cbegin()}; }
        constexpr const_reverse_iterator crbegin_node_weights() const noexcept { return const_reverse_iterator{m_NodeWeights.crbegin()}; }

        constexpr const_iterator cend_node_weights() const noexcept { return const_iterator{m_NodeWeights.cend()}; }
        constexpr const_reverse_iterator crend_node_weights() const noexcept { return const_reverse_iterator{m_NodeWeights.crend()}; }

        template<class Arg, class... Args>
        constexpr void node_weight(const_iterator pos, Arg&& arg, Args&&... args)
        {
          if constexpr (ThrowOnRangeError) if(pos == cend_node_weights()) throw std::out_of_range("node_storage::node_weight - index out of range!\n");

          const auto index{distance(cbegin_node_weights(), pos)};
          m_NodeWeights[index].set(std::forward<Arg>(arg), std::forward<Args>(args)...);
        }

        template<class Fn>
        constexpr void mutate_node_weight(const_iterator pos, Fn fn)
        {
          if constexpr (ThrowOnRangeError) if(pos == cend_node_weights()) throw std::out_of_range("node_storage::node_weight - index out of range!\n");

          const auto index{distance(cbegin_node_weights(), pos)};
          m_NodeWeights[index].mutate(fn);
        }

        friend constexpr bool operator==(const node_storage& lhs, const node_storage& rhs) noexcept
        {
          return lhs.m_NodeWeights == rhs.m_NodeWeights;
        }

        friend constexpr bool operator!=(const node_storage& lhs, const node_storage& rhs) noexcept
        {
          return !(lhs == rhs);
        }
      protected:
        constexpr node_storage(std::true_type, const size_type n)
          : m_NodeWeights{make_default_array(std::make_index_sequence<Container<WeightProxy>::num_elements()>{})} {}
        
        constexpr node_storage(std::false_type, const size_type n) : m_NodeWeights(n) {}
        
        constexpr node_storage(const node_storage& in) = default;
      
        constexpr node_storage(node_storage&&) noexcept = default;
        
        ~node_storage() = default;

        constexpr node_storage& operator=(const node_storage& in) = default;
 
        constexpr node_storage& operator=(node_storage&&) noexcept = default;
      
        void reserve_nodes(const size_type newCapacity)
        {
          m_NodeWeights.reserve(newCapacity);
        }

        void node_capacity() const noexcept
        {
          return m_NodeWeights.capacity();
        }

        void shrink_to_fit()
        {
          m_NodeWeights.shrink_to_fit();
        }
        
        template<class... Args>
        void add_node(Args&&... args)
        {
          m_NodeWeights.emplace_back(std::forward<Args>(args)...);
        }

        template<class... Args>
        const_iterator insert_node(const_iterator pos, Args&&... args)
        {
          auto iter = m_NodeWeights.emplace(pos.base_iterator(), std::forward<Args>(args)...);

          return cbegin_node_weights() + std::distance(m_NodeWeights.begin(), iter);
        }

        const_iterator erase_node(const_iterator pos)
        {
          if constexpr (ThrowOnRangeError)
          {
            if(pos == cend_node_weights()) throw std::out_of_range("Attempting to delete a node which does not exist");
          }

          return const_iterator{m_NodeWeights.erase(pos.base_iterator())};
        }

        const_iterator erase_nodes(const_iterator first, const_iterator last)
        {
          if constexpr (ThrowOnRangeError)
          {
            if(first > last) throw std::out_of_range("Attempting to delete a range of nodes with first > last");
          }

          return const_iterator{m_NodeWeights.erase(first.base_iterator(), last.base_iterator())};
        }
        
        void clear() noexcept
        {
          m_NodeWeights.clear();
        }
      
      private:
        using StaticType = typename std::is_base_of<data_structures::static_data_base, Container<WeightProxy>>::type;
        using DefaultConstructibleType = typename std::is_default_constructible<weight_type>::type;
      
        Storage m_NodeWeights;

        constexpr node_storage(std::false_type, std::initializer_list<weight_type> weights)
        {
          for(const auto& weight : weights)
          {
            m_NodeWeights.emplace_back(weight);
          }
        }

        constexpr node_storage(std::true_type, std::initializer_list<weight_type> weights)
          : m_NodeWeights{make_array(std::make_index_sequence<Container<WeightProxy>::num_elements()>{}, weights)}
        {
          if(weights.size() != Container<WeightProxy>::num_elements())
            throw std::logic_error("Initializer list of wrong size");
        }

        constexpr WeightProxy make_element(const std::size_t i, std::initializer_list<weight_type> weights)
        {
          return WeightProxy{*(weights.begin() + i)};
        }

        template<std::size_t... Inds>
        constexpr Storage make_array(std::integer_sequence<std::size_t, Inds...>, std::initializer_list<weight_type> weights)
        {
          return { make_element(Inds, weights)... };
        }

        template<std::size_t... Inds>
        constexpr Storage make_default_array(std::integer_sequence<std::size_t, Inds...>)
        {
          return { make_default_element(Inds)... };
        }        
        
        constexpr WeightProxy make_default_element(const std::size_t i)
        {
          return WeightProxy{weight_type{}};
        }
      };

      template<class WeightProxy, template<class...> class Container, bool ThrowOnRangeError>
      class node_storage<WeightProxy, Container, ThrowOnRangeError, true>
      {
      public:
        using weight_proxy_type = WeightProxy;
        using weight_type = typename WeightProxy::value_type;
        
        constexpr node_storage() = default;
        constexpr node_storage(const std::size_t) {}

        constexpr friend bool operator==(const node_storage& lhs, const node_storage& rhs) noexcept { return true;}
        constexpr friend bool operator!=(const node_storage& lhs, const node_storage& rhs) noexcept { return !(lhs == rhs);}
      protected:
        constexpr node_storage(const node_storage&)                = default;
        constexpr node_storage(node_storage&&) noexcept            = default;
        constexpr node_storage& operator=(const node_storage&)     = default;
        constexpr node_storage& operator=(node_storage&&) noexcept = default;

        constexpr void swap_nodes(node_storage&) noexcept {}
        
        ~node_storage() = default;
      };

      template<class WeightProxy, std::size_t N, bool ThrowOnRangeError, bool=std::is_empty_v<typename WeightProxy::value_type>>
      class static_node_storage : public node_storage<WeightProxy, data_structures::static_contiguous_data<1,N>::template data, ThrowOnRangeError>
      {
        using node_storage<WeightProxy, data_structures::static_contiguous_data<1,N>::template data, ThrowOnRangeError>::node_storage;
      };

      template<class WeightProxy, std::size_t N, bool ThrowOnRangeError>
      class static_node_storage<WeightProxy, N, ThrowOnRangeError, true>
      {
      public:
        using weight_proxy_type = WeightProxy;
        using weight_type = typename WeightProxy::value_type;
        
        constexpr static_node_storage() = default;
        constexpr static_node_storage(const std::size_t) {}

        constexpr friend bool operator==(const static_node_storage& lhs, const static_node_storage& rhs) noexcept { return true;}
        constexpr friend bool operator!=(const static_node_storage& lhs, const static_node_storage& rhs) noexcept { return !(lhs == rhs);}

        constexpr static std::size_t size() noexcept { return N; }
      protected:
        constexpr static_node_storage(const static_node_storage&)                = default;
        constexpr static_node_storage(static_node_storage&&) noexcept            = default;
        constexpr static_node_storage& operator=(const static_node_storage&)     = default;
        constexpr static_node_storage& operator=(static_node_storage&&) noexcept = default;

        void swap_nodes(static_node_storage&) noexcept {}
        
        ~static_node_storage() = default;
      };
    }
  }
}
