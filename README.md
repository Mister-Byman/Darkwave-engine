# Darkwave Engine

**Darkwave Engine** is a Vulkan-first, low-level game engine for **Windows**, built on **SDL2**.  
It focuses on explicit GPU control, deterministic rendering, and a clean, minimal architecture for real-time games and tools.

Darkwave Engine powers **VOID SIGNAL**, providing a modern Vulkan-based rendering core with predictable frame behavior and low overhead.

---

## Goals

- Vulkan-first rendering pipeline
- Explicit control over GPU resources and synchronization
- Deterministic frame execution
- Minimal abstraction over low-level systems
- Clean and modular architecture

---

## Non-Goals

- High-level visual scripting
- Heavy editor tooling
- Hidden engine-side automation
- Cross-platform support (for now)

---

## Technology Stack

- **Language:** C++
- **Graphics API:** Vulkan
- **Platform:** Windows
- **Windowing & Input:** SDL2
- **Build System:** CMake (Visual Studio generator)
- **Compiler:** MSVC

---

## Project Status

Darkwave Engine is in **early active development**.  
The architecture is evolving, APIs are unstable, and breaking changes are expected.

---

## Building

```bash
git clone https://github.com/Mister-Byman/Darkwave-engine.git
cd Darkwave-engine

cmake -S . -B build
cmake --build build
```

---

## Contributing

If you are interested in participating in the development of Darkwave Engine, feel free to reach out:

Telegram: @mister_byman

Contributions, discussions, and technical feedback are welcome.

---

## License

License information will be added later.
