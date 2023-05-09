////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes to allow homogenous treatment of pooled/independently spawned data.

    The idea is, in each case, to supply a `make` method which returns a proxy; the
    latter provides a uniform interface.
 */

#include "sequoia/Core/Object/Creator.hpp"
#include "sequoia/Core/Object/FaithfulWrapper.hpp"
#include "sequoia/Core/ContainerUtilities/Iterator.hpp"

#include <vector>
#include <memory>
#include <algorithm>

namespace sequoia::object
{
  template<class T>
  using faithful_producer = producer<T, faithful_wrapper<T>>;

  namespace impl
  {
    template<class Wrapper>
    struct pool_deref_policy
    {
      using value_type = typename Wrapper::value_type;
      using proxy      = std::pair<const value_type&, long>;
      using reference  = proxy;

    protected:
      using product_type = std::shared_ptr<Wrapper>;

      pool_deref_policy() = default;
      pool_deref_policy(const pool_deref_policy&)     = default;
      pool_deref_policy(pool_deref_policy&&) noexcept = default;
      pool_deref_policy& operator=(const pool_deref_policy&)     = default;
      pool_deref_policy& operator=(pool_deref_policy&&) noexcept = default;

      [[nodiscard]]
      static proxy get(const product_type& ptr)
      {
        return {ptr->get(), ptr.use_count() - 1};
      }
    };

    template<std::input_or_output_iterator Iterator, class Wrapper>
    using pool_iterator = utilities::iterator<Iterator, pool_deref_policy<Wrapper>>;
  }

  /*! \brief A data pool, allowing what would otherwise be identical instances of a type to reference a single instance.

      The pool is illustrated by the following diagram. The bottom layer is the internal storage:
      a container holding instances of std::shared_ptr, each pointing to an intermediate wrapper,
      `W`, which holds an instance of the pool's value type, `T`.
      `W` is also pointed to by the client-level proxies, P, of which there may be an arbitrary
      number. If a mutating call is made to the proxy then its pointer back to `W` is redirected.
      If a different `W` exists that happens to already hold an instance of `T` equal to the mutated
      value, then this is where `P` now points. Otherwise, a new `W` is created.

      <pre>
       |P| |P|
        |  /
        | /
        |/
       |W|
        |
        |
       | |....
      </pre>

      To remove intermediate wrappers, `W`, with no associated proxies, invoke sequoia::object::data_pool::tidy.
   */
  template<class T, class Allocator=std::allocator<T>>
    requires (!std::is_empty_v<T>)
  class data_pool
  {
    friend class data_wrapper;
  public:

    class data_wrapper
    {
      friend class data_pool;

    public:
      using value_type = T;

      template<class... Args>
      data_wrapper(data_pool& pool, Args&&... args) : m_pPool{&pool}, m_Data{std::forward<Args>(args)...} {}

      [[nodiscard]]
      const T& get() const noexcept { return m_Data; }

      template<class... Args>
      std::shared_ptr<data_wrapper> set(Args&&... args)
      {
        return m_pPool->make_data_wrapper(std::forward<Args>(args)...);
      }

    private:
      data_pool* m_pPool{};
      T m_Data;

      data_wrapper(const data_wrapper&) = delete;
      data_wrapper(data_wrapper&&) noexcept = default;

      data_wrapper& operator=(const data_wrapper&) = delete;
      data_wrapper& operator=(data_wrapper&&) noexcept = default;
    };

    class proxy
    {
    public:
      friend class data_pool;
      using value_type = T;

      proxy(const proxy&)     = default;
      proxy(proxy&&) noexcept = default;
      proxy& operator=(const proxy& in)
      {
        set(in.get());
        return *this;
      }
      proxy& operator=(proxy&&) noexcept = default;

      [[nodiscard]]
      const T& get() const noexcept { return m_Handle->get(); }

      template<class... Args>
      void set(Args&&... args)
      {
        m_Handle = m_Handle->set(std::forward<Args>(args)...);
      }

      template<std::invocable<T&> Fn>
      std::invoke_result_t<Fn, T&> mutate(Fn fn)
      {
        auto nascent{m_Handle->get()};
        if constexpr (std::is_void_v<std::invoke_result_t<Fn, T&>>)
        {
            fn(nascent);
            set(std::move(nascent));
        }
        else
        {
            auto&& retVal{fn(nascent)};
            set(std::move(nascent));

            return retVal;
        }
      }

      [[nodiscard]]
      friend bool operator==(const proxy& lhs, const proxy& rhs) noexcept
        requires deep_equality_comparable<T>
      {
        return lhs.get() == rhs.get();
      }
    private:
      std::shared_ptr<data_wrapper> m_Handle;

