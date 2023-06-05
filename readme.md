# 
Authors (team): [Oleksandr Sobkovych](https://github.com/oleksandr-sobkovych)<br>

## Prerequisites

- gcc or clang
- cmake
- make
- Wayland compositor with xdg-shell extension support
- OpenGL libraries and devices
- wayland development libraries
- cairo library
- pango library
- harfbuzz library
- glib library

### Compilation

```bash
mkdir build
cd build
cmake .. # optionally select non-default toolchain
cmake --build .
```

### Usage

Test the toolkit with the provided application:
```bash
./build/test_app
```
