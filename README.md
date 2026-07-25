# VSI Resolve

A distribution format for native software.

Today, distributing a compiled application for Windows, Linux, and macOS requires generating a different binary for each operating system and CPU architecture combination. VSI Resolve proposes a different approach: a single universal artifact (`.vsi`), compiled once by the developer and resolved into a native binary during installation on the end user's machine.

## How it works, in two phases

**Build.** The developer compiles the application as usual, in C/C++/Rust/Zig, against the VSI. The result is a `.vsi` file, an intermediate artifact that is not yet executable on any operating system.

**Install.** `vsir` receives the `.vsi` file on the target machine and resolves its VSI functions against a real native implementation for that platform, producing a pure native binary. No resident VM, no background interpreter. Once installed, the program runs directly on the hardware, just like any other native executable.

## What is a VSI function?

A VSI function is not a system call from any specific operating system. It is a **pure intention**: "play this audio", "create this window", "read this file", without any commitment to how it will be implemented. The application calls that intention. Only during installation does `vsir` decide what it becomes in practice—`CreateWindowEx` on Windows, X11 on Linux, or whatever is appropriate—always chosen by the target platform, never by the developer.

## Status

The project is in its early stages. A working proof of concept already validates the complete pipeline (build → install) for one domain (audio). The remaining VSI modules are still under design.

## Learn More

The complete technical specification, including the architecture, design decisions, and roadmap, is available in [SPEC.md](./SPEC.md).
