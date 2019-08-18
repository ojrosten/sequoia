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
#include "AssignmentUtilities.hpp"
#include "Iterator.hpp"
#include "ProtectiveWrapper.hpp"
#include "Algorithms.hpp"

#include <type_traits>

namespace sequoia::maths::graph_impl
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
                                  
  template<class WeightMaker, class Traits, bool=std::is_empty_v<typename WeightMaker::weight_proxy::value_type>>
  class node_storage : private WeightMaker
  {
    friend struct sequoia::impl::assignment_helper;
    
  private:
    template<class S> using Container = typename Traits::template container_type<S>;    
  public:
    using node_weight_container_type = Container<typename WeightMaker::weight_proxy>;
    using weight_proxy_type          = typename WeightMaker::weight_proxy;
    using weight_type                = typename weight_proxy_type::value_type;
    using size_type                  = typename node_weight_container_type::size_type;

    using const_iterator         = utilities::iterator<typename node_weight_container_type::const_iterator, proxy_dereference_policy<typename node_weight_container_type::const_iterator>>;
    using const_reverse_iterator = utilities::iterator<typename node_weight_container_type::const_reverse_iterator, proxy_dereference_policy<typename node_weight_container_type::const_reverse_iterator>>;

    constexpr static bool throw_on_range_error{Traits::throw_on_range_error};

    constexpr node_storage() = default;

    constexpr explicit node_storage(const size_type n)
      : node_storage(StaticType{}, n)
    {}

    constexpr node_storage(std::initializer_list<weight_type> weights)
      : node_storage{StaticType{}, weights}
    {}

    [[nodiscard]]
    constexpr auto size() const noexcept { return m_NodeWeights.size(); }

    [[nodiscard]]
    constexpr const_iterator cbegin_node_weights() const noexcept
    {
      return const_iterator{m_NodeWeights.cbegin()};
    }

    [[nodiscard]]
    constexpr const_reverse_iterator crbegin_node_weights() const noexcept
    {
      return const_reverse_iterator{m_NodeWeights.crbegin()};
    }

    [[nodiscard]]
    constexpr const_iterator cend_node_weights() const noexcept
    {
      return const_iterator{m_NodeWeights.cend()};
    }

    [[nodiscard]]
    constexpr const_reverse_iterator crend_node_weights() const noexcept
    {
      return const_reverse_iterator{m_NodeWeights.crend()};
    }

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
    template<class Allocator>
    constexpr explicit node_storage(const Allocator& allocator)
      : m_NodeWeights(allocator)
    {}

    template<class Allocator>
    constexpr node_storage(const size_t n, const Allocator& allocator)
      : m_NodeWeights(init(n), allocator)
    {}

    template<class Allocator>
    constexpr node_storage(std::initializer_list<weight_type> weights, const Allocator& allocator)
      : m_NodeWeights(init(weights), allocator)
    {}
    
    constexpr node_storage(const node_storage& in)
      : node_storage{direct_copy(), in}
    {}

    template<class Allocator>
    constexpr node_storage(const node_storage& in, const Allocator& allocator)
      : node_storage{direct_copy(), in, allocator}
    {}
      
    constexpr node_storage(node_storage&&) noexcept = default;

    template<class Allocator>
    constexpr node_storage(node_storage&& s, const Allocator& allocator) noexcept
      : m_NodeWeights{std::move(s.m_NodeWeights), allocator}
    {}
        
    ~node_storage() = default;

    constexpr node_storage& operator=(const node_storage& in)
    {
      if(&in != this)
      {
        auto allocGetter{
          [](const node_storage& in){
            if constexpr(has_allocator_type_v<node_weight_container_type>)
            {
              return in.m_NodeWeights.get_allocator();
            }
          }
        };
        sequoia::impl::assignment_helper::assign(*this, in, allocGetter);
      }
 
      return *this;
    }    

    constexpr node_storage& operator=(node_storage&&) = default;

    constexpr void swap(node_storage& other) noexcept(noexcept(sequoia::swap(m_NodeWeights, other.m_NodeWeights)))
    {
      sequoia::swap(m_NodeWeights, other.m_NodeWeights);
    }

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
      m_NodeWeights.emplace_back(make_node_weight(std::forward<Args>(args)...));
    }

    template<class... Args>
    const_iterator insert_node(const_iterator pos, Args&&... args)
    {
      auto iter = m_NodeWeights.emplace(pos.base_iterator(), make_node_weight(std::forward<Args>(args)...));

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
    using StaticType = std::bool_constant<Traits::static_storage_v>;

    // copy meta
    template<bool Direct>
    struct copy_constant : std::bool_constant<Direct>
    {
    };
      
    using direct_copy_type   = copy_constant<true>;
    using indirect_copy_type = copy_constant<false>;
      
    [[nodiscard]]
    static constexpr auto direct_copy() noexcept
    {
      constexpr bool protective{
        std::is_same_v<weight_proxy_type, utilities::protective_wrapper<weight_type>>
      };
      return copy_constant<protective>{};
    }

    // private data
    node_weight_container_type m_NodeWeights;

    // constructors impl
    constexpr node_storage(std::true_type, const size_type n)
      : m_NodeWeights{make_default_array(std::make_index_sequence<Traits::num_elements_v>{})} {}

    constexpr node_storage(std::false_type, const size_type n)
      : m_NodeWeights{init(n)}
    {}

    constexpr node_storage(std::true_type, std::initializer_list<weight_type> weights)
      : m_NodeWeights{make_array(weights)}
    {
    }

    constexpr node_storage(std::false_type, std::initializer_list<weight_type> weights)
      : m_NodeWeights{init(weights)}
    {}
  
    constexpr node_storage(direct_copy_type, const node_storage& in)
      : m_NodeWeights{in.m_NodeWeights}
    {}

    template<class Allocator>
    constexpr node_storage(direct_copy_type, const node_storage& in, const Allocator& allocator)
      : m_NodeWeights{in.m_NodeWeights, allocator}
    {}

    constexpr node_storage(indirect_copy_type, const node_storage& in)
      : m_NodeWeights{clone(in.m_NodeWeights, in.m_NodeWeights.get_allocator())}
    {}

    template<class Allocator>
    constexpr node_storage(indirect_copy_type, const node_storage& in, const Allocator& allocator)
    : m_NodeWeights{clone(in.m_NodeWeights, allocator)}
    {}
  
    // helper methods

    [[nodiscard]]
    node_weight_container_type init(const size_type n)
    {
      node_weight_container_type nodeWeights{};
      nodeWeights.reserve(n);
      for(size_type i{}; i<n; ++i)
      {
        nodeWeights.push_back(make_node_weight());
      }

      return nodeWeights;
    }

    [[nodiscard]]
    node_weight_container_type init(std::initializer_list<weight_type> weights)
    {
      node_weight_container_type nodeWeights{};
      nodeWeights.reserve(weights.size());
      for(const auto& weight : weights)
      {
        nodeWeights.push_back(make_node_weight(weight));
      }

      return nodeWeights;
    }

    template<class Allocator>
    [[nodiscard]]
    node_weight_container_type clone(const node_weight_container_type& from, const Allocator& alloc)
    {
      node_weight_container_type nodeWeights(alloc);
      nodeWeights.reserve(from.size());
      for(const auto& weight : from)
      {
        nodeWeights.emplace_back(make_node_weight(weight.get()));
      }

      return nodeWeights;
    }

    [[nodiscard]]
    constexpr node_weight_container_type make_array(std::initializer_list<weight_type> weights)
    {
      if(weights.size() != Traits::num_elements_v)
        throw std::logic_error("Initializer list of wrong size");
          
      constexpr auto N{Traits::num_elements_v};
      return utilities::to_array<weight_proxy_type, N>(weights, [](const auto& weight) {
          return weight_proxy_type{weight}; });
    }

    template<std::size_t... Inds>
    [[nodiscard]]
    constexpr node_weight_container_type make_default_array(std::index_sequence<Inds...>)
    {
      return { make_default_element(Inds)... };
    }        

    [[nodiscard]]
    constexpr weight_proxy_type make_default_element(const std::size_t i)
    {
      return weight_proxy_type{weight_type{}};
    }

    template<class... Args>
    [[nodiscard]]
    decltype(auto) make_node_weight(Args&&... args)
    {
      return WeightMaker::make(std::forward<Args>(args)...);
    }
  };
  
  template<class WeightMaker, class Traits>
  class node_storage<WeightMaker, Traits, true>
  {
  public:
    using weight_proxy_type = typename WeightMaker::weight_proxy;
    using weight_type       = typename weight_proxy_type::value_type;
    using size_type         = typename std::size_t;
    
    constexpr node_storage() noexcept = default;

    [[nodiscard]]
    constexpr friend bool operator==(const node_storage& lhs, const node_storage& rhs) noexcept { return true;}

    [[nodiscard]]
    constexpr friend bool operator!=(const node_storage& lhs, const node_storage& rhs) noexcept { return !(lhs == rhs);}
  protected:
    constexpr node_storage(const node_storage&) noexcept = default;
    constexpr node_storage(node_storage&&)      noexcept = default;
        
    ~node_storage() = default;

    constexpr node_storage& operator=(const node_storage&) noexcept = default;
    constexpr node_storage& operator=(node_storage&&)      noexcept = default;
  };     
}
