# Goose language migration

This repository uses the [Goose programming language](https://github.com/aardappel/goose), pinned to **v2026.3**, source commit `8511ae34bdc9aabf77baaab80b4d659ea87835e2`, runtime hash `4d3da949e563`. The existing NT operating system remains the native platform while application logic is moved into Goose. A Goose application module linked into that platform is a partial language port; it is not a Goose implementation of the kernel or a Windows equivalent.

## Supported migration boundary

Goose emits C and can expose real Goose implementations using `export fn`. With `--header`, the generated translation unit has no `main`, and its generated header describes the C interface. The native host calls `goose_init(argc, argv)` before calling any exports. In v2026.3, initialization and subsequent export calls must stay on the same thread; exported calls and mutable Goose globals are not thread-safe. This release does not expose `goose_exit`.

The Store retry policy in `goose/catalog_retry.goose` is an example of a suitable boundary: a scalar error code enters and a scalar result leaves. It accepts exactly the seven original transient network errors. Certificate, integrity, format and access failures remain excluded. Its generated C return type is `uint8_t`, representing Goose `bool` as 0 or 1. Generate and include the compiler's header rather than declaring a C++ `bool` signature manually.

Further deterministic application policies and numerical algorithms can be moved behind similar interfaces. Win32 windows, callback entry points, COM objects, WLAN resources and service handles need native adapters that preserve the platform's lifetimes, alignment and calling conventions. A native adapter must not retain a borrowed Goose reference after its call. UI-thread policies must not be called directly by background native worker threads.

## Constraints on a complete OS port

| Area | Evidence in the pinned compiler | Consequence |
| --- | --- | --- |
| Target representation | The C backend targets 64-bit little-endian hosts with unaligned packed accesses. | A 32-bit NT target needs a compiler/backend port. Emitting C does not establish support for every C target. |
| Data layout | Goose structures are packed concatenations of their fields. | Native NT/Win32 structures must be translated explicitly; matching field names do not establish ABI compatibility. Prefer scalars or generated-header types at the boundary. |
| Runtime initialization | The Windows runtime calls `AddVectoredExceptionHandler`, `VirtualAlloc`, `VirtualFree`, `SetThreadStackGuarantee` and CRT routines. | The supplied runtime is a user-mode runtime. It cannot be linked unchanged into the bootloader or NT kernel, where these process APIs are unavailable. |
| Memory and faults | Data stacks reserve virtual address ranges and commit pages in an access-violation handler. | Kernel IRQL rules, nonpaged allocation, interrupt stacks and kernel fault handling require a separately designed runtime. |
| Concurrency | Goose thread programs isolate globals and communicate through queues; the Windows runtime starts them using `CreateThread`. | NT scheduler, interrupt, DPC, driver and shared-state synchronization code requires a deliberate concurrency design and a native/kernel runtime port. |
| C exports | The host initializes the program once, and exports remain on that initializing thread. | A native multithreaded program cannot treat one Goose program instance as a generally thread-safe library. |

A complete conversion therefore requires compiler/runtime work, explicit NT ABI support, replacing native kernel and driver logic, boot and hardware testing, and validating Windows application compatibility. The current application ports do not demonstrate those properties. Passing host tests, including Wine tests, does not demonstrate that a newly built image boots or that physical WLAN hardware works.

## Sources for v2026.3

- [Compiler release](https://github.com/aardappel/goose/releases/tag/v2026.3) and [pinned source](https://github.com/aardappel/goose/tree/8511ae34bdc9aabf77baaab80b4d659ea87835e2).
- [Language specification, C interoperation and exports (§7.10–7.11)](https://github.com/aardappel/goose/blob/v2026.3/docs/goose_spec.md#710-extern-fn), including the [export initialization/thread restriction](https://github.com/aardappel/goose/blob/v2026.3/docs/goose_spec.md#711-exporting-goose-functions-to-c).
- [C backend target representation (§6.1)](https://github.com/aardappel/goose/blob/v2026.3/docs/implementation.md#61-values) and [packed representation (Appendix C)](https://github.com/aardappel/goose/blob/v2026.3/docs/goose_spec.md#appendix-c-representation-and-calling-convention-normative-for-the-c-backend).
- [Windows runtime memory and exception handling](https://github.com/aardappel/goose/blob/v2026.3/src/runtime/runtime_impl.h#L284), [runtime startup](https://github.com/aardappel/goose/blob/v2026.3/src/runtime/runtime_impl.h#L1332), and [native thread startup](https://github.com/aardappel/goose/blob/v2026.3/src/runtime/runtime_threads.h#L213).
- [Concurrency model (§11.2)](https://github.com/aardappel/goose/blob/v2026.3/docs/goose_spec.md#112-concurrency).
- Microsoft error definitions: [system errors 1000–1299](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--1000-1299-), [system errors 2100–3999](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--2100-3999-), and [WinINet errors](https://learn.microsoft.com/en-us/windows/win32/wininet/wininet-errors).
