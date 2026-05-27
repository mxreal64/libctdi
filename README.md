# libctdi

A blazing-fast, zero-allocation compile-time dependency injection (DI) container utilizing **C++26 Static Reflection** and modern **C++23 Modules**. 

`libctdi` automatically extracts struct dependencies, prevents runtime heap degradation via inline stack construction, and employs static graph analysis to catch circular or missing dependencies directly as explicit compiler errors.

## Key Features

* **Automatic Constructor Injection**: Leverages C++26 reflection (`^^`) to extract data fields without requiring manual registration or macro decoration.
* **Zero Runtime Overhead**: All dependency validation and graph resolutions happen during compilation. Objects are constructed instantly on the stack.
* **Static Graph Validation**: Catches circular dependency loops and missing service registrations before the binary is ever built.
* **Clean Module Boundaries**: Fully encapsulated module layout utilizing `import std;` to keep translation units insulated and compilation speeds maximum.

## Installation & Setup

Ensure you are utilizing a cutting-edge compiler layout (such as **GCC 16.1+** or Clang equivalents) with experimental C++26 features active.

### Build Configuration
Compile your targets with the following dialect adjustments enabled:
```bash
g++ -std=c++26 -fmodules-ts --compile-std-module main.cpp -o app
```

## Architectural Example

```cpp
import std;
import CompileTimeDI;

using namespace ctdi;

// Your daily operational service structures
struct DatabaseEngine {
    std::string connection_string = "db://localhost";
};

struct NotificationService {
    // libctdi automatically detects this dependency via static reflection!
    DatabaseEngine db; 
};

int main() {
    // 1. Establish your compile-time service registry layout
    constexpr CompileTimeDI di_container<
        ServiceDescriptor<DatabaseEngine, Lifetime::Singleton>,
        ServiceDescriptor<NotificationService, Lifetime::Transient>
    >();

    // 2. Resolve your target root object instantly with zero runtime allocation
    const auto& notifier = di_container.resolve<NotificationService>();
    
    std::println("DI Pipeline successfully initialized.");
    return 0;
}
```

## Static Error Enforcement

If a circular loop or a missing service registration occurs within your tree, the compiler will safely halt production immediately using explicit `static_assert` diagnostic aborts:

```text
error: static assertion failed: " COMPILE-TIME ERROR: Circular Dependency Loop Detected!"
   52 |         static_assert(!Contains_v<CleanTarget, PathList>, "...");
```

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
