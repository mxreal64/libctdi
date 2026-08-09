// Copyright (C) 2026 mxreal64
// Licensed under the GPL-3.0 layout framework
export module ctdi;

import <meta>;
import <type_traits>;
import <tuple>;
import <utility>;
import <cstddef>;

namespace ctdi {

    template <typename... Ts>
    struct TypeList {
        static constexpr std::size_t size = sizeof...(Ts);
    };

    // Internal metadata collectors to build a TypeList safely
    template <typename List, typename T> struct AppendToTypeList;
    template <typename... Ts, typename T>
    struct AppendToTypeList<TypeList<Ts...>, T> {
        using type = TypeList<Ts..., T>;
    };

    // Extracts dependencies and applies structural memory safety audits
    template <typename T>
    consteval auto ExtractDependencies() {
        using CleanType = std::decay_t<T>;
        using FinalList = TypeList<>;

        // 🚀 FIXED: Added "static" to anchor the compile-time storage address firmly
        static constexpr auto static_members = std::define_static_array(
            std::meta::members_of(^^CleanType, std::meta::access_context::unchecked())
        );

        // 🛡️ Iterate over the stabilized static array safely without lifetime warnings
        template for (constexpr std::meta::info member : static_members) {

            // Filter: Check if the member token is a field variable using standard is_variable
            if constexpr (std::meta::is_variable(member)) {
                using FieldType = typename [: std::meta::type_of(member) :];

                // 🛡️ The Raw Pointer Audit: Halt if an unmanaged raw pointer is found
                constexpr bool is_raw_ptr = std::is_pointer_v<FieldType>;
                if constexpr (is_raw_ptr) {
                    static_assert(!is_raw_ptr, " HARD DISMISSAL: Secure architecture violation! Raw pointers are forbidden in registered services.");
                }

                // Append field type to our compile-time type collection
                using FinalList = typename AppendToTypeList<FinalList, FieldType>::type;
            }
        }

        return FinalList{};
    }

    template <typename T>
    using GetDependencies_t = decltype(ExtractDependencies<T>());

    export enum class Lifetime { Transient, Singleton };

    export template <typename T, Lifetime L>
    struct ServiceDescriptor {
        using ServiceType = T;
        static constexpr Lifetime lifetime = L;
    };

    // Metaprogramming graph resolution helper traits
    template <typename T, typename List> struct Contains;
    template <typename T, typename... Ts>
    struct Contains<T, TypeList<Ts...>> : std::bool_constant<(std::is_same_v<std::decay_t<T>, std::decay_t<Ts>> || ...)> {};

    template <typename T, typename List> constexpr bool Contains_v = Contains<T, List>::value;

    template <typename T, typename List> struct Append;
    template <typename T, typename... Ts> struct Append<T, TypeList<Ts...>> { using type = TypeList<Ts..., T>; };

    // Deep recursive validation pass for tracking circular dependency loops
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

        // cristiannoooo
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
                return []<typename... Ds>(TypeList<Ds...>, const auto& self) -> std::decay_t<T> {
                    return std::decay_t<T>{ self.template resolve<std::decay_t<Ds>>()... };
                }(Deps{}, *this);
            }
        }
    };

} // namespace ctdi
