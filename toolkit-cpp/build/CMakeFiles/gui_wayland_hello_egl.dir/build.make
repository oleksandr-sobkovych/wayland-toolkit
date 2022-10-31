# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.24

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build

# Include any dependencies generated for this target.
include CMakeFiles/gui_wayland_hello_egl.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/gui_wayland_hello_egl.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/gui_wayland_hello_egl.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/gui_wayland_hello_egl.dir/flags.make

xdg-shell-client-protocol.c:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating xdg-shell-client-protocol.c"
	/usr/bin/wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-shell-client-protocol.c

xdg-shell-client-protocol.h:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Generating xdg-shell-client-protocol.h"
	/usr/bin/wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-shell-client-protocol.h

xdg-decoration-unstable-v1.h:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Generating xdg-decoration-unstable-v1.h"
	/usr/bin/wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-decoration-unstable-v1.h

xdg-decoration-unstable-v1.c:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Generating xdg-decoration-unstable-v1.c"
	/usr/bin/wayland-scanner private-code /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-decoration-unstable-v1.c

CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o: CMakeFiles/gui_wayland_hello_egl.dir/flags.make
CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o: /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl.c
CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o: CMakeFiles/gui_wayland_hello_egl.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o -MF CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o.d -o CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o -c /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl.c

CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.i"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl.c > CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.i

CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.s"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl.c -o CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.s

CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o: CMakeFiles/gui_wayland_hello_egl.dir/flags.make
CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o: xdg-shell-client-protocol.c
CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o: CMakeFiles/gui_wayland_hello_egl.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o -MF CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o.d -o CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o -c /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-shell-client-protocol.c

CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.i"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-shell-client-protocol.c > CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.i

CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.s"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-shell-client-protocol.c -o CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.s

CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o: CMakeFiles/gui_wayland_hello_egl.dir/flags.make
CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o: xdg-decoration-unstable-v1.c
CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o: CMakeFiles/gui_wayland_hello_egl.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o -MF CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o.d -o CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o -c /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-decoration-unstable-v1.c

CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.i"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-decoration-unstable-v1.c > CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.i

CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.s"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/xdg-decoration-unstable-v1.c -o CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.s

CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o: CMakeFiles/gui_wayland_hello_egl.dir/flags.make
CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o: /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl_draw.c
CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o: CMakeFiles/gui_wayland_hello_egl.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o -MF CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o.d -o CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o -c /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl_draw.c

CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.i"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl_draw.c > CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.i

CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.s"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland_egl_draw.c -o CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.s

# Object files for target gui_wayland_hello_egl
gui_wayland_hello_egl_OBJECTS = \
"CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o" \
"CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o" \
"CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o" \
"CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o"

# External object files for target gui_wayland_hello_egl
gui_wayland_hello_egl_EXTERNAL_OBJECTS =

gui_wayland_hello_egl: CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl.c.o
gui_wayland_hello_egl: CMakeFiles/gui_wayland_hello_egl.dir/xdg-shell-client-protocol.c.o
gui_wayland_hello_egl: CMakeFiles/gui_wayland_hello_egl.dir/xdg-decoration-unstable-v1.c.o
gui_wayland_hello_egl: CMakeFiles/gui_wayland_hello_egl.dir/hello_wayland_egl_draw.c.o
gui_wayland_hello_egl: CMakeFiles/gui_wayland_hello_egl.dir/build.make
gui_wayland_hello_egl: /usr/lib/libwayland-client.so
gui_wayland_hello_egl: /usr/lib/libcairo.so
gui_wayland_hello_egl: /usr/lib/libEGL.so
gui_wayland_hello_egl: /usr/lib/libwayland-egl.so
gui_wayland_hello_egl: /usr/lib/libOpenGL.so
gui_wayland_hello_egl: /usr/lib/libGLX.so
gui_wayland_hello_egl: /usr/lib/libGLU.so
gui_wayland_hello_egl: /usr/lib/libpango-1.0.so
gui_wayland_hello_egl: CMakeFiles/gui_wayland_hello_egl.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Linking C executable gui_wayland_hello_egl"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gui_wayland_hello_egl.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/gui_wayland_hello_egl.dir/build: gui_wayland_hello_egl
.PHONY : CMakeFiles/gui_wayland_hello_egl.dir/build

CMakeFiles/gui_wayland_hello_egl.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/gui_wayland_hello_egl.dir/cmake_clean.cmake
.PHONY : CMakeFiles/gui_wayland_hello_egl.dir/clean

CMakeFiles/gui_wayland_hello_egl.dir/depend: xdg-decoration-unstable-v1.c
CMakeFiles/gui_wayland_hello_egl.dir/depend: xdg-decoration-unstable-v1.h
CMakeFiles/gui_wayland_hello_egl.dir/depend: xdg-shell-client-protocol.c
CMakeFiles/gui_wayland_hello_egl.dir/depend: xdg-shell-client-protocol.h
	cd /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles/gui_wayland_hello_egl.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/gui_wayland_hello_egl.dir/depend
