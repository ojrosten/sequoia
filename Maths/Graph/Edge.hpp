////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Edge.hpp
    \brief Various edge types for use by graphs.

    The fundamental building block of the edge types is the edge_base. This
    can be thought of as a directed edge with no additional information; as
    such is 'points', via an index, to a target node.

    All concrete edges
    inherit from this class but may have additional adornments. Depending on
    the context, edges may have one or more of the following:

    1. A non-trivial weight;

    2. A record of the 'host' node, from which the edge points to its target;

    3. An index recording the location on the target node into which the edge is
    embedded.
*/

#include "TypeTraits.hpp"

namespace sequoia
{
  namespace data_sharing
  {
    template<class> struct independent;
  }
  
  namespace maths
  {
    /*! \class edge_base
        \brief The base class of all edges.
        
        This class is designed for inheritance and has protected destructor
        and assignment operators.

        The private data comprises a single member of IndexType which is the
        'target node' to which the edge points.
    */
    
    template<class IndexType>
    class edge_base
    {   
    public:
      using index_type = IndexType;
      
      explicit constexpr edge_base(const index_type target) noexcept : m_Target{target} {}      
      constexpr edge_base(const edge_base&) noexcept = default;
      constexpr edge_base(edge_base&&)      noexcept = default;

      [[nodiscard]]
      constexpr IndexType target_node() const noexcept { return m_Target; }

      constexpr void target_node(const index_type target) noexcept { m_Target = target; }      
    protected:      
      ~edge_base() = default;
      
      constexpr edge_base& operator=(const edge_base&) noexcept = default;
      constexpr edge_base& operator=(edge_base&&)      noexcept = default;   
    private:
      IndexType m_Target;
    };

    template<class IndexType>
    [[nodiscard]]
    constexpr bool operator==(const edge_base<IndexType>& lhs, const edge_base<IndexType>& rhs) noexcept
    {
      return (lhs.target_node() == rhs.target_node());
    }

