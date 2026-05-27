// Copyright (C) 2026 mxreal64
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://gnu.org>.

module;

#include <meta>

export module CompileTimeDI;

import std;

namespace ctdi {

template <typename... Ts>
struct TypeList {
    static constexpr std::size_t size = sizeof...(Ts);
};

template <typename T>
consteval auto ExtractDependencies() {
    constexpr std::meta::info type_meta = ^^std::decay_t<T>;
    constexpr auto fields = std::meta::nonstatic_data_members_of(type_meta);
    return []<std::size_t... Is>(std::index_sequence<Is...>, auto fields_range) {
        return TypeList<typename [: std::meta::type_of(fields_range[Is]) :]...>{};
    }(std::make_index_sequence<fields.size()>(), fields);
}

template <typename T>
using GetDependencies_t = decltype(ExtractDependencies<T>());

export enum class Lifetime { Transient, Singleton };

export template <typename T, Lifetime L>
struct ServiceDescriptor {
    using ServiceType = T;
    static constexpr Lifetime lifetime = L;
};

template <typename T, typename List> struct Contains;
template <typename T, typename... Ts> 
struct Contains<T, TypeList<Ts...>> : std::bool_constant<(std::is_same_v<std::decay_t<T>, std::decay_t<Ts>> || ...)> {};

template <typename T, typename List> constexpr bool Contains_v = Contains<T, List>::value;

template <typename T, typename List> struct Append;
template <typename T, typename... Ts> struct Append<T, TypeList<Ts...>> { using type = TypeList<Ts..., T>; };

template <typename Target, typename ContainerList, typename PathList>
constexpr bool ValidateDependencyGraph() {
    using CleanTarget = std::decay_t<Target>;
    if constexpr (Contains_v<CleanTarget, PathList>) {
        static_assert(!Contains_v<CleanTarget, PathList>, " COMPILE-TIME ERROR: Circular Dependency Loop Detected!");
        return false;
    }
    else if constexpr (!Contains_v<CleanTarget, ContainerList>) {
        static_assert(Contains_v<CleanTarget, ContainerList>, " COMPILE-TIME ERROR: Required Dependency missing from registration!");
        return false;
    } 
    else {
        using Deps = GetDependencies_t<CleanTarget>;
        return []<typename... Ds>(TypeList<Ds...>) {
            using NewPath = typename Append<CleanTarget, PathList>::type;
            return (ValidateDependencyGraph<Ds, ContainerList, NewPath>() && ...);
        }(Deps{});
    }
}

export template <typename... Registrations>
class CompileTimeDI {
private:
    using RegisteredTypes = TypeList<typename Registrations::ServiceType...>;
    template <typename T> struct Wrapper { T instance; };
    using SingletonStorageTuple = std::tuple<Wrapper<typename Registrations::ServiceType>...>;
    mutable SingletonStorageTuple mutable_storage;

    static constexpr bool ValidateAll() {
        return (ValidateDependencyGraph<typename Registrations::ServiceType, RegisteredTypes, TypeList<>>() && ...);
    }
    static_assert(ValidateAll(), "DI Tree validation failed.");

    template <typename T>
    static constexpr Lifetime GetLifetime() {
        Lifetime found = Lifetime::Transient;
        ((std::is_same_v<std::decay_t<T>, std::decay_t<typename Registrations::ServiceType>> ? (found = Registrations::lifetime) : found), ...);
        return found;
    }

// ronaldooooooooooooooooooooooo

public:
    constexpr CompileTimeDI() noexcept = default;

    template <typename T>
    [[nodiscard]] constexpr decltype(auto) resolve() const {
        static_assert(Contains_v<T, RegisteredTypes>, " Requested root type is not registered.");
        constexpr Lifetime L = GetLifetime<T>();
        if constexpr (L == Lifetime::Singleton) {
            return (std::get<Wrapper<std::decay_t<T>>>(mutable_storage).instance);
        } else {
            using Deps = GetDependencies_t<std::decay_t<T>>;
            return []<typename... Ds>(TypeList<Ds...>, const auto& self) {
                return std::decay_t<T>{ self.template resolve<std::decay_t<Ds>>()... };
            }(Deps{}, *this);
        }
    }
};

}
