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
include CMakeFiles/gui_wayland_hello.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/gui_wayland_hello.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/gui_wayland_hello.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/gui_wayland_hello.dir/flags.make

CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o: CMakeFiles/gui_wayland_hello.dir/flags.make
CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o: /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland.c
CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o: CMakeFiles/gui_wayland_hello.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o -MF CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o.d -o CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o -c /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland.c

CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.i"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland.c > CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.i

CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.s"
	/usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/hello_wayland.c -o CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.s

# Object files for target gui_wayland_hello
gui_wayland_hello_OBJECTS = \
"CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o"

# External object files for target gui_wayland_hello
gui_wayland_hello_EXTERNAL_OBJECTS =

gui_wayland_hello: CMakeFiles/gui_wayland_hello.dir/hello_wayland.c.o
gui_wayland_hello: CMakeFiles/gui_wayland_hello.dir/build.make
gui_wayland_hello: /usr/lib/libwayland-client.so
gui_wayland_hello: /usr/lib/libcairo.so
gui_wayland_hello: /usr/lib/libEGL.so
gui_wayland_hello: /usr/lib/libwayland-egl.so
gui_wayland_hello: /usr/lib/libOpenGL.so
gui_wayland_hello: /usr/lib/libGLX.so
gui_wayland_hello: /usr/lib/libGLU.so
gui_wayland_hello: /usr/lib/libpango-1.0.so
gui_wayland_hello: CMakeFiles/gui_wayland_hello.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable gui_wayland_hello"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gui_wayland_hello.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/gui_wayland_hello.dir/build: gui_wayland_hello
.PHONY : CMakeFiles/gui_wayland_hello.dir/build

CMakeFiles/gui_wayland_hello.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/gui_wayland_hello.dir/cmake_clean.cmake
.PHONY : CMakeFiles/gui_wayland_hello.dir/clean

CMakeFiles/gui_wayland_hello.dir/depend:
	cd /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build /home/osobkovych/Documents/Study/UCU/Course_3-1/OS/Project/defense_1/gui_wayland/hello_wayland/build/CMakeFiles/gui_wayland_hello.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/gui_wayland_hello.dir/depend