    template<class IndexType>
    [[nodiscard]]
    constexpr bool operator!=(const edge_base<IndexType>& lhs, const edge_base<IndexType>& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    /*! \class weighting
        \brief A class to store non-trivial edge weights.
        
        This class is designed to wrap an edge but in a manner which is flexible
        enough to support the sharing of weights between edges. The latter is used
        for undirected graphs where two partial edges representing the pair of
        links between two vertices may wish to share the common weight, particularly
        if it is expensive to store twice. The sharing of weights between distinct
        edges is done at the level of the graph.
    */

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType,
      bool=std::is_empty_v<Weight>
    >
    class weighting
    {
    public:
      using weight_type = Weight;
      using weight_proxy_type = WeightProxy;
      
      template<class Arg, class... Args>
      constexpr void weight(Arg&& arg, Args&&... args)
      {
        wrapped_weight::get(m_Weight).set(std::forward<Arg>(arg), std::forward<Args>(args)...);
      }

      [[nodiscard]]
      constexpr const Weight& weight() const noexcept { return wrapped_weight::get(m_Weight).get(); }

      template<class Fn>
      constexpr void mutate_weight(Fn fn)
      {
        wrapped_weight::get(m_Weight).mutate(fn);
      }
    protected:
      ~weighting() = default;
      
      template<class... Args, class=std::enable_if_t<!utilities::same_decay_v<weighting, Args...> && !utilities::is_base_of_head_v<weighting, Args...>>>
      constexpr explicit weighting(Args&&... args) : m_Weight{wrapped_weight::make(std::forward<Args>(args)...)}
      {}

      constexpr weighting(const weighting& in) : m_Weight{wrapped_weight::make(wrapped_weight::get(in.m_Weight))}
      {
      }

      constexpr weighting(const weighting& in, const IndexType) : m_Weight{in.m_Weight}        
      {
      }

      constexpr weighting(weighting&&) noexcept = default;

      constexpr weighting& operator=(const weighting& in) 
      {
        if(&in != this) m_Weight = wrapped_weight::make(wrapped_weight::get(in.m_Weight));
        return *this;
      }

      constexpr weighting& operator=(weighting&&) noexcept = default;        
    private:
      using wrapped_weight = WeightSharingPolicy<WeightProxy>;
      typename wrapped_weight::handle_type m_Weight; 
    };

    /*! \class weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType, true>
        \brief An empty (base) class for edges without a weight.

     */

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType
    >
    class weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType, true>
    {
    public:
      using weight_type = Weight;
      using weight_proxy_type = WeightProxy;

      [[nodiscard]]
      friend constexpr bool operator==(const weighting&, const weighting&) noexcept { return true; }
      
      [[nodiscard]]
      friend constexpr bool operator!=(const weighting&, const weighting&) noexcept { return false; }
    protected:
      constexpr weighting() = default;      
      ~weighting() = default;

      constexpr weighting(const weighting&)                  noexcept = default;
      constexpr weighting(weighting&&)                       noexcept = default;
      constexpr weighting& operator=(const weighting&)       noexcept = default;
      constexpr weighting& operator=(weighting&&)            noexcept = default;      
      constexpr weighting(const weighting&, const IndexType) noexcept {}
      
    };

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType
    >
    [[nodiscard]]
    constexpr bool operator==(const weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>& lhs, const weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>& rhs) noexcept
    {
      return lhs.weight() == rhs.weight();
    }

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType
    >
    [[nodiscard]]
    constexpr bool operator!=(const weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>& lhs, const weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>& rhs) noexcept
    {
      return !(lhs == rhs);
    }
    
    //===================================Partial Edge Base===================================//

    /*! \class partial_edge_base
        \brief Combines the edge_base and weighting class

     */
    
    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType
    >
    class partial_edge_base : public edge_base<IndexType>, public weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>
    {
    public:
      using weight_type = Weight;
      using index_type = IndexType;
      using weight_proxy_type = WeightProxy;

      template
      <
        class... Args,
        class=typename std::enable_if_t<!utilities::same_decay_v<partial_edge_base, Args...> && !utilities::is_base_of_head_v<partial_edge_base, Args...>>
      >
      constexpr explicit partial_edge_base(const index_type target, Args&&... args)
        : edge_base<IndexType>{target}
        , weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>{std::forward<Args>(args)...}
      {}
      
      constexpr partial_edge_base(const index_type target, const partial_edge_base& in)
        : edge_base<IndexType>{target}
        , weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>{in, target}
      {
      }

      constexpr partial_edge_base(const partial_edge_base& in)     = default;
      constexpr partial_edge_base(partial_edge_base&& in) noexcept = default;
    protected:      
      ~partial_edge_base() = default;

      constexpr partial_edge_base& operator=(const partial_edge_base& in)  = default;
      constexpr partial_edge_base& operator=(partial_edge_base&&) noexcept = default;      
    };

    //===================================Comparison operators===================================/

    template <class Weight, template<class> class WeightSharingPolicy, class WeightProxy, class IndexType>
    [[nodiscard]]
    constexpr bool operator==(const partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>& lhs, const partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>& rhs) noexcept
    {
      return (static_cast<const edge_base<IndexType>&>(lhs) == static_cast<const edge_base<IndexType>&>(rhs))
        && (static_cast<const weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>&>(lhs) == static_cast<const weighting<Weight, WeightSharingPolicy, WeightProxy, IndexType>&>(rhs));
    }

    template <class Weight, template<class> class WeightSharingPolicy, class WeightProxy, class IndexType>
    [[nodiscard]]
    constexpr bool operator!=(const partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>& lhs, const partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    //===================================Helper Enum===================================//

    enum class edge_flavour{ partial, partial_embedded, full, full_embedded};
    
    //===================================Partial Edge===================================//

    /*! \class partial_edge
        \brief A concrete edge containing a target index and, optionally, a weight

     */

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType=std::size_t
    >
    class partial_edge : public partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::partial};
      using partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::partial_edge_base;
    };

    //===================================Decorated Partial Edge Base===================================//

