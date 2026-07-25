# Architectural Specification and Technical Design
Virtualized Environment for Binary Programs — VSI Resolve
A Universal Layer for Hermetic Distribution and Native AOT Compilation

*Revision 2 — incorporates design decisions consolidated after validation of the first functional POC (audio domain).*

## 1. Executive Summary and Philosophy

Modern computing suffers from a fundamental historical fragmentation: compiled programs are rigidly coupled to a specific combination of CPU Architecture (ISA) and Operating System (OS). An executable compiled for x86_64/Linux does not run on ARM64/Windows, nor on x86_64/macOS, requiring complex cross-compilation pipelines and packagers specific to each environment.

The VSI Resolve project redefines the compiled-software distribution model based on a central thesis:

> The LLVM ecosystem decoupled programming languages from CPU hardware. The VSI Resolve ecosystem decouples the distributed binary from both the CPU and the Operating System, turning system calls into declarations of semantic intent and performing native AOT (Ahead-Of-Time) compilation at install time through a hermetic, self-contained toolchain.

Central point of clarity: VSI Resolve is not a runtime, not a virtual machine, and not a sandboxing mechanism. It is native distribution and installation infrastructure. WASM64 bytecode exists only as an intermediate transport format. What happens at install time is AOT compilation to native C (via `wasm2c`) and direct compilation by the compiler embedded in the installer, emitting a pure machine executable (.exe, ELF, or Mach-O) with no runtime overhead.

### On the term "late linking"

The term is coined by the author; it is not an established market term. The point of comparison is traditional linking: in the conventional model, symbol resolution against a concrete implementation and native code generation happen at build time, on the developer's machine. In VSI Resolve, both of these are deferred to install time, on the end user's machine. It is "later than traditional linking," hence the name.

This deferral is not just a metaphor: when a VSI+ symbol (see Section 5) has no possible implementation for the target platform of `vsir install`, this manifests as an unresolved symbol error, in the classic linker sense, except occurring at install time rather than at the developer's build time.

### Fundamental Principles

* **Zero Changes to Languages or Compilers.** No syntax of C, C++, Rust, Zig, or Odin is modified. The developer uses standard market compilers, with the already-official WASM64 target of Clang/LLVM.
* **Linear Memory Model.** Fully compatible with raw pointers, direct allocation, and languages without a Garbage Collector.
* **Unified Distribution (.vsi).** A single generic binary artifact is distributed for all platforms.
* **Native AOT Installation.** The local installer (`vsir`) translates the bytecode and semantic intents directly into a native executable for the host system.
* **Zero Runtime Overhead.** After installation, there is no virtual machine, interpreter, or translation layer running in the background.
* **Not sandboxing.** The project's goal is distribution portability, not security isolation. A developer's memory bug (buffer overflow, incorrect pointer arithmetic) should behave exactly as it would in a traditional native program, with no artificial safety net (see Section 7).

## 2. Naming and Distribution Format

* **Tool / CLI:** `vsir` (from "VSI Resolve"), short and in the same pattern as `cargo`, `rustc`, `npm`.
* **Install command:** `vsir install file.vsi`. A common market verb ("install") was deliberately chosen over an exotic brand-tied verb ("resolve"), because vocabulary familiarity reduces discovery friction more than a differentiated verb adds in brand reinforcement. Precedent that "install" already accommodates real building and conditional failure: Homebrew (`--build-from-source`), Nix, Gentoo/Portage.
* **Artifact extension:** `.vsi`. Closed decision, ruling out `.vmbin` (carries the word "VM," which the project wants to avoid) and ruling out a double qualifier (`.vsi.wasm` or `.wasm.vsi`). The file, byte for byte, remains valid WASM64 underneath (magic number `\0asm` preserved), so any tool in the WASM ecosystem already recognizes the content without needing the extension. The dedicated extension exists only to signal to the OS/user that the file should not be opened by a generic WASM runtime, and must go through `vsir`.

## 3. The Three Pillars of the Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                        VSI RESOLVE ARCHITECTURE                        │
├────────────────────────────────────────────────────────────────────────┤
│ 1. The .vsi Artifact    ──► Universal transport format (WASM64)        │
│ 2. The VSI               ──► Semantic Intents ABI / SDK (vsi.h)        │
│ 3. vsir                   ──► Self-contained Toolchain (AOT + Compiler + Libs)│
└────────────────────────────────────────────────────────────────────────┘
```

**Pillar 1 — The .vsi Artifact.** Valid WASM64 bytecode, packaging the compute instructions, the 64-bit linear memory model, and the VSI's imported symbol table.

**Pillar 2 — The VSI (Virtual System Interface).** Portable abstraction layer that replaces direct operating-system APIs with semantic intents.

**Pillar 3 — `vsir` (Self-contained Toolchain).** Orchestrator distributed as a static executable, embedding:
* AOT translator: `wasm2c` engine (WABT).
* Native C compiler: embedded static backend (`zig cc` / `clang`).
* Static runtime: pre-compiled headers and libraries for the VSI (SDL, Vulkan, miniaudio).

## 4. Pipeline Validated by POC

The first functional POC (audio domain, via SDL2) validated the riskiest link in the chain:

```
app.cpp (unresolved vsi_audio_* symbols)
        │  clang --target=wasm64 -nostdlib -Wl,--allow-undefined
        ▼
