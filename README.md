# ZC

A very small teaching compiler written in C++ that tokenizes a tiny language and emits x86\_64 NASM assembly.
This repository contains:

* `main.cpp` — the generator (reads a source file, produces assembly).
* `Makefile` — builds the generator and places all outputs under `build/`.
* `build/asm/main1.asm` — assembly produced by the generator (after you run it).
* `build/asm/main1` — assembled & linked executable (after you assemble & link).

---

## Language overview

This is a minimal language for demonstration. The important constructs are:

* **Variable declarations**

  * `let <name> = 42;` → integer variable (declared as `dd` in `.data`)
  * `let <name> = "hello";` → string variable (declared as `db "hello", 0`)
* **Print**

  * `print(name);` → print the contents of a string variable
  * `print("literal");` → print the literal string
* **Return / exit**

  * `return 0;` → exit syscall with code `0`
  * `return someVar;` → exit using an integer variable
* **Syntax rules**

  * Statements must end with `;`
  * Strings must use double quotes
  * Parentheses required for `print(...)`
  * Identifiers: letters/digits only
  * No complex expressions — just the forms above

Example program:

```text
let name = "hello";
let name2 = "world";
print(name);
print(" ");
print(name2);
return 0;
```

---

## Requirements

Linux x86\_64 environment with:

* `g++` (C++17)
* `make`
* `nasm`
* `ld` (comes with binutils)

Install on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential nasm
```

---

## Build & Run Instructions

Everything is managed with the provided `Makefile`.
All outputs go into the `build/` directory.

### 1. Build the generator

```bash
make
```

Produces:

```
build/compiler
```

### 2. Run the generator on a source file

```bash
make run INPUT=examples/hello.zc.
```

This creates:

```
build/asm/main1.asm
```

### 3. Assemble & link the generated assembly

```bash
make asm
```

This creates:

```
build/asm/main1   # executable
```

### 4. Run the assembled program

```bash
make run-asm
```

### 5. Clean everything

```bash
make clean
```

---

## Example Workflow

Create a file `examples/hello.zc`, then run:

```bash
make
make run INPUT=examples/hello.zc
make asm
make run-asm
```

Expected output:

```
hello world
```

---

## Ready-made example file (`examples/hello.src`)

You can copy-paste the following into `examples/hello.src`:

```text
let name = "hello";
let name2 = "world";
print(name);
print(" ");
print(name2);
print("\n");
return 0;
```

---

## Files Produced (summary)

* `build/compiler` — compiled generator
* `build/asm/main1.asm` — assembly from generator
* `build/asm/main1.o` — object file
* `build/asm/main1` — final executable

---

## Troubleshooting

* **`nasm: command not found`** → install with `sudo apt install nasm`
* **Assembler errors** → inspect `build/asm/main1.asm` for malformed `.data` or strings
* **Wrong output length** → `rdx` must equal string length (the generator handles this automatically, but manual edits require adjustments)
* **Linking issues** → use `ld -o build/asm/main1 build/asm/main1.o` (or `gcc -nostdlib -no-pie ...`)

---

## Notes

This is not meant to be a full compiler — it’s a teaching project showing how to:

* Tokenize a source string
* Generate NASM assembly
* Assemble & run the result

**Future ideas**

* Escape sequences (`\n`, `\t`) in strings
* More operators / expressions
* Better error messages

---

