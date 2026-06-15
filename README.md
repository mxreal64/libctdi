# libctdi

A blazing-fast, zero-allocation compile-time dependency injection (DI) container utilizing **C++26 Static Reflection** and modern **C++26 Named Modules**. 

`libctdi` automatically extracts struct dependencies via static reflection, prevents runtime heap degradation via inline stack construction, and employs static graph analysis alongside memory-safety audits to drop the hammer on bad architecture directly as explicit compiler errors.

## Key Features

* **Automatic Constructor Injection**: Leverages C++26 reflection (`^^`) to extract data fields without requiring manual constructor registration, configuration blocks, or toxic macro decoration.
* **Automated Memory Auditing**: The compilation engine scans the fields of every registered service node. If an unmanaged raw pointer field is found anywhere in your dependency trees, compilation drops immediately.
* **Zero Runtime Overhead**: All dependency validation, safety checks, and graph resolutions happen entirely during the compilation phase. Objects are constructed instantly on the stack.
* **Static Graph Validation**: Catches circular dependency loops and missing service registrations before a single line of your binary is ever generated.
* **Clean Module Boundaries**: Fully encapsulated module layout utilizing pure named module paths and standard header unit isolation to keep translation units insulated and compilation speeds maximum.

## Installation & Setup

Ensure you are utilizing a cutting-edge compiler layout (**GCC 16.1+** or Clang equivalents) with experimental C++26 features active.

### Pre-compiling Core Header Units
Because early compiler builds do not bundle experimental reflection symbols into standard monolithic modules yet, you must pre-generate the Compiled Module Interfaces (CMIs) for the required standard utilities before building the framework to avoid global token collisions:

```bash
g++ -std=c++26 -fmodules-ts -freflection -x c++-header -c /usr/include/c++/16.1.1/meta
g++ -std=c++26 -fmodules-ts -freflection -x c++-header -c /usr/include/c++/16.1.1/type_traits
g++ -std=c++26 -fmodules-ts -freflection -x c++-header -c /usr/include/c++/16.1.1/tuple
g++ -std=c++26 -fmodules-ts -freflection -x c++-header -c /usr/include/c++/16.1.1/utility
g++ -std=c++26 -fmodules-ts -freflection -x c++-header -c /usr/include/c++/16.1.1/cstdio
```

### Build & Link Sequence
Once your system's module cache is populated, compile your module interface and link your main executable file:

```bash
g++ -std=c++26 -fmodules-ts -freflection -c CompileTimeDI.cppm
g++ -std=c++26 -fmodules-ts -freflection main.cpp CompileTimeDI.o -o app
```

## Architectural Example

```cpp
import CompileTimeDI;
import <cstdio>; 

using namespace ctdi;

// Your daily operational service structures
struct DatabaseEngine {
    int connection_id = 1337;
};

struct NotificationService {
    // libctdi automatically detects this dependency via C++26 static reflection!
    DatabaseEngine db; 
};

int main() {
    // 1. Establish your compile-time service registry layout
    constexpr CompileTimeDI<
        ServiceDescriptor<DatabaseEngine, Lifetime::Singleton>,
        ServiceDescriptor<NotificationService, Lifetime::Transient>
    > di_container;

    // 2. Resolve your target root object instantly with zero runtime allocation
    auto notifier = di_container.resolve<NotificationService>();
    
    std::printf(" DI Pipeline successfully initialized! Conn ID: %d\n", notifier.db.connection_id);
    return 0;
}
```

## Static Error Enforcement

### 1. Circular Dependency Trap
If a circular loop or a missing service registration occurs within your tree, the compiler will safely halt production immediately using explicit `static_assert` diagnostic aborts:

```text
error: static assertion failed: " COMPILE-TIME ERROR: Circular Dependency Loop Detected!"
   72 |         static_assert(!Contains_v<CleanTarget, PathList>, "...");
```

### 2. Memory Safety Audit Trap
If a service sneaks an unmanaged raw pointer into its members, the `template for` unroller catches it immediately, throwing a hard architectural dismissal:

```text
error: static assertion failed: " HARD DISMISSAL: Secure architecture violation! Raw pointers are forbidden in registered services."
   34 |             static_assert(!is_raw_ptr, "...");
```

*disclaimer: this may or may not compile until the c++26 standard is finalized and releazed*

## License

 Copyright (C) 2026 mxreal64

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://gnu.org>.
