// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

//! This code is based on several sources:
//! Wayland and EGL interaction: https://jan.newmarch.name/Wayland/
//! Font rendering: https://dthompson.us/font-rendering-in-opengl-with-pango-and-cairo.html
//! Cairo is used just for pango. Other drawing is done by OpenGL ES (GLES)
//! XDG interaction (they do like to call internal functions -- weston_platform_create_egl_surface amd so on):
//! https://gitlab.freedesktop.org/wayland/weston/blob/master/clients/simple-egl.c
//! XDG server side decorations: https://gitlab.freedesktop.org/wayland/weston/blob/master/clients/simple-dmabuf-egl.c
// and https://git.sr.ht/~sircmpwn/wlroots/tree/master/examples/toplevel-decoration.c
//! Texture to screen: https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_01
//! One more nice example, though no code was used from there:
//! https://github.com/libretro/RetroArch/blob/9b0bad5d9f1e20816e92e682e68418a4d1b4f03f/gfx/drivers_context/wayland_ctx.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <linux/input.h>
#include <signal.h>

#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

//! Генерується при компіляції:
// /usr/bin/wayland-scanner private-code //usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml src/xdg-shell-protocol.c
//  /usr/bin/wayland-scanner client-header //usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml src/xdg-shell-client-protocol.h
// Пошук шляху до wayland-scanner:
// pkg-config --variable=wayland_scanner  wayland-scanner
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1.h"

#include "hello_wayland_egl_draw.h"

// #define DEFAULT_EXIT_ON_ANY_KEY
const char window_title[] = "Hello-wayland";
const char window_message[] = "Hello, world!";

struct wl_registry *registry = NULL;

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface = NULL;
struct wl_egl_window *egl_window = NULL;
struct wl_region *region = NULL;
struct wl_callback *callback = NULL;

struct xdg_surface *xdg_surface;
struct xdg_toplevel *xdg_toplevel;
struct xdg_wm_base *xdg_shell;
int is_fullscreen, is_maximized;
int is_ctrl_pressed = 0;
bool wait_for_configure;
int needs_redraw = 1;

struct zxdg_decoration_manager_v1 *decoration_manager = NULL;
struct zxdg_toplevel_decoration_v1 *decoration;
enum zxdg_toplevel_decoration_v1_mode client_preferred_mode, current_mode;

PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage = NULL;

static const char *get_mode_name(enum zxdg_toplevel_decoration_v1_mode mode) {
    switch (mode) {
        case ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE:
            return "client-side decorations";
        case ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE:
            return "server-side decorations";
    }
    abort();
}
static void request_preferred_mode(void) {
    enum zxdg_toplevel_decoration_v1_mode mode = client_preferred_mode;
    if (mode == 0) {
        printf("Requesting compositor preferred mode\n");
        zxdg_toplevel_decoration_v1_unset_mode(decoration);
        return;
    }
    if (mode == current_mode) {
        return;
    }

    printf("Requesting %s\n", get_mode_name(mode));
    zxdg_toplevel_decoration_v1_set_mode(decoration, mode);
}

struct rect_t window_size = {300,200};

struct wl_seat *seat = NULL;
struct wl_pointer *pointer = NULL;
struct wl_keyboard *keyboard = NULL;

int is_running = 1; // Прапорець, чи не пора закінчувати

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t sx, wl_fixed_t sy)
{
    fprintf(stderr, "Pointer entered surface %p at %d %d\n", surface, sx, sy);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
    fprintf(stderr, "Pointer left surface %p\n", surface);
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    fprintf(stderr, "Pointer moved at %d %d\n", sx, sy);
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                      uint32_t serial, uint32_t time, uint32_t button,
                      uint32_t state)
{
    fprintf(stderr, "Pointer button: %x\n", button);

    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED){
        if(xdg_shell){
            xdg_toplevel_move(xdg_toplevel, seat, serial);
        }else{
            assert(NULL && "Unexpectedly no shell");
        }
    }
}

static void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    fprintf(stderr, "Pointer handle axis\n");
}

