////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes to allow homogeneous treatment of graphs with empty/non-empty node weights.

    These classes are designed to be inherited from publically, in order that graphs may aggregate
    the interface of the node storage. For empty node weights, the node storage specialization
    is itself empty, meaning that empty base class optimization will ensure that no space is
    wasted.

 */

#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Core/ContainerUtilities/AssignmentUtilities.hpp"
#include "sequoia/Core/ContainerUtilities/Iterator.hpp"
#include "sequoia/Maths/Graph/EdgesAndNodesUtilities.hpp"

#include <type_traits>
#include <algorithm>

namespace sequoia::maths::graph_impl
{
  //===================================Storage for the list of nodes===================================//

  template<class T>
  inline constexpr bool has_get_allocator{
    requires(const T& t) { t.get_allocator(); }
  };

  template<class Weight, class Traits>
  class node_storage
  {
    friend struct sequoia::assignment_helper;

    template<class S> using Container = typename Traits::template container_type<S>;
  public:
    using weight_type                = Weight;
    using traits_type                = Traits;
    using node_weight_container_type = Container<Weight>;
    using size_type                  = typename node_weight_container_type::size_type;

    using iterator           = utilities::iterator<typename node_weight_container_type::iterator, utilities::identity_dereference_policy<typename node_weight_container_type::iterator>>;
    using reverse_iterator   = utilities::iterator<typename node_weight_container_type::reverse_iterator, utilities::identity_dereference_policy<typename node_weight_container_type::reverse_iterator>>;
    using node_weights_range = std::ranges::subrange<iterator>;

    using const_iterator           = utilities::iterator<typename node_weight_container_type::const_iterator, utilities::identity_dereference_policy<typename node_weight_container_type::const_iterator>>;
    using const_reverse_iterator   = utilities::iterator<typename node_weight_container_type::const_reverse_iterator, utilities::identity_dereference_policy<typename node_weight_container_type::const_reverse_iterator>>;
    using const_node_weights_range = std::ranges::subrange<const_iterator>;

    constexpr static bool throw_on_range_error{Traits::throw_on_range_error};

    constexpr node_storage() = default;

    constexpr node_storage(const size_type)
      requires (Traits::static_storage_v)
      : m_NodeWeights{}
    {}

    constexpr node_storage(const size_type n)
      requires (!Traits::static_storage_v)
      : m_NodeWeights(n)
    {}

    constexpr node_storage(std::initializer_list<weight_type> weights)
      requires (Traits::static_storage_v)
      : m_NodeWeights{utilities::to_array<weight_type, Traits::num_elements_v>(weights, std::identity{})}
    {}

    constexpr node_storage(std::initializer_list<weight_type> weights)
      requires (!Traits::static_storage_v)
      : m_NodeWeights{weights}
    {}

    [[nodiscard]]
    constexpr auto size() const noexcept { return m_NodeWeights.size(); }

    [[nodiscard]]
    constexpr iterator begin_node_weights() noexcept
      requires std::indirectly_writable<iterator, std::iter_value_t<iterator>>
    {
      return iterator{m_NodeWeights.begin()};
    }

    [[nodiscard]]
    constexpr reverse_iterator rbegin_node_weights() noexcept
      requires std::indirectly_writable<iterator, std::iter_value_t<iterator>>
    {
      return reverse_iterator{m_NodeWeights.rbegin()};
    }

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
    constexpr iterator end_node_weights() noexcept
      requires std::indirectly_writable<iterator, std::iter_value_t<iterator>>
    {
      return iterator{m_NodeWeights.end()};
    }