    /*! \class decorated_edge_base
        \brief A new base class which aggregates a partial_edge_base and an auxiliary index.

     */
    
    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType=std::size_t
    >
    class decorated_edge_base : public partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>
    {
    public:
      using weight_type = typename partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::weight_type;
      using index_type = typename partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::index_type;

      template
      <
        class... Args,
        class=typename std::enable_if_t<!utilities::same_decay_v<decorated_edge_base, Args...> && !utilities::is_base_of_head_v<decorated_edge_base, Args...>>
      >
      constexpr decorated_edge_base(const index_type target, const index_type auxIndex, Args&&... args)
        : partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>{target, std::forward<Args>(args)...}
        , m_AuxiliaryIndex{auxIndex}
      {}

      constexpr decorated_edge_base(const index_type target, const index_type auxIndex, const decorated_edge_base& in)
        : partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>{target, in}
        , m_AuxiliaryIndex{auxIndex}
      {}

      constexpr decorated_edge_base(const decorated_edge_base& in)             = default;
      constexpr decorated_edge_base(decorated_edge_base&& in)                  = default;
      constexpr decorated_edge_base& operator=(const decorated_edge_base& in)  = default;
      constexpr decorated_edge_base& operator=(decorated_edge_base&&)          = default;

      [[nodiscard]]
      friend constexpr bool operator==(const decorated_edge_base& lhs, const decorated_edge_base& rhs) noexcept
      {
        return (static_cast<const partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>&>(lhs) == static_cast<const partial_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>&>(rhs))
        && (lhs.auxiliary_index() == rhs.auxiliary_index());
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const decorated_edge_base& lhs, const decorated_edge_base& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    protected:      
      ~decorated_edge_base() = default;

      [[nodiscard]]
      constexpr index_type auxiliary_index() const noexcept { return m_AuxiliaryIndex; }
      
      constexpr void auxiliary_index(const index_type auxIndex) noexcept { m_AuxiliaryIndex = auxIndex; }
    private:
      IndexType m_AuxiliaryIndex;
    };
    
    //===================================Embedded Partial Edge===================================//

    /*! \class embedded_partial_edge
        \brief Decoration of a partial_edge to record the location on the target node into
               which the edge is embedded.

     */
    
    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType=std::size_t
    >
    class embedded_partial_edge : public decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::partial_embedded};
      using weight_type = typename decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::weight_type;
      using index_type = typename decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::index_type;

      using decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::decorated_edge_base;

      [[nodiscard]]
      constexpr index_type complementary_index() const noexcept { return this->auxiliary_index(); }
      constexpr void complementary_index(const index_type index) noexcept { return this->auxiliary_index(index); }
    };

    //===================================Full Edge===================================//
    
    template<bool Inverted> struct inversion_constant : public std::bool_constant<Inverted> {};

    template<bool Inverted> constexpr bool inversion_v{inversion_constant<Inverted>::value};

    /*! \class edge
        \brief Stores the host node as well as the target node and an optional weight.
      
     */
    
    template
    <
      class Weight,
      class WeightProxy,
      class IndexType=std::size_t
    >
    class edge : public decorated_edge_base<Weight, data_sharing::independent, WeightProxy, IndexType>
    {
    public:      
      constexpr static edge_flavour flavour{edge_flavour::full};
      using weight_type = typename decorated_edge_base<Weight, data_sharing::independent, WeightProxy, IndexType>::weight_type;
      using index_type = typename decorated_edge_base<Weight, data_sharing::independent, WeightProxy, IndexType>::index_type;
      using weight_proxy_type = WeightProxy;