static const struct wl_pointer_listener pointer_listener = {
        pointer_handle_enter,
        pointer_handle_leave,
        pointer_handle_motion,
        pointer_handle_button,
        pointer_handle_axis,
};
// Інші події, що не використовуються у цій програмі, в такому порядку:
// pointer_handle_frame,    // Кінець блоку пов'язаних подій
// pointer_handle_axis_source,
// pointer_handle_axis_stop,
// pointer_handle_axis_discrete

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
    /* Just so we don’t leak the keymap fd */
    close(fd);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
    fprintf(stderr, "Keyboard gained focus\n");
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
    fprintf(stderr, "Keyboard lost focus\n");
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
    fprintf(stderr, "Key is %d state is %d\n", key, state);
//! Default hello exits on any key
#ifdef DEFAULT_EXIT_ON_ANY_KEY
    is_running = 0;
#else
    //! This is more interesting case:
    if( key==KEY_Q && is_ctrl_pressed && state == 0 ){
        is_running = 0;
    }
#endif
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
    is_ctrl_pressed = !!(mods_depressed && KEY_LEFTCTRL);
    fprintf(stderr, "Modifiers depressed %x, latched %x, locked %x, group %x\n",
            mods_depressed, mods_latched, mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
        keyboard_handle_keymap,
        keyboard_handle_enter,
        keyboard_handle_leave,
        keyboard_handle_key,
        keyboard_handle_modifiers,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        printf("Display has a pointer\n");
    }

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        printf("Display has a keyboard\n");
    }

    if (caps & WL_SEAT_CAPABILITY_TOUCH) {
        printf("Display has a touch screen\n");
    }

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer) {
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer) {
        wl_pointer_destroy(pointer);
        pointer = NULL;
    }

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wl_keyboard_destroy(keyboard);
        keyboard = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
        seat_handle_capabilities,
};

static void
handle_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                          int32_t width, int32_t height,
                          struct wl_array *states)
{
    printf("handle_toplevel_configure\n");
    uint32_t *p;

    is_fullscreen = 0;
    is_maximized = 0;
    wl_array_for_each(p, states) {
        uint32_t state = *p;
        switch (state) {
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                is_fullscreen = 1;
                break;
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                is_maximized = 1;
                break;
        }
    }

    if (width > 0 && height > 0) {
        window_size.width = width;
        window_size.height = height;
    }

    if (egl_window) {
        wl_egl_window_resize(egl_window,
                             window_size.width,
                             window_size.height, 0, 0);
        change_viewport(0, 0, window_size.width, window_size.height); // OpenGL calls are in separated file
    }
    needs_redraw = 1;
}

static void
handle_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
    is_running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
        handle_toplevel_configure,
        handle_toplevel_close,
};

static void
xdg_shell_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener xdg_shell_listener = {
        xdg_shell_ping,
};

static void
handle_surface_configure(void *data, struct xdg_surface *surface,
                         uint32_t serial)
{
    xdg_surface_ack_configure(surface, serial);
    wait_for_configure = false;
}

static const struct xdg_surface_listener xdg_surface_listener = {
        handle_surface_configure
};

static void decoration_handle_configure(void *data,
                                        struct zxdg_toplevel_decoration_v1 *decoration,
                                        enum zxdg_toplevel_decoration_v1_mode mode) {
    fprintf(stderr, "Using %s\n", get_mode_name(mode));
    current_mode = mode;
}

static const struct zxdg_toplevel_decoration_v1_listener decoration_listener = {
        .configure = decoration_handle_configure,
};


