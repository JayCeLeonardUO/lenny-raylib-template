-----------------------------------
_DISCLAIMER:_

Welcome to the **lenny-raylib-template**!

This template provides a base structure to start developing a small raylib game in plain C. The repo is also pre-configured with a default `LICENSE` (zlib/libpng) and a `README.md` (this one) to be properly filled by users. Feel free to change the LICENSE as required.

All the sections defined by `$(Data to Fill)` are expected to be edited and filled properly. It's recommended to delete this disclaimer message after editing this `README.md` file.

-----------------------------------

## Getting Started with this template

### Neovim

A project-local `.nvim.lua` is included and loads automatically when launching nvim from the repo root (requires `vim.o.exrc = true` in your init.lua). It sets `makeprg` to `make -C src`, adds a `:CMakeBuild` command (which also generates `compile_commands.json` for clangd), and maps `<leader>mm` to build and `<leader>mr` to run.

Other bindings: `<leader>R` (or the on-screen button) builds and runs the game in a detached tmux session, `<C-t>` toggles a pane attached to it, `<leader>r` debugs with gdbgui, and `:RadDebug` / `<leader>D` debugs with the [RAD Debugger](https://github.com/EpicGames/raddebugger) (vendored as a submodule at `vendor/raddebugger` — clone with `--recurse-submodules`; it is built automatically on first use).

### Linux

When setting up this template on linux for the first time, install the dependencies from this page:
([Working on GNU Linux](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux))

This repository comes with CMake and a plain Makefile already set up.

### mylibs

Unity build: drop plain `.c` files into `mylibs/` — no headers needed — and the CMake build amalgamates them into a single header, `mylibs/mylib.h` (or regenerate manually with `cmake -P mylibs/amalgamate.cmake`). The generator hoists everything above each file's first function (includes, defines, types) plus auto-extracted prototypes of non-static functions into the declaration section, and puts the function bodies behind `MYLIB_IMPLEMENTATION`. The header is included via `src/screens.h`, so everything in `mylibs/` is usable from `main.c` and every screen file; `main.c` defines `MYLIB_IMPLEMENTATION` so implementations compile exactly once. Conventions: one-line function signatures with the brace on its own line (raylib style — `.clang-format` enforces this), and `static` for file-scope state. The generated `mylib.h` is committed so the plain Makefile builds (and CI) work without a codegen step.

### CLI: Makefile

```sh
mkdir ~/raylib-gamejam && cd ~/raylib-gamejam
git clone --depth 1 --branch 6.0 https://github.com/raysan5/raylib
make -C raylib/src
git clone https://github.com/$(User Name)/$(Repo Name).git
cd $(Repo Name)
make -C src
src/lenny-raylib-template
```

This template has been created to be used with raylib (www.raylib.com) and it's licensed under an unmodified zlib/libpng license.

_Copyright (c) 2014-2026 Ramon Santamaria ([@raysan5](https://github.com/raysan5))_

-----------------------------------

## $(Game Title)

![$(Game Title)](screenshots/screenshot000.png "$(Game Title)")

### Description

$(Your Game Description)

### Features

 - $(Game Feature 01)
 - $(Game Feature 02)
 - $(Game Feature 03)

### Controls

Keyboard:
 - $(Game Control 01)
 - $(Game Control 02)
 - $(Game Control 03)

### Screenshots

_TODO: Show your game to the world, animated GIFs recommended!._

### Developers

 - $(Developer 01) - $(Role/Tasks Developed)
 - $(Developer 02) - $(Role/Tasks Developed)
 - $(Developer 03) - $(Role/Tasks Developed)

### Links

 - YouTube Gameplay: $(YouTube Link)
 - itch.io Release: $(itch.io Game Page)
 - Steam Release: $(Steam Game Page)

### License

This project sources are licensed under an unmodified zlib/libpng license, which is an OSI-certified, BSD-like license that allows static linking with closed source software. Check [LICENSE](LICENSE) for further details.

$(Additional Licenses)

*Copyright (c) $(Year) $(User Name) ($(User Twitter/GitHub Name))*
