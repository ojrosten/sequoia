////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file NodeStorage.hpp
    \brief Classes to allow homogeneous treatment of graphs with empty/non-empty node weights.

    These classes are designed to be inherited from publically, in order that graphs may aggregate
    the interface of the node storage. For empty node weights, the node storage specialization
    is itself empty, meaning that empty base class optimization will ensure that no space is
    wasted.

 */

#include "ArrayUtilities.hpp"
#include "Iterator.hpp"
#include "StaticData.hpp"
#include "Algorithms.hpp"

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
        using value_type = std::remove_cv_t<reference>;

        constexpr proxy_dereference_policy() = default;
        constexpr proxy_dereference_policy(const proxy_dereference_policy&) = default;

        static constexpr reference get(proxy_reference ref) noexcept { return ref.get(); }

        [[nodiscard]]
        static constexpr pointer get_ptr(proxy_reference ref) noexcept { return &ref; }
      protected:
        constexpr proxy_dereference_policy(proxy_dereference_policy&&) noexcept = default;

        ~proxy_dereference_policy() = default;

        constexpr proxy_dereference_policy& operator=(const proxy_dereference_policy&)     = default;
        constexpr proxy_dereference_policy& operator=(proxy_dereference_policy&&) noexcept = default;
      };
                                  
      template<class WeightProxy, class Traits, bool=std::is_empty_v<typename WeightProxy::value_type>>
      class node_storage
      {
      private:
        template<class S> using Container = typename Traits::template underlying_storage_type<S>;
        using Storage = typename data_structures::impl::storage_helper<Container<WeightProxy>>::storage_type;
      public:        
        using weight_type       = typename WeightProxy::value_type;
        using weight_proxy_type = WeightProxy;
        using size_type         = typename Storage::size_type;

        using const_iterator = utilities::iterator<typename Storage::const_iterator, proxy_dereference_policy<typename Storage::const_iterator>>;
        using const_reverse_iterator = utilities::iterator<typename Storage::const_reverse_iterator, proxy_dereference_policy<typename Storage::const_reverse_iterator>>;

        constexpr static bool throw_on_range_error{Traits::throw_on_range_error};

        constexpr node_storage() {}

        constexpr node_storage(const size_type n) : node_storage(StaticType{}, n) {}

        constexpr node_storage(std::initializer_list<weight_type> weights) : node_storage{StaticType{}, weights} {}

        [[nodiscard]]
        constexpr auto size() const { return m_NodeWeights.size(); }

        [[nodiscard]]
        constexpr const_iterator cbegin_node_weights() const noexcept { return const_iterator{m_NodeWeights.cbegin()}; }

        [[nodiscard]]
        constexpr const_reverse_iterator crbegin_node_weights() const noexcept { return const_reverse_iterator{m_NodeWeights.crbegin()}; }

        [[nodiscard]]
        constexpr const_iterator cend_node_weights() const noexcept { return const_iterator{m_NodeWeights.cend()}; }

        [[nodiscard]]
        constexpr const_reverse_iterator crend_node_weights() const noexcept { return const_reverse_iterator{m_NodeWeights.crend()}; }

        template<class Arg, class... Args>
        constexpr void node_weight(const_iterator pos, Arg&& arg, Args&&... args)
        {
          if constexpr (throw_on_range_error) if(pos == cend_node_weights()) throw std::out_of_range("node_storage::node_weight - index out of range!\n");

          const auto index{distance(cbegin_node_weights(), pos)};
          m_NodeWeights[index].set(std::forward<Arg>(arg), std::forward<Args>(args)...);
        }

        template<class Fn>
        constexpr void mutate_node_weight(const_iterator pos, Fn fn)
        {
          if constexpr (throw_on_range_error) if(pos == cend_node_weights()) throw std::out_of_range("node_storage::node_weight - index out of range!\n");

          const auto index{distance(cbegin_node_weights(), pos)};
          m_NodeWeights[index].mutate(fn);
        }

        [[nodiscard]]
        friend constexpr bool operator==(const node_storage& lhs, const node_storage& rhs) noexcept
        {
          return lhs.m_NodeWeights == rhs.m_NodeWeights;
        }

        [[nodiscard]]
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

        constexpr void swap_nodes(const size_type i, const size_type j)
        {
          if((i < size()) && (j < size()))
          {
            sequoia::swap(m_NodeWeights[i], m_NodeWeights[j]);
          }
          else if constexpr (throw_on_range_error)
          {
            throw std::out_of_range("node_storage::swap - index out of range");
          }
        }
        
        void reserve(const size_type newCapacity)
        {
          m_NodeWeights.reserve(newCapacity);
        }

        size_type capacity() const noexcept
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
          if constexpr (throw_on_range_error)
          {
            if(pos == cend_node_weights()) throw std::out_of_range("Attempting to erase a node which does not exist");
          }

          return const_iterator{m_NodeWeights.erase(pos.base_iterator())};
        }

        const_iterator erase_nodes(const_iterator first, const_iterator last)
        {
          if constexpr (throw_on_range_error)
          {
            if(first > last) throw std::out_of_range("Attempting to erase a range of nodes with first > last");
          }

          return const_iterator{m_NodeWeights.erase(first.base_iterator(), last.base_iterator())};
        }
        
        void clear() noexcept
        {
          m_NodeWeights.clear();
        }
      
      private:
        using StaticType = typename data_structures::impl::is_static_data<Container<WeightProxy>>::type;
         
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
          : m_NodeWeights{make_array(weights)}
        {
        }

        constexpr Storage make_array(std::initializer_list<weight_type> weights)
        {
          if(weights.size() != Container<WeightProxy>::num_elements())
            throw std::logic_error("Initializer list of wrong size");
          
          constexpr auto N{Container<WeightProxy>::num_elements()};
          return utilities::to_array<WeightProxy, N>(weights, [](const auto& weight) {
              return WeightProxy{weight}; });
        }

        template<std::size_t... Inds>
        constexpr Storage make_default_array(std::index_sequence<Inds...>)
        {
          return { make_default_element(Inds)... };
        }        
        
        constexpr WeightProxy make_default_element(const std::size_t i)
        {
          return WeightProxy{weight_type{}};
        }
      };

      template<class WeightProxy, class Traits>
      class node_storage<WeightProxy, Traits, true>
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
        
        ~node_storage() = default;
      };     
    }
  }
}
