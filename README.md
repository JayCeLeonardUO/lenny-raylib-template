-----------------------------------
_DISCLAIMER:_

Welcome to the **lenny-raylib-template**!

This template provides a base structure to start developing a small raylib game in plain C. The repo is also pre-configured with a default `LICENSE` (zlib/libpng) and a `README.md` (this one) to be properly filled by users. Feel free to change the LICENSE as required.

All the sections defined by `$(Data to Fill)` are expected to be edited and filled properly. It's recommended to delete this disclaimer message after editing this `README.md` file.

-----------------------------------

## Getting Started with this template

### Neovim

A project-local `.nvim.lua` is included and loads automatically when launching nvim from the repo root (requires `vim.o.exrc = true` in your init.lua). It sets `makeprg` to `make -C src`, adds a `:CMakeBuild` command (which also generates `compile_commands.json` for clangd), and maps `<leader>mm` to build and `<leader>mr` to run.

### Linux

When setting up this template on linux for the first time, install the dependencies from this page:
([Working on GNU Linux](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux))

This repository comes with CMake and a plain Makefile already set up.

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