static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                        const char *interface, uint32_t version) {
    fprintf(stderr, "Got a registry event for %s id %d\n", interface, id);
    if (strcmp(interface, wl_compositor_interface.name) == 0) { //  "wl_compositor"
        compositor = wl_registry_bind(registry,
                                      id,
                                      &wl_compositor_interface,
                                      1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) { // "wl_seat"
        seat = wl_registry_bind(registry, id,
                                &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) { // "xdg_wm_base"
        xdg_shell = wl_registry_bind(registry, id,
                                     &xdg_wm_base_interface, version);
    } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        decoration_manager = wl_registry_bind(registry, id,
                                              &zxdg_decoration_manager_v1_interface, 1);
    }

    if (xdg_shell){
        xdg_wm_base_add_listener(xdg_shell, &xdg_shell_listener, NULL);
    }else{
        assert("No shell found.");
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    fprintf(stderr, "Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
        global_registry_handler,
        global_registry_remover
};

static void
redraw(void *data, struct wl_callback *callback, uint32_t time){
    fprintf(stderr, "Redrawing\n");
    draw_window(window_size, window_message);
}

static const struct wl_callback_listener frame_listener = {
        redraw
};

static void
configure_callback(void *data, struct wl_callback *callback, uint32_t  time)
{
    printf("configure_callback\n");
    if (callback == NULL)
        redraw(data, NULL, time);
}

static struct wl_callback_listener configure_callback_listener = {
        configure_callback,
};

static void
init_egl()
{
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    int i;
    EGLint config_attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, // API, які підтримуватимуться -- OpenGL ES 2.0
            EGL_NONE
    };

    static const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };


    egl_display = eglGetDisplay((EGLNativeDisplayType) display);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Can't create egl display\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl display\n");
    }

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE) {
        fprintf(stderr, "Can't initialise egl display\n");
        exit(1);
    }
    printf("EGL major: %d, minor %d\n", major, minor);

    eglGetConfigs(egl_display, NULL, 0, &count);
    printf("EGL has %d configs\n", count);

    configs = calloc(count, sizeof *configs);

    eglChooseConfig(egl_display, config_attribs,
                    configs, count, &n);

    for (i = 0; i < n; i++) {
        eglGetConfigAttrib(egl_display,
                           configs[i], EGL_BUFFER_SIZE, &size);
        printf("Buffer size for config %d is %d\n", i, size);
        eglGetConfigAttrib(egl_display,
                           configs[i], EGL_RED_SIZE, &size);
        printf("Red size for config %d is %d\n", i, size);

        // just choose the first one
        egl_conf = configs[i];
        break;
    }

    egl_context =
            eglCreateContext(egl_display,
                             egl_conf,
                             EGL_NO_CONTEXT, context_attribs);

    //! Без цих розширень, щоб одночасно працювали xdg i seat, доведеться постійно redraw викликати, займачюи CPU
    //! https://community.arm.com/developer/tools-software/graphics/b/blog/posts/mali-performance-3-is-egl_5f00_buffer_5f00_preserved-a-good-thing
    static const struct {
        char *extension, *entrypoint;
    } swap_damage_ext_to_entrypoint[] = {
            {
                    .extension = "EGL_EXT_swap_buffers_with_damage",
                    .entrypoint = "eglSwapBuffersWithDamageEXT",
            },
            {
                    .extension = "EGL_KHR_swap_buffers_with_damage",
                    .entrypoint = "eglSwapBuffersWithDamageKHR",
            },
    };
    static const size_t swap_damage_ext_to_entrypoint_size
                                    = sizeof(swap_damage_ext_to_entrypoint)/sizeof(swap_damage_ext_to_entrypoint[0]);


    const char *extensions = eglQueryString(egl_display, EGL_EXTENSIONS);
    //! Не відтестовано, так як під VirtualBox не працює.
    if (extensions &&
            ( strstr(extensions, "EGL_EXT_buffer_age") || strstr(extensions, "EGL_KHR_partial_update") )
            )
    {
        for (i = 0; i < swap_damage_ext_to_entrypoint_size; i++) {
            if (strstr(extensions, swap_damage_ext_to_entrypoint[i].extension)) {
                /* The EXTPROC is identical to the KHR one */
                swap_buffers_with_damage =
                        (PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC)
                                eglGetProcAddress(swap_damage_ext_to_entrypoint[i].entrypoint);
                break;
            }
        }
    }

    if (swap_buffers_with_damage)
        fprintf(stderr, "has EGL_EXT_buffer_age and EGL_???_swap_buffers_with_damage\n");
    else
        fprintf(stderr, "Not found EGL_EXT_buffer_age and EGL_???_swap_buffers_with_damage \n");
}

static void
deinit_egl()
{
    eglTerminate(egl_display);
    eglReleaseThread();
}

static void
swap_egl_surface()
{
    EGLint buffer_age = 0;
    EGLint rect[4];
    if (swap_buffers_with_damage)
        eglQuerySurface(egl_display, egl_surface,
                        EGL_BUFFER_AGE_EXT, &buffer_age);

    if (swap_buffers_with_damage && buffer_age > 0) {
        rect[0] = window_size.width / 4 - 1;
        rect[1] = window_size.height / 4 - 1;
        rect[2] = window_size.width / 2 + 2;
        rect[3] = window_size.height / 2 + 2;
        swap_buffers_with_damage(egl_display, egl_surface, rect, 1);
    } else {
        usleep(1);
        if (eglSwapBuffers(egl_display, egl_surface)) {
            fprintf(stderr, "Swapped buffers\n");
        } else {
            fprintf(stderr, "Swapped buffers failed\n");
        }
    }
}

