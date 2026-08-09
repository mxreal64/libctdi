# ctdi

A blazing-fast, zero-allocation compile-time dependency injection (DI) container utilizing **C++26 Static Reflection** and modern **C++26 Named Modules**.

`ctdi` serves as **Pillar 1 (The Builder)** of an upcoming non-strict compile-time safety framework. It automatically extracts struct field dependencies via static reflection, blocks runtime heap degradation via inline stack construction, and employs recursive static graph analysis alongside strict memory-safety audits to drop the hammer on bad architecture directly as explicit compiler errors.

## Key Features

* **Automatic Field Subscript Injection**: Leverages modern C++26 reflection operator (`^^`) to pull target data fields without requiring manual token registrations, verbose configuration blocks, or macro decoration.
* **Automated Memory Auditing**: The compilation engine performs deep static scans on the fields of every registered service. If an unmanaged raw pointer field is sneaked into your structural definitions, compilation halts immediately.
* **Zero Runtime Overhead**: All dependency topology validation, reference safety checks, and graph resolutions are processed entirely during the compilation phase. Final execution states are constructed instantly on the stack frame.
* **Static Graph Validation**: Catches nested circular dependency loops and missing service component registrations before a single line of your target machine binary code is ever generated.
* **Clean Module Isolation**: Fully encapsulated module boundary layout using pure C++ named module paths to keep compilation translation units insulated and compilation speeds maximum.

## Installation & Setup

Ensure you are utilizing a cutting-edge compiler layout (**GCC 16.1+ snapshot** or Clang equivalents) with experimental C++26 reflection features active.

### Pre-compiling Core System Header Modules
Because early snapshot compiler builds do not bundle experimental reflection symbols into standard monolithic modules yet, you must use `-fsearch-include-path` to pre-generate the path-aware Compiled Module Interfaces (CMIs) for the system standard library before constructing the framework:

```bash
# Initialize and build the path-tracked system module maps
g++ -std=c++26 -fmodules-ts -freflection -fsearch-include-path -x c++-header -c meta
g++ -std=c++26 -fmodules-ts -freflection -fsearch-include-path -x c++-header -c type_traits
g++ -std=c++26 -fmodules-ts -freflection -fsearch-include-path -x c++-header -c tuple
g++ -std=c++26 -fmodules-ts -freflection -fsearch-include-path -x c++-header -c utility
g++ -std=c++26 -fmodules-ts -freflection -fsearch-include-path -x c++-header -c cstddef
```

### Build & Link Sequence
Once your local machine's module cache is populated, compile your local core developer module interface and link your main entry runner file:

```bash
# Compile the framework module component interface
g++ -std=c++26 -fmodules-ts -freflection -c ctdi.cppm

# Compile your main application file and link down to bare metal machine code
g++ -std=c++26 -fmodules-ts -freflection main.cpp ctdi.o -o app
```

## Architectural Example

```cpp
// main.cpp
import CompileTimeDI;

using namespace ctdi;

// Your daily operational service structures
struct DatabaseEngine {
    int connection_id = 42;
};

struct NotificationService {
    // ctdi automatically detects this dependency via C++26 static reflection!
    DatabaseEngine db; 
};

int main() {
    // 1. Establish your pure static container pipeline registry
    using ApplicationContainer = CompileTimeDI<
        ServiceDescriptor<DatabaseEngine, Lifetime::Singleton>,
        ServiceDescriptor<NotificationService, Lifetime::Transient>
    >;

    constexpr ApplicationContainer di_container;

    // 2. Resolve your target root object instantly with zero runtime allocation tax
    auto notifier = di_container.resolve<NotificationService>();
    
    __builtin_printf("DI Pipeline successfully initialized! Conn ID: %d\n", notifier.db.connection_id);
    return 0;
}
```

## Static Error Enforcement

### 1. Circular Dependency Trap
If a circular loop or an unregistered service sequence path is introduced within your application topology tree, the compiler will safely halt production immediately using explicit static diagnostic aborts:

```text
error: static assertion failed: " COMPILE-TIME ERROR: Circular Dependency Loop Detected!"
   72 |         static_assert(!Contains_v<CleanTarget, PathList>, "...");
```

### 2. Secure Architecture Memory Audit
If a service definition violates memory safety guidelines by introducing an unmanaged raw pointer, the `template for` unroller catches it immediately, throwing a hard architectural dismissal:

```text
error: static assertion failed: " HARD DISMISSAL: Secure architecture violation! Raw pointers are forbidden in registered services."
   34 |             static_assert(!is_raw_ptr, "...");
```

*Disclaimer: This library relies on cutting-edge features tracking the working ISO C++26 P2996 specifications and is validated to compile successfully on GCC 16 snapshot builds.*

## License

Copyright (C) 2026 mxreal64

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://gnu.org>.
