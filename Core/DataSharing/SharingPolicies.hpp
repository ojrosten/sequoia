#pragma once

#include "StaticData.hpp"

#include <map>
#include <memory>

namespace sequoia
{
  namespace data_sharing
  {
    template<class T, class... Args>
    std::shared_ptr<T> make_shared_braced(Args&&... args)
    {
      return std::shared_ptr<T>(new T{std::forward<Args>(args)...});
    }
    
    template<class T>
    class shared
    {
    public:
      using handle_type = std::shared_ptr<T>;
      using elementary_type = T;
      static constexpr bool is_shared() { return true; }

      template<class... Args> static auto make(Args&&... args) { return make_shared_braced<T>(std::forward<Args>(args)...); }
      static T& get(const handle_type ptr) { return *ptr; }
    private:
    };

    template<class T>
    class independent
    {
    public:
      using handle_type = T;
      using elementary_type = T;
      static constexpr bool is_shared() { return false; }

      template<class... Args> constexpr static T make(Args&&... args) { return T{std::forward<Args>(args)...}; }

      static constexpr T& get(T& in) noexcept { return in; }
      static constexpr const T& get(const T& in) noexcept { return in; }
    private:
    };
  }
}