static void
create_opaque_region(int x, int y, int w, int h) {
    region = wl_compositor_create_region(compositor);
    wl_region_add(region, x, y, w, h);
    wl_surface_set_opaque_region(surface, region);
}

static void
create_window()
{
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface\n");
        exit(1);
    } else {
        fprintf(stderr, "Created surface\n");
    }

    // create_opaque_region(0, 0, window_size.width, window_size.height);
    init_egl();
    egl_window = wl_egl_window_create(surface, window_size.width, window_size.height);
    if (egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl window\n");
    }
    egl_surface =
            eglCreatePlatformWindowSurface(egl_display, // Or eglCreateWindowSurface
                                           egl_conf,
                                           egl_window, NULL);
// Про можливі помилки -- див. https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglCreatePlatformWindowSurface.xhtml

    if (decoration_manager == NULL) {
        fprintf(stderr, "xdg-decoration not available\n");
    }

    if(xdg_shell) {
        xdg_surface = xdg_wm_base_get_xdg_surface(xdg_shell,
                                                  surface);

        xdg_surface_add_listener(xdg_surface,
                                 &xdg_surface_listener, NULL);

        xdg_toplevel =
                xdg_surface_get_toplevel(xdg_surface);
        xdg_toplevel_add_listener(xdg_toplevel,
                                  &xdg_toplevel_listener, NULL);

        xdg_toplevel_set_title(xdg_toplevel, window_title);

        if(decoration_manager) {
            decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
                    decoration_manager, xdg_toplevel);
            assert(decoration && "Failed to create decoration");
            zxdg_toplevel_decoration_v1_add_listener(decoration, &decoration_listener,
                                                     NULL);
        }
    }else{
        fprintf(stderr, "Failed to create shell\n");
        exit(1);
    }

    wait_for_configure = true;

    callback = wl_surface_frame(surface);
    wl_callback_add_listener(callback, &frame_listener, NULL);

//    callback = wl_display_sync(display);
//    wl_callback_add_listener(callback, &configure_callback_listener,
//                             NULL);

    wl_surface_commit(surface);

    if (eglMakeCurrent(egl_display, egl_surface,
                       egl_surface, egl_context)) {
        fprintf(stderr, "Made current\n");
    } else {
        fprintf(stderr, "Made current failed\n");
    }
}

static void
destroy_window()
{
    /* Required, otherwise segfault in egl_dri2.c: dri2_make_current()
     * on eglReleaseThread(). */
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    eglDestroySurface(egl_display, egl_surface);
    wl_egl_window_destroy(egl_window);

    if (xdg_toplevel)
        xdg_toplevel_destroy(xdg_toplevel);
    if (xdg_surface)
        xdg_surface_destroy(xdg_surface);
    if (xdg_shell)
        xdg_wm_base_destroy(xdg_shell);

    wl_surface_destroy(surface);

    if (callback)
        wl_callback_destroy(callback);
}

static void
get_server_references(void)
{
    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    fprintf(stderr, "connected to display\n");

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL ) {
        fprintf(stderr, "Can't find compositor \n");
        exit(1);
    } else {
        fprintf(stderr, "Found compositor \n");
    }
}

static void
signal_int(int signum)
{
    is_running = 0;
}

int main(int argc, char **argv) {
    if (argc == 2) {
        char *mode = argv[1];
        if (strcmp(mode, "client") == 0) {
            client_preferred_mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;
        } else if (strcmp(mode, "server") == 0) {
            client_preferred_mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
        } else {
            fprintf(stderr, "Invalid decoration mode\n");
            return EXIT_FAILURE;
        }
    }

    get_server_references();

    create_window();

    struct sigaction sigint;
    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);


    while (is_running && wl_display_dispatch(display) != -1) {
        if(needs_redraw){
            draw_window(window_size, window_message);
            swap_egl_surface();
            needs_redraw = 0;
        }
    }

    destroy_window();
    deinit_egl();

    if (compositor)
        wl_compositor_destroy(compositor);

    wl_registry_destroy(registry);
    wl_display_flush(display);

    wl_display_disconnect(display);
    fprintf(stderr, "disconnected from display\n");

    exit(0);
}
