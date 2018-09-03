#pragma once

#include "Utilities/ProtectiveWrapper.hpp"

#include <vector>
#include <memory>

namespace sequoia
{
  namespace data_sharing
  {
    template<class T> class unpooled
    {
    public:
      using proxy = utilities::protective_wrapper<T>;
      using value_type = T;

      template<class... Args> static constexpr proxy make(Args&&... args)
      {
        return proxy{std::forward<Args>(args)...};
      }
    private:
    };
    
    template<class T> class data_pool
    {
    public:
      class handle
      {
      friend class data_pool;
        
      public:
        template<class... Args> handle(data_pool& pool, Args&&... args) : m_pPool{&pool}, m_Data{std::forward<Args>(args)...} {}        
        
        const T& get() const { return m_Data; }

        template<class... Args>
        std::shared_ptr<handle> set(Args&&... args)
        {
          return m_pPool->make_handle(std::forward<Args>(args)...);
        }

      private:        
        data_pool* m_pPool{};
        T m_Data;

        handle(const handle&) = delete;
        handle(handle&&) noexcept = default;

        handle& operator=(const handle&) = delete;
        handle& operator=(handle&&) noexcept = default;
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
                                                     
        const T& get() const noexcept { return m_Handle->get(); }

        template<class... Args>
        void set(Args&&... args)
        {
          m_Handle = m_Handle->set(std::forward<Args>(args)...);
        }

        template<class Fn>
        void mutate(Fn&& fn)
        {
          auto nascent{m_Handle->get()};
          fn(nascent);
          set(nascent);
        }

        friend bool operator==(const proxy& lhs, const proxy& rhs) noexcept
        {
          return lhs.get() == rhs.get();
        }

        friend bool operator!=(const proxy& lhs, const proxy& rhs) noexcept
        {
          return lhs != rhs;
        }
      private:
        std::shared_ptr<handle> m_Handle;

        proxy(const std::shared_ptr<handle>& ptr) : m_Handle{ptr} {}
      };

      friend class handle;
      
      data_pool() {}

      data_pool(const data_pool&) = delete;

      data_pool(data_pool&& in) noexcept : data_pool()
      {
        swap(*this, in);
      }

      data_pool& operator=(const data_pool&) = delete;

      data_pool& operator=(data_pool&& in) noexcept
      {
        swap(*this, in);
        return *this;
      }

      friend void swap(data_pool& lhs, data_pool& rhs) noexcept
      {
        std::swap(lhs.m_Data, rhs.m_Data);
        for(auto& pData : lhs.m_Data) pData->m_pPool = &lhs;
        for(auto& pData : rhs.m_Data) pData->m_pPool = &rhs;
      }

      // may also want to count number of times items are used
      std::size_t size() const { return m_Data.size(); }

      template<class... Args> proxy make(Args&&... args)
      {
        const T nascent{std::forward<Args>(args)...};
        const auto found = std::find_if(m_Data.begin(), m_Data.end(), [&nascent](const auto& pData) { return pData->get() == nascent;});
        if(found == m_Data.end())
        {
          m_Data.emplace_back(std::make_shared<handle>(*this, nascent));
          return {m_Data.back()};
        }
        else
        {
          return {*found};
        }
      }
      
      void tidy()
      {
        m_Data.erase(std::remove_if(m_Data.begin(), m_Data.end(), [](const auto& ptr) { ptr.use_count() == 1;}), m_Data.end());
      }

    private:
      std::vector<std::shared_ptr<handle>> m_Data;

      template<class... Args> std::shared_ptr<handle> make_handle(Args&&... args)
      {
        const T nascent{std::forward<Args>(args)...};
        const auto found = std::find_if(m_Data.begin(), m_Data.end(), [&nascent](const auto& pData) { return pData->get() == nascent;});
        if(found == m_Data.end())
        {
          m_Data.emplace_back(std::make_shared<handle>(*this, nascent));
          return m_Data.back();
        }
        else
        {
          return *found;
        }
      }
    };
  }
}
