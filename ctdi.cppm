// Copyright (C) 2026 mxreal64
// Licensed under the GPL-3.0 license
//
// NOTE: This module relies on C++26 reflection (P2996), which is not yet
// implemented by any released compiler. It has been fixed by careful reading
// of the reflection/splicing rules, not by compiling it — validate against
// an experimental P2996 toolchain (e.g. Bloomberg's Clang fork) before use.
export module ctdi;

import <meta>;
import <type_traits>;
import <tuple>;
import <utility>;
import <optional>;
import <vector>;
import <array>;
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

    // ------------------------------------------------------------------
    // Dependency extraction
    //
    // FIX: the original implementation tried to accumulate a TypeList by
    // reassigning `using FinalList = ...` *inside* a `template for` body.
    // Each iteration of `template for` is its own scope, so that
    // reassignment never escaped the loop — the function always returned
    // TypeList<>. There is no mutable "using" state to fold over like that.
    //
    // Fix: collect the qualifying member reflections into a runtime
    // std::vector<std::meta::info> during the consteval call (legal in
    // C++26 consteval), freeze it into a static array with
    // std::define_static_array, and then convert that array into a type
    // pack via splicing under an index_sequence expansion. This is the
    // standard "array of std::meta::info -> type pack" pattern for P2996.
    // ------------------------------------------------------------------

    template <typename T>
    consteval auto CollectFieldInfos() {
        using CleanType = std::decay_t<T>;
        std::vector<std::meta::info> fields;
        for (std::meta::info member :
             std::meta::members_of(^^CleanType, std::meta::access_context::unchecked())) {
            if (std::meta::is_variable(member)) {
                fields.push_back(member);
            }
        }
        return fields;
    }

    template <typename T>
    consteval auto ExtractDependencies() {
        using CleanType = std::decay_t<T>;

        // Anchor the filtered member list at a stable compile-time address.
        static constexpr auto fields = std::define_static_array(CollectFieldInfos<CleanType>());

        return [] <std::size_t... I> (std::index_sequence<I...>) {
            // The Raw Pointer Audit: halt if any field is an unmanaged raw pointer.
            // Run once per field before building the list, so the assertion
            // message points at the actual offending registration.
            ([] {
                using FieldType = typename [: std::meta::type_of(fields[I]) :];
                static_assert(!std::is_pointer_v<FieldType>,
                    " HARD DISMISSAL: Secure architecture violation! "
                    "Raw pointers are forbidden in registered services.");
            }(), ...);

            return TypeList<typename [: std::meta::type_of(fields[I]) :]...>{};
        }(std::make_index_sequence<fields.size()>{});
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
    struct Contains<T, TypeList<Ts...>>
        : std::bool_constant<(std::is_same_v<std::decay_t<T>, std::decay_t<Ts>> || ...)> {};

    template <typename T, typename List> constexpr bool Contains_v = Contains<T, List>::value;

    template <typename T, typename List> struct Append;
    template <typename T, typename... Ts> struct Append<T, TypeList<Ts...>> { using type = TypeList<Ts..., T>; };

    // Deep recursive validation pass for tracking circular dependency loops.
    // This now actually walks real dependency lists, since ExtractDependencies
    // is fixed above — previously it only ever recursed into TypeList<>.
    template <typename Target, typename ContainerList, typename PathList>
    constexpr bool ValidateDependencyGraph() {
        using CleanTarget = std::decay_t<Target>;
        if constexpr (Contains_v<CleanTarget, PathList>) {
            static_assert(!Contains_v<CleanTarget, PathList>,
                " COMPILE-TIME ERROR: Circular Dependency Loop Detected!");
            return false;
        }
        else if constexpr (!Contains_v<CleanTarget, ContainerList>) {
            static_assert(Contains_v<CleanTarget, ContainerList>,
                " COMPILE-TIME ERROR: Required Dependency missing from registration!");
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

        // FIX: singletons are now constructed lazily, on first resolve(),
        // rather than eagerly as a plain default-constructed member. This:
        //   1. actually threads resolved dependencies into the constructor
        //      (previously they were default-constructed with no args and
        //      would fail to compile for any type without a default ctor);
        //   2. sidesteps singleton-depends-on-singleton construction-order
        //      problems, since each singleton is only built the first time
        //      it's actually asked for.
        template <typename T> struct Wrapper {
            mutable std::optional<T> instance;
        };
        using SingletonStorageTuple = std::tuple<Wrapper<typename Registrations::ServiceType>...>;
        mutable SingletonStorageTuple mutable_storage;

        static constexpr bool ValidateAll() {
            return (ValidateDependencyGraph<typename Registrations::ServiceType, RegisteredTypes, TypeList<>>() && ...);
        }

        static_assert(ValidateAll(), "DI Tree validation failed.");

        template <typename T>
        static constexpr Lifetime GetLifetime() {
            Lifetime found = Lifetime::Transient;
            ((std::is_same_v<std::decay_t<T>, std::decay_t<typename Registrations::ServiceType>>
                  ? (found = Registrations::lifetime)
                  : found), ...);
            return found;
        }

        template <typename T>
        constexpr std::decay_t<T> Construct() const {
            using Deps = GetDependencies_t<std::decay_t<T>>;
            return []<typename... Ds>(TypeList<Ds...>, const auto& self) -> std::decay_t<T> {
                return std::decay_t<T>{ self.template resolve<std::decay_t<Ds>>()... };
            }(Deps{}, *this);
        }

    public:
        constexpr CompileTimeDI() noexcept = default;

        template <typename T>
        [[nodiscard]] constexpr decltype(auto) resolve() const {
            static_assert(Contains_v<T, RegisteredTypes>, " Requested root type is not registered.");
            constexpr Lifetime L = GetLifetime<T>();

            if constexpr (L == Lifetime::Singleton) {
                auto& wrapper = std::get<Wrapper<std::decay_t<T>>>(mutable_storage);
                if (!wrapper.instance.has_value()) {
                    wrapper.instance = Construct<T>();
                }
                return (*wrapper.instance);
            } else {
                return Construct<T>();
            }
        }
    };

} // namespace ctdi