      template
      <
        class... Args,
        class=typename std::enable_if_t<!utilities::same_decay_v<edge, Args...> && !utilities::is_base_of_head_v<edge, Args...>>
      >
      constexpr edge(const index_type host, const index_type target, Args&&... args)
        : decorated_edge_base<Weight, data_sharing::independent, WeightProxy, IndexType>{target, host, std::forward<Args>(args)...}
      {
        if(host == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      template
      <
        bool Inverted,
        class... Args
      >
      constexpr edge(const index_type node, const inversion_constant<Inverted> inverted, Args&&... args)
        : decorated_edge_base<Weight, data_sharing::independent, WeightProxy, IndexType>{
            node, inverted.value ? npos : node, std::forward<Args>(args)...
          }
      {
        if(node == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      constexpr edge(const edge& in)             = default;
      constexpr edge(edge&& in)                  = default;
      constexpr edge& operator=(const edge& in)  = default;
      constexpr edge& operator=(edge&&)          = default;

      [[nodiscard]]
      constexpr index_type host_node() const noexcept
      {
        return this->auxiliary_index() < npos ? this->auxiliary_index() : this->target_node();
      }
      
      constexpr void host_node(const index_type host) noexcept
      {
        if(!inverted()) this->auxiliary_index(host);
        else            this->target_node(host);
      }

      [[nodiscard]]
      constexpr bool inverted() const noexcept { return this->auxiliary_index() == npos; }
    private:
      static constexpr auto npos = std::numeric_limits<IndexType>::max();
    };

    //===================================Full Embedded Edge===================================//

    /*! \class embedded_edge
        \brief Decoration of edge to record the location on the target node into
               which the edge is embedded.

     */
    
    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType=std::size_t
    >
    class embedded_edge : public decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::full_embedded};
      using weight_type = typename decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::weight_type;
      using index_type = typename decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>::index_type;

      template
      <
        class... Args,
        class=typename std::enable_if_t<!utilities::same_decay_v<embedded_edge, Args...> && !utilities::is_base_of_head_v<embedded_edge, Args...>>
      >
      constexpr embedded_edge(const index_type host, const index_type target, const index_type auxIndex, Args&&... args)
        : decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>{target, auxIndex, std::forward<Args>(args)...}
        , m_HostIndex{host}
      {
        if(host == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      template
      <
        bool Inverted,
        class... Args
      >
      constexpr embedded_edge(const index_type node, const inversion_constant<Inverted> inverted, const index_type auxIndex, Args&&... args)
        : decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>{node, auxIndex, std::forward<Args>(args)...}
        , m_HostIndex{inverted.value ? npos : node}
      {
        if(node == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      constexpr embedded_edge(const index_type auxIndex, const embedded_edge& in)
        : decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>{in.target_node(), auxIndex, in}
        , m_HostIndex{in.m_HostIndex}
      {
      }
      
      constexpr embedded_edge(const embedded_edge& in)             = default;
      constexpr embedded_edge(embedded_edge&& in)                  = default;
      constexpr embedded_edge& operator=(const embedded_edge& in)  = default;
      constexpr embedded_edge& operator=(embedded_edge&&)          = default;

      [[nodiscard]]
      constexpr index_type complementary_index() const noexcept { return this->auxiliary_index(); }
      constexpr void complementary_index(const index_type auxIndex) noexcept { return this->auxiliary_index(auxIndex); }

      [[nodiscard]]
      constexpr index_type host_node() const noexcept { return !inverted() ? m_HostIndex : this->target_node(); }
      
      constexpr void host_node(const index_type host) noexcept
      {
        if(!inverted()) m_HostIndex = host;
        else            this->target_node(host);
      }

      [[nodiscard]]
      constexpr bool inverted() const noexcept { return m_HostIndex == npos; }
    private:
      static constexpr auto npos = std::numeric_limits<IndexType>::max();
      
      IndexType m_HostIndex;
    };

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType=std::size_t
    >
    [[nodiscard]]
    constexpr bool operator==(const embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& lhs, const embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& rhs)
    {
      return (lhs.host_node() == rhs.host_node())
        && (static_cast<const decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>&>(lhs) ==
            static_cast<const decorated_edge_base<Weight, WeightSharingPolicy, WeightProxy, IndexType>&>(rhs));
    }

    template
    <
      class Weight,
      template <class> class WeightSharingPolicy,
      class WeightProxy,
      class IndexType=std::size_t
    >
    [[nodiscard]]
    constexpr bool operator!=(const embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& lhs, const embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& rhs)
    {
      return !(lhs == rhs);
    }
  }
}
