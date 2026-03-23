# Custom Malloc Allocator (C)

This project implements a small custom allocator with:

- `my_malloc`
- `my_free`
- `my_calloc`
- `my_realloc`

It also includes an interactive CLI test interface to display allocator behavior, plus diagnostics for dump, heap visualization, and fragmentation stats.

## Requirements

- macOS/Linux with a C compiler (`cc`/`clang`/`gcc`)
- `make`

## Build

```sh
make
```

This produces the `demo` executable.

## Run

```sh
./demo
```

You will enter an interactive prompt:

```text
allocator>
```

## CLI Commands

- `malloc <slot> <bytes>`: Allocate memory into a slot.
- `calloc <slot> <n> <sz>`: Allocate and zero-initialize `n * sz` bytes.
- `realloc <slot> <bytes>`: Resize an existing slot allocation.
- `free <slot>`: Free a slot allocation.
- `write <slot> <text>`: Write text into allocated memory (null-terminated).
- `read <slot>`: Read slot memory as a string.
- `list`: List active slots.
- `dump`: Print detailed internal block metadata.
- `heap`: Print ASCII heap visualization (`USED`/`FREE` blocks).
- `stats`: Print allocator and fragmentation stats.
- `help`: Show command help.
- `quit`: Exit the program.

## Example Session

```text
malloc 0 64
write 0 hello
read 0
heap
stats
free 0
quit
```

## Clean Build Artifacts

```sh
make clean
```

## Notes

- The allocator API uses `my_*` names to avoid clashing with libc `malloc/free`.
- `.o` files and `demo` are generated artifacts and are ignored via `.gitignore`.
