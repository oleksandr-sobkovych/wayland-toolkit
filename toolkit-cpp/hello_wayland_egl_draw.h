#ifndef WAYLAND_HELLO_HELLO_WAYLAND_EGL_DRAW_H
#define WAYLAND_HELLO_HELLO_WAYLAND_EGL_DRAW_H

struct rect_t {
    int width, height;
};

void change_viewport(int dx, int dy, int width, int height);
void draw_window( struct rect_t window_size, const char* window_message);

#endif //WAYLAND_HELLO_HELLO_WAYLAND_EGL_DRAW_H