app.wasm (imports from the "env" module)
        │  wasm2c --enable-memory64
        ▼
app_transpiled.c
        │  compiled together with vsi_bridge.cpp (translates offset → real pointer)
        │  and vsi_backend.cpp (calls real SDL2)
        ▼
Single native executable
```

This POC proved that it is possible to take real C/C++, compile it to WASM64, transpile it with `wasm2c`, and produce a functional native binary with no resident VM. The validated scope is deliberately minimal (audio only); the other VSI domains (Section 8) do not yet have a reference implementation.

**Deliberate bet on memory64**, despite it being a feature still under active evolution in WABT, motivated by the need for heavy applications to use more than 4GB of RAM. WebAssembly 3.0, ratified in June 2026, standardizes memory64 and native exception handling as production features, which reduces the risk of this bet.

**Optimization note:** `vsir`'s final link should use `-flto` to enable inlining between the app's transpiled code and the VSI implementation, currently absent from the POC's build configuration. The loss of type/alias information (TBAA) already happens the moment C++ compiles to WASM64 (linear, untyped memory model); it is not an additional loss introduced by `wasm2c`.

## 5. VSI Core and VSI+ — Universality and Extensions

The VSI is split into two headers with different guarantees:

* **`vsi.h` (core):** functions guaranteed on any supported target, including the browser. Includes, for example, the graphics model of at least one rendering queue.
* **`vsi_plus.h` (VSI+):** extensions with no universality guarantee (e.g., multiple asynchronous GPU queues). A developer who includes this header is explicitly and visibly, in the source code, giving up guaranteed portability for more restricted targets (the browser, for example).

### Two axes of variation, resolved at different moments

1. **Compatibility (install-time / real "linking").** Is there, in principle, some implementation path for this VSI+ symbol on this target class? If there is no possible fallback combination (e.g., a function that no browser implementation via WebGPU could ever satisfy), `vsir install` for that target refuses to compile, as an unresolved linker symbol.
2. **Flexibility (runtime).** Within a target class that structurally supports the feature, specific hardware still varies (weak integrated GPU vs. dedicated, cloud VM, outdated driver). This is resolved dynamically within the VSI implementation itself (`vsi.c`), via fallback and retry, never blocking compilation.

Capability-discovery model inspired by WebGPU (not OpenGL/OpenGL ES, which fragmented the ecosystem with header variants): advanced capabilities are obtained via typed opaque handles returned by a query function (e.g., `vsi_gpu_get_queue(VSI_QUEUE_COMPUTE)`). Important: since the final boundary between app and host is always raw integers (Section 6), this C type does not by itself prevent misuse — real validation on the host side is what actually guarantees safety.

## 6. Boundary Security

Every VSI call crosses from an untrusted context (the `.vsi`, potentially with arbitrary offsets and integers) to a privileged context (`vsi.c`, with real access to the GPU, filesystem, network, audio devices). Under the Golden Rule of pointer translation (Section 9 of the original revision — only primitive types and contiguous buffers cross the boundary), any "opaque" handle on the C side becomes, at the crossing, a raw integer with no type information.

**Principle:** every handle received by the host must be validated against an internal table (slot map, with index + generation to prevent reuse of a stale handle) before operating on the real resource it references. This protects the VSI implementation (which holds real OS resources) from receiving a forged or stale handle and attempting to operate on a resource that no longer exists.

**This is different from bounds checking in the app's linear memory.** Linear memory is exclusively the app's problem, not the VSI's. Given the "not sandboxing" principle (Section 1), the correct default behavior is memory-access bounds checking **turned off**, so that overflow, incorrect pointer arithmetic, etc. behave exactly as they would in a traditional native program. An optional debug tool (analogous to `-fsanitize=address`), entirely separate from the standard runtime, is a future possibility, never default behavior.

**Technical note:** `-DWASM_RT_USE_MMAP=0`, already present in the POC, is a mandatory consequence of the memory64 bet — `wasm2c`'s `wasm-rt` only offers the guard-page trick (bounds checking "for free" via hardware page fault) for 32-bit memory; memory64 forces malloc mode with explicit software bounds checking when enabled. This is an architectural tension of memory64 itself, not a passing tool immaturity — but, given the "no sandboxing" principle above, this bounds check should be explicitly disabled by default regardless.

## 7. Mapped VSI Module Surface

**Core / libc-like:** heap growth, raw I/O, exit/signal, clock (monotonic and wall-clock), random.

**File and storage:** open/read/write/close, seek, list directory, standardized paths (data, cache, temp).

**Audio:** init/write/pause/status/shutdown (already validated by POC); pending: microphone capture, multiple streams.

**Window and Display:** create/close window, resize, fullscreen, multiple monitors, DPI scaling, event loop.

**Graphics:** core = at least one queue; VSI+ = multi-queue (async compute/transfer), Vulkan/MoltenVK driver discovery at runtime (not statically linked).

**Input:** keyboard (key + text), mouse; VSI+: gamepad, touch.

**Threading:** thread creation, mutex, condition variable, atomics.

**IPC (inter-process communication):** module mapped from the start of the project, needed for plugin architectures based on separate processes (e.g., GIMP).

**Networking (VSI+):** TCP/UDP socket, convenience HTTP client. Likely VSI+ due to strong browser restrictions (CORS).

**Clipboard:** copy/paste text and binary data.

**Native dialogs:** open/save file, message box — candidate to stay outside the core, scope decision pending.

**Log/Debug:** platform-standardized channel.

**Mobile lifecycle:** pause/resume/focus — identified as necessary for the Android/iOS targets (Section 10), not yet designed.

**Deliberately out of scope:** `dlopen`/dynamic library loading. Conflicts with the hermetic AOT model (everything resolved at once at install time). This is not a missing module — it is a scope decision: a developer who needs this steps outside the VSI on purpose, losing universality only in that section, as is already acceptable for any native feature not covered.

## 8. Libc Layer

The libc shim's `.a` is **universal**, does not vary by platform or architecture, because it is compiled for the same `wasm64` target as the developer's app, never directly to native code. The difference between platforms still exists, but it is deferred to the same place as always: `vsi.c`, resolved on the target by `vsir`.

**Recommended strategy:** fork `wasi-libc`, which already splits exactly into the two needed halves:
* **Top half** (musl-based): real implementation of pure functions (`strlen`, `memcpy`, `malloc`/`free`, `math.h`), with no syscalls at all. Kept almost intact.
* **Bottom half:** thin translation layer from libc call to syscall. Rewritten to call VSI functions instead of WASI syscalls (e.g., `write()` calls `vsi_io_write()`).

This avoids rewriting the hard, tedious part of a libc (format parsing, allocator logic) from scratch, concentrating one's own effort exactly on the boundary with the outside world.

Build: each file compiled with `clang --target=wasm64 -c file.c -o file.o`, packaged with `llvm-ar rcs libvsi_libc.a *.o`, and linked together with the developer's `app.cpp` in the first build phase (resolved early, inside the `.vsi` itself, unlike domain VSI functions, which remain as an unresolved import for "late linking").

**Error policy:** any libc symbol with no real VSI implementation behind it must fail loudly (abort with a clear message) instead of turning into a silent stub, the same standard used by Emscripten (`-sERROR_ON_UNDEFINED_SYMBOLS`).

## 9. Adoption Strategy

**Main vector: forking existing libraries**, keeping headers and the public API identical, replacing only the deepest internal implementation (the part that talks directly to the OS) with VSI calls. This reduces the adoption decision from "rewrite against a new ABI" to "swap which library I link," even enabling passive adoption (projects that depend on SDL indirectly gain portability with no decision of their own).

Recommended order of attack:
1. **SDL** — first fork, because it is pure C, has a small and stable API, no moc, no exceptions, no dynamic plugins.
2. **GLib** — before GTK, because GIMP already centralizes most of the filesystem/thread/string access there, rather than scattering it across application code.
3. **Qt (via QPA plugin)** — longer-term goal. QPA is already Qt's official extension point (pluggable by design, like `xcb`/`cocoa`/`windows` today), so a `vsi` plugin fits without a deep fork of Qt itself. Direct precedent to study: Qt for WebAssembly, the official `wasm` QPA backend via Emscripten, already proves that all of Qt can go through a WASM pipeline. The conflict between `QPluginLoader` (based on `dlopen`) and the hermetic model is resolved by using the static build configuration Qt already supports (used today for iOS/embedded).

**Planned case study: GIMP.** Surfaces early the need for the IPC module (GIMP plugins run as a separate process, communicating via pipe, not via `dlopen`), already accounted for from the start of module mapping.

**Ongoing fork maintenance kept in sync with upstream** (not a one-off, abandoned fork):
* Patch stack on top of upstream (periodic `git rebase`, or `quilt`/`stgit`), the same pattern used by Debian, Gentoo, LineageOS.
* Semi-automatic triage tool: since libraries like SDL already organize backends by folder (`src/video/win32/`, `src/video/x11/`), much of the "this needs attention" classification comes for free just from looking at which file changed in each upstream release, complemented by a scan for known native API calls outside those folders.
* Conversion of new code to VSI calls is **assisted, not automatic**: the tool can suggest a first pass (a good use case for a coding assistant), but it is always reviewed by a human before entering the fork — silently mis-generated boundary code is worse than having no automation at all.

## 10. Mobile Support (Android / iOS)

Structurally different model from desktop: `vsir` **does not run on the end user's device**. The Play Store and App Store have explicit policies against downloading/compiling/executing new executable code after review (Apple guideline 2.5.2, updated June 2026), which is incompatible with "resolving on the target" the way it happens on desktop.

AOT compilation happens on the developer's own build/CI machine, generating:
* **Android:** a `.so`, packaged inside a `NativeActivity`/`android_native_app_glue` shell (the same pattern already used by SDL and Godot), avoiding writing real Java/Kotlin.
* **iOS:** a static framework/lib, inside a minimal `UIApplicationMain` in Objective-C++ (which compiles C++ directly, with no JNI-style bridge), plus Apple's mandatory signing step, which is outside `vsir`'s scope.

The rest of the architecture (VSI, libc shim) stays the same — mobile is just another `vsir` output format, not a new category of problem.

## 11. Deliberately Out of Scope (v1.0)

* **Sourcemap / code-position debugging in the final native binary.** `wasm2c` does not propagate DWARF or generate line mapping for the transpiled C. v1.0 strategy: debugging happens against the `.vsi` running on a real WASM runtime (wasmtime, browser), where DWARF already works natively today. A future `wasm2c` extension (fork/patch) to emit a WASM-offset → generated-C-position correspondence table is a post-1.0 roadmap item, not a prerequisite.
* **WASM → LLVM IR direct (via emerging MLIR dialect).** There is real, active development in LLVM (June 2025 RFC, `RaiseWasmMLIR` pass merged in 2026) that would recover full LLVM optimization power over the transpiled code. It is technically promising, but still too recent (weeks of upstream maturity) to be the foundation of v1.0. `wasm2c`, despite generating C that is less ideal for the optimizer, has more than a decade of real-world use. Treat as a separate roadmap bet from the one already made on memory64, not simultaneous with it.
* **Interception of existing pre-compiled binaries against the VSI (Wine-style).** Reimplementing a library already used dynamically by an existing binary (swapping `LD_PRELOAD`) is possible for the same ISA, but does not solve porting between different CPU architectures — that would require reliable binary lifting (nothing ready exists) or emulation (which would reintroduce the runtime VM the project rejects on principle). Out of scope, treated as an adjacent project and not under consideration.

## 12. Realistic Adoption Timeline

Correct comparison category: not a new language (Rust, Zig), but an interface layer over an already-mature ecosystem (Clang, WASM). Closest precedents: Emscripten, SDL, Vulkan.

| Window | Expected milestone |
|---|---|
| Year 1 post-launch | Curiosity traction, one-off experiments, no real third-party production use |
| Year 2–3 | First small third-party project published using VSI (plugin, niche tool) |
| Year 3–5 | Integration with a medium-sized OSS project (Godot, Audacity, GIMP) |
| Year 5+ | Any conversation with a large enterprise (long-tail bet, not an expected milestone) |

The library-fork strategy (Section 9) tends to compress this timeline, because it lowers the adoption cost from a rewrite to a dependency swap, even enabling passive adoption across dependency chains.

## 13. Comparative Infrastructure Analysis

| Feature | VSI Resolve | WebAssembly (WASI/Browser) | Java (JVM) | Traditional Compilation |
|---|---|---|---|---|
| Single Distribution Artifact? | Yes (.vsi) | Yes (.wasm) | Yes (.jar) | No (CI/CD Matrix) |
| Generates Native OS Binary? | Yes (.exe/ELF/Mach-O) | No (Runs in VM) | No (Runs on JVM) | Yes |
| User Toolchain Required? | None (Self-contained) | Requires Runtime/VM | Requires JRE/JVM | Requires GCC/Clang/MSVC |
| Base Graphics Engine | Universal Vulkan (Multi-Queue) | WebGPU (Single Queue) | JavaFX/AWT | OS-dependent |
| Runtime Overhead | 0% (Native) | Low / Medium | High (JVM + GC) | 0% |
| Multiple GPU Queues (Async Compute)? | Yes, VSI+ (Native) | No (Blocked in spec) | No | Depends on API |
| ABI Fragmentation across Linux distros | Not applicable (compiled on target) | Not applicable | Not applicable | Yes (versioned glibc) |