    [[nodiscard]]
    constexpr reverse_iterator rend_node_weights() noexcept
      requires std::indirectly_writable<iterator, std::iter_value_t<iterator>>
    {
      return reverse_iterator{m_NodeWeights.rend()};
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

    [[nodiscard]]
    constexpr node_weights_range node_weights() noexcept
      requires std::indirectly_writable<iterator, std::iter_value_t<iterator>>
    {
      return {begin_node_weights(), end_node_weights()};
    }

    [[nodiscard]]
    constexpr const_node_weights_range node_weights() const noexcept
    {
      return {cbegin_node_weights(), cend_node_weights()};
    }

    [[nodiscard]]
    constexpr const_node_weights_range cnode_weights() const noexcept
    {
      return node_weights();
    }

    template<class... Args>
    constexpr void node_weight(const_iterator pos, Args&&... args)
    {
      if constexpr (throw_on_range_error) if(pos == cend_node_weights()) throw std::out_of_range("node_storage::node_weight - index out of range!\n");

      const auto index{std::ranges::distance(cbegin_node_weights(), pos)};
      m_NodeWeights[index] = weight_type{std::forward<Args>(args)...};
    }

    template<class Fn>
    constexpr decltype(auto) mutate_node_weight(const_iterator pos, Fn&& fn)
    {
      if constexpr (throw_on_range_error) if(pos == cend_node_weights()) throw std::out_of_range("node_storage::node_weight - index out of range!\n");

      const auto index{std::ranges::distance(cbegin_node_weights(), pos)};
      return std::forward<Fn>(fn)(m_NodeWeights[index]);
    }

    template<alloc Allocator>
      requires (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
    void reset(const Allocator& allocator) noexcept
    {
      const node_weight_container_type storage(allocator);
      m_NodeWeights = storage;
    }

    [[nodiscard]]
    friend constexpr bool operator==(const node_storage&, const node_storage&) noexcept
      requires deep_equality_comparable<weight_type>
        = default;
  protected:
    template<alloc Allocator>
    constexpr explicit node_storage(const Allocator& allocator)
      : m_NodeWeights(allocator)
    {}

    template<alloc Allocator>
    constexpr node_storage(const size_t n, const Allocator& allocator)
      : m_NodeWeights(n, allocator)
    {}

    template<alloc Allocator>
    constexpr node_storage(std::initializer_list<weight_type> weights, const Allocator& allocator)
      : m_NodeWeights{weights, allocator}
    {}

    constexpr node_storage(const node_storage&) = default;

    template<alloc Allocator>
    constexpr node_storage(const node_storage& other, const Allocator& allocator)
      : m_NodeWeights{other.m_NodeWeights, allocator}
    {}

    constexpr node_storage(node_storage&&) noexcept = default;

    template<alloc Allocator>
    constexpr node_storage(node_storage&& s, const Allocator& allocator) noexcept
      : m_NodeWeights{std::move(s.m_NodeWeights), allocator}
    {}

    ~node_storage() = default;

    constexpr node_storage& operator=(const node_storage& other)
    {
      if(&other != this)
      {
        auto allocGetter{
          []([[maybe_unused]] const node_storage& other){
            if constexpr(has_allocator_type_v<node_weight_container_type>)
            {
              return other.m_NodeWeights.get_allocator();
            }
          }
        };
        assignment_helper::assign(*this, other, allocGetter);
      }

      return *this;
    }

    constexpr node_storage& operator=(node_storage&&) = default;

    constexpr void swap(node_storage& rhs)
      noexcept(noexcept(std::ranges::swap(this->m_NodeWeights, rhs.m_NodeWeights)))
    {
      std::ranges::swap(m_NodeWeights, rhs.m_NodeWeights);
    }

    constexpr void swap_nodes(const size_type i, const size_type j)
    {
      if((i < size()) && (j < size()))
      {
        std::ranges::swap(m_NodeWeights[i], m_NodeWeights[j]);
      }
      else if constexpr (throw_on_range_error)
      {
        throw std::out_of_range("node_storage::swap - index out of range");
      }
    }

    auto get_node_allocator() const
      requires has_get_allocator<node_weight_container_type>
    {
      return m_NodeWeights.get_allocator();
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

      return cbegin_node_weights() + std::ranges::distance(m_NodeWeights.begin(), iter);
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
    node_weight_container_type m_NodeWeights;
  };

  template<class Weight, class Traits>
    requires std::is_empty_v<Weight>
  class node_storage<Weight, Traits>
  {
  public:
    using weight_type = Weight;
    using size_type   = std::size_t;

    constexpr node_storage() noexcept = default;

    [[nodiscard]]
    constexpr friend bool operator==(const node_storage&, const node_storage&) noexcept = default;
  protected:
    constexpr node_storage(const node_storage&) noexcept = default;
    constexpr node_storage(node_storage&&)      noexcept = default;

    ~node_storage() = default;

    constexpr node_storage& operator=(const node_storage&) noexcept = default;
    constexpr node_storage& operator=(node_storage&&)      noexcept = default;

    void swap(node_storage&) noexcept {};
  };
}