      proxy(const std::shared_ptr<data_wrapper>& ptr) : m_Handle{ptr} {}
    };

  private:
    using wrapper_handle = std::shared_ptr<data_wrapper>;
  public:
    using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<wrapper_handle>;
  private:
    using container = std::vector<wrapper_handle, allocator_type>;
  public:
    using value_type             = T;
    using product_type           = proxy;
    using const_iterator         = impl::pool_iterator<typename container::const_iterator, data_wrapper>;
    using const_reverse_iterator = impl::pool_iterator<typename container::const_reverse_iterator, data_wrapper>;

    data_pool() = default;

    data_pool(const allocator_type& a) : m_Data(a)
    {}

    data_pool(const data_pool&) = delete;

    data_pool(data_pool&& other) noexcept : m_Data{std::move(other.m_Data)}
    {
      for(auto& pData : m_Data) pData->m_pPool = this;

      other.m_Data.clear();
    }

    data_pool(data_pool&& other, const allocator_type& a) : m_Data(a)
    {
      if constexpr(!std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
      {
        *this = std::move(other);
      }
      else if constexpr(std::allocator_traits<allocator_type>::is_always_equal::value)
      {
        swap(*this, other);
      }
      else
      {
        m_Data.reserve(other.m_Data.size());
        std::ranges::copy(other.m_Data, std::back_inserter(m_Data));

        for(auto& pData : m_Data) pData->m_pPool = this;
        other.m_Data.clear();
      }
    }

    data_pool& operator=(const data_pool&) = delete;

    data_pool& operator=(data_pool&& other) noexcept
    {
      if(&other != this)
      {
        m_Data = std::move(other.m_Data);
        for(auto& pData : m_Data) pData->m_pPool = this;
      }

      return *this;
    }

    void swap(data_pool& rhs) noexcept
    {
      std::ranges::swap(this->m_Data, rhs.m_Data);
      for(auto& pData : this->m_Data) pData->m_pPool = this;
      for(auto& pData : rhs.m_Data)   pData->m_pPool = &rhs;
    }

    friend void swap(data_pool& lhs, data_pool& rhs) noexcept
    {
      lhs.swap(rhs);
    }

    [[nodiscard]]
    friend bool operator==(const data_pool& lhs, const data_pool& rhs) noexcept
    {
      return std::ranges::equal(lhs.m_Data, rhs.m_Data, [](const wrapper_handle& l, const wrapper_handle& r){ return l->get() == r->get(); });
    }

    [[nodiscard]]
    bool empty() const noexcept { return m_Data.empty(); }

    [[nodiscard]]
    std::size_t size() const noexcept { return m_Data.size(); }

    [[nodiscard]]
    const_iterator begin() const noexcept { return const_iterator{m_Data.begin()}; }

    [[nodiscard]]
    const_iterator end() const noexcept { return const_iterator{m_Data.end()}; }

    [[nodiscard]]
    const_iterator cbegin() const noexcept { return const_iterator{m_Data.cbegin()}; }

    [[nodiscard]]
    const_iterator cend() const noexcept { return const_iterator{m_Data.cend()}; }

    [[nodiscard]]
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{m_Data.rbegin()}; }

    [[nodiscard]]
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator{m_Data.rend()}; }

    [[nodiscard]]
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{m_Data.crbegin()}; }

    [[nodiscard]]
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator{m_Data.crend()}; }

    [[nodiscard]]
    allocator_type get_allocator() const
    {
      return m_Data.get_allocator();
    }

    template<class... Args>
    [[nodiscard]]
    proxy make(Args&&... args)
    {
      const T nascent{std::forward<Args>(args)...};
      const auto found = std::ranges::find_if(m_Data, [&nascent](const auto& pData) { return pData->get() == nascent;});
      if(found == m_Data.end())
      {
        m_Data.emplace_back(std::make_shared<data_wrapper>(*this, nascent));
        return {m_Data.back()};
      }
      else
      {
        return {*found};
      }
    }

    /*! \brief Removes elements to which no proxies point. */
    void tidy()
    {
      m_Data.erase(std::ranges::remove_if(m_Data, [](const auto& ptr) { ptr.use_count() == 1;}), m_Data.end());
    }

  private:
    container m_Data;

    template<class... Args>
    wrapper_handle make_data_wrapper(Args&&... args)
    {
      const T nascent{std::forward<Args>(args)...};
      const auto found{
        std::ranges::find_if(m_Data, [&nascent](const auto& pData) { return pData->get() == nascent;})
      };

      if(found == m_Data.end())
      {
        m_Data.emplace_back(std::make_shared<data_wrapper>(*this, nascent));
        return m_Data.back();
      }
      else
      {
        return *found;
      }
    }
  };
}
