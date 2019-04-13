////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file SharingPolicies.hpp
    \brief Structs to enable homogenous treatment of data which is/is not
    handled via shared pointers.

 */

#include <memory>

namespace sequoia::data_sharing
{
  template<class T, class... Args>
  [[nodiscard]]
  std::shared_ptr<T> make_shared_braced(Args&&... args)
  {
    return std::shared_ptr<T>(new T{std::forward<Args>(args)...});
  }
    
  template<class T>
  struct shared
  {
  public:
    using handle_type = std::shared_ptr<T>;
    using elementary_type = T;

    [[nodiscard]]
    static constexpr bool is_shared() noexcept { return true; }

    template<class... Args>
    [[nodiscard]] static handle_type make(Args&&... args)
    {
      return make_shared_braced<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]]
    static T& get(handle_type& ptr)
    {
      return *ptr;
    }

    [[nodiscard]]
    static const T& get(const handle_type& ptr)
    {
      return *ptr;
    }

    [[nodiscard]]
    static T* get_ptr(handle_type& ptr)
    {
      return &*ptr;
    }

    [[nodiscard]]
    static const T* get_ptr(const handle_type& ptr)
    {
      return &*ptr;
    }  
  private:
  };

  template<class T>
  struct independent
  {
  public:
    using handle_type = T;
    using elementary_type = T;

    [[nodiscard]]
    static constexpr bool is_shared() noexcept { return false; }

    template<class... Args>
    [[nodiscard]] constexpr static T make(Args&&... args)
    {
      return T{std::forward<Args>(args)...};
    }

    [[nodiscard]]
    static constexpr T& get(T& in) noexcept { return in; }

    [[nodiscard]]
    static constexpr const T& get(const T& in) noexcept { return in; }

    [[nodiscard]]
    static constexpr T* get_ptr(T& in) noexcept { return &in; }

    [[nodiscard]]
    static constexpr const T* get_ptr(const T& in) noexcept { return &in; }
  private:
  };
}
