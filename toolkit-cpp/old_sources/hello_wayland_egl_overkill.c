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
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <pango/pangocairo.h>
#include <linux/input.h>
#include <unistd.h>
#include <signal.h>

const char window_title[] = "Hello-wayland";
const char window_message[] = "Hello, world!";

//! Генерується при компіляції:
// /usr/bin/wayland-scanner private-code //usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml src/xdg-shell-protocol.c
//  /usr/bin/wayland-scanner client-header //usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml src/xdg-shell-client-protocol.h
// Пошук шляху до wayland-scanner:
// pkg-config --variable=wayland_scanner  wayland-scanner

#include "xdg-shell-client-protocol.h"
#include "xdg-shell-unstable-v6.h"
#include "xdg-decoration-unstable-v1.h"

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
int is_fullscreen, is_maximized, is_opaque, is_buffer_size;
bool wait_for_configure;
int need_redrawn = 1;

struct zxdg_surface_v6 *zxdg_surface;
struct zxdg_shell_v6 *zxdg_shell;
struct zxdg_toplevel_v6 *zxdg_toplevel;
struct zxdg_decoration_manager_v1 *decoration_manager = NULL;

PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage = NULL;

struct zxdg_toplevel_decoration_v1 *decoration;
enum zxdg_toplevel_decoration_v1_mode client_preferred_mode, current_mode;

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

struct rect_t {
    int width, height;
} window_size = {300,200};
struct rect_t geometry;


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
    printf("Pointer moved at %d %d\n", sx, sy);
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                      uint32_t serial, uint32_t time, uint32_t button,
                      uint32_t state)
{
    printf("Pointer button: %x\n", button);

    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED){
        if(zxdg_shell)
        {
            zxdg_toplevel_v6_move(zxdg_toplevel, seat, serial);
        }else if(xdg_shell){
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
    printf("Pointer handle axis\n");
}

static const struct wl_pointer_listener pointer_listener = {
        pointer_handle_enter,
        pointer_handle_leave,
        pointer_handle_motion,
        pointer_handle_button,
        pointer_handle_axis,
};

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

    // KEY_LEFTALT
    if( key==KEY_Q && state == 0 ){
        is_running = 0;
    }
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
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
        if (!is_fullscreen && !is_maximized) {
            window_size.width = width;
            window_size.height = height;
        }
        geometry.width = width;
        geometry.height = height;
    } else if (!is_fullscreen && !is_maximized) {
        geometry = window_size;
    }

    if (egl_window)
        wl_egl_window_resize(egl_window,
                             geometry.width,
                             geometry.height, 0, 0);

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

static void
zxdg_shell_ping(void *data, struct zxdg_shell_v6 *shell, uint32_t serial)
{
    zxdg_shell_v6_pong(shell, serial);
}

static const struct zxdg_shell_v6_listener zxdg_shell_listener = {
        zxdg_shell_ping,
};

void draw_window();

static void
zxdg_surface_handle_configure(void *data, struct zxdg_surface_v6 *surface,
                             uint32_t serial)
{
    zxdg_surface_v6_ack_configure(surface, serial);
    draw_window();
    wait_for_configure = false;
}

static const struct zxdg_surface_v6_listener zxdg_surface_listener = {
        zxdg_surface_handle_configure,
};

static void
zxdg_toplevel_handle_configure(void *data, struct zxdg_toplevel_v6 *toplevel,
                              int32_t width, int32_t height,
                              struct wl_array *states)
{
}

static void
zxdg_toplevel_handle_close(void *data, struct zxdg_toplevel_v6 *xdg_toplevel)
{
    is_running = 0;
}

static const struct zxdg_toplevel_v6_listener zxdg_toplevel_listener = {
        zxdg_toplevel_handle_configure,
        zxdg_toplevel_handle_close,
};

static void decoration_handle_configure(void *data,
                                        struct zxdg_toplevel_decoration_v1 *decoration,
                                        enum zxdg_toplevel_decoration_v1_mode mode) {
    printf("Using %s\n", get_mode_name(mode));
    current_mode = mode;
}

static const struct zxdg_toplevel_decoration_v1_listener decoration_listener = {
        .configure = decoration_handle_configure,
};


static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                        const char *interface, uint32_t version) {
    printf("Got a registry event for %s id %d\n", interface, id);
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry,
                                      id,
                                      &wl_compositor_interface,
                                      1);
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(registry, id,
                                &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, "zxdg_shell_v6") == 0) {
        zxdg_shell = wl_registry_bind(registry, id,
                                 &zxdg_shell_v6_interface, 1);
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg_shell = wl_registry_bind(registry, id,
                                     &xdg_wm_base_interface, version);
    } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        decoration_manager = wl_registry_bind(registry, id,
                                              &zxdg_decoration_manager_v1_interface, 1);
    }

    if(zxdg_shell){
        // zxdg_shell = NULL; return;
        zxdg_shell_v6_add_listener(zxdg_shell, &zxdg_shell_listener, NULL);
    }else if (xdg_shell){
        xdg_wm_base_add_listener(xdg_shell, &xdg_shell_listener, NULL);
    }else{
        assert("No shell found.");
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
        global_registry_handler,
        global_registry_remover
};



static void
create_opaque_region(int x, int y, int w, int h) {
    region = wl_compositor_create_region(compositor);
    wl_region_add(region, x, y, w, h);
    wl_surface_set_opaque_region(surface, region);
}

unsigned int
create_texture (unsigned int width,
                unsigned int height,
                unsigned char *pixels)
{
    unsigned int texture_id;

    glGenTextures (1, &texture_id);
    glBindTexture (GL_TEXTURE_2D, texture_id);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  width,
                  height,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  pixels);

    return texture_id;
}

// ШЕйдер вершин. Задає, де відображати.
const GLchar* vShaderStr =
        "#version 300 es                            \n"
        "layout(location = 0) in vec4 a_position;   \n"
        "layout(location = 1) in vec2 a_texCoord;   \n"
        "out vec2 v_texCoord;                       \n"
        "void main()                                \n"
        "{                                          \n"
        "   gl_Position = a_position;               \n"
        "   v_texCoord = vec2(a_texCoord.x, -a_texCoord.y); \n"
        "}                                          \n";

// ШЕйдер фрагментів. Задає колір.
const GLchar*  fShaderStr =
        "#version 300 es                                     \n"
        "precision mediump float;                            \n"
        "in vec2 v_texCoord;                                 \n"
        "layout(location = 0) out vec4 outColor;             \n"
        "uniform sampler2D s_texture;                        \n"
        "void main()                                         \n"
        "{                                                   \n"
        "  outColor = texture( s_texture, v_texCoord );      \n"
        "}                                                   \n";

static void
prepare_shaiders() {
    GLint status;
    // Create and compile the vertex shader
    GLuint vertexShader2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader2, 1, &vShaderStr, 0);
    glCompileShader(vertexShader2);
    glGetShaderiv(vertexShader2, GL_COMPILE_STATUS, &status);
    if ( status == GL_FALSE ) {
        char log[1000];
        GLsizei len;
        glGetShaderInfoLog(vertexShader2, sizeof(log), &len, log);
        fprintf(stderr, "Error: compiling vertex shader: %*s\n", len, log);
        exit(1);
    }
    // Create and compile the fragment shader
    GLuint fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader2, 1, &fShaderStr, 0);
    glCompileShader(fragmentShader2);
    glGetShaderiv(fragmentShader2, GL_COMPILE_STATUS, &status);
    if ( status == GL_FALSE ) {
        char log[1000];
        GLsizei len;
        glGetShaderInfoLog(fragmentShader2, sizeof(log), &len, log);
        fprintf(stderr, "Error: compiling fragment shader: %*s\n", len, log);
        exit(1);
    }

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram2 = glCreateProgram();
    glAttachShader(shaderProgram2, vertexShader2);
    glAttachShader(shaderProgram2, fragmentShader2);
    // glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram2);
    glUseProgram(shaderProgram2);
}


static void
draw_text_gl(unsigned int text_x,
             unsigned int text_y,
             unsigned int text_width,
             unsigned int text_height,
             unsigned int window_width,
             unsigned int window_height
             ){
    prepare_shaiders();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //! Координати OpenGL -- [-1, 1], початок -- центр, вісь x -- праворуч, y -- вверх (!)
    float step_per_coord_x = 2.0f/window_width;
    float step_per_coord_y = 2.0f/window_height;

    float left_x   = text_x * step_per_coord_x - 1;
    float right_x  = left_x + text_width * step_per_coord_x;
    float bottom_y = text_y  * step_per_coord_y  - 1;
    float top_y    = bottom_y + text_height * step_per_coord_y;

    float top_left_x = left_x;
    float top_left_y = top_y;
    float top_right_x = right_x;
    float top_right_y = top_y;

    float bottom_left_x = left_x;
    float bottom_left_y = bottom_y;
    float bottom_right_x = right_x;
    float bottom_right_y = bottom_y;

    glClearColor(0.85, 0.85, 0.85, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);


    GLfloat vVertices[] = { bottom_left_x, bottom_left_y, 0.0f,  // Position 0
                            0.0f,  0.0f,                         // TexCoord 0
                            top_left_x,  top_left_y, 0.0f,       // Position 1
                            0.0f   , 1.0f,                       // TexCoord 1
                            top_right_x, top_right_y, 0.0f,      // Position 2
                            1.0f,  1.0f,                         // TexCoord 2
                            bottom_right_x, bottom_right_y, 0.0f,// Position 3
                            1.0f,  0.0f,                         // TexCoord 3
    };

    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    // Load the vertex position
    glVertexAttribPointer ( 0, 3, GL_FLOAT,
                            GL_FALSE, 5 * sizeof ( GLfloat ), vVertices );
    // Load the texture coordinate
    glVertexAttribPointer ( 1, 2, GL_FLOAT,
                            GL_FALSE, 5 * sizeof ( GLfloat ), &vVertices[3] );

    glEnableVertexAttribArray ( 0 );
    glEnableVertexAttribArray ( 1 );
    // Bind the texture
    glActiveTexture ( GL_TEXTURE0 );
    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

    glFlush();
}


void
get_text_size (PangoLayout *layout,
               unsigned int *width,
               unsigned int *height)
{
    pango_layout_get_size (layout, width, height);
    /* Divide by pango scale to get dimensions in pixels. */
    *width /= PANGO_SCALE;
    *height /= PANGO_SCALE;
}

cairo_t*
create_layout_context ()
{
    cairo_surface_t *temp_surface;
    cairo_t *context;

    temp_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 0, 0);
    context = cairo_create (temp_surface);

    cairo_surface_destroy (temp_surface);

    return context;
}

cairo_t*
create_cairo_context (int width,
                      int height,
                      int channels,
                      cairo_surface_t** surf,
                      unsigned char** buffer)
{
    *buffer = calloc (channels * width * height, sizeof (unsigned char));
    *surf = cairo_image_surface_create_for_data (*buffer,
                                                 CAIRO_FORMAT_ARGB32,
                                                 width,
                                                 height,
                                                 channels * width);
    return cairo_create (*surf);
}

unsigned int
render_text (const char *text,
             unsigned int *text_width,
             unsigned int *text_height,
             unsigned int *texture_id)
{
    cairo_t *layout_context;
    cairo_t *render_context;
    cairo_surface_t *temp_surface;
    cairo_surface_t *surface;
    unsigned char* surface_data = NULL;
    PangoFontDescription *desc;
    PangoLayout *layout;

    layout_context = create_layout_context ();

    /* Create a PangoLayout, set the font and text */
    layout = pango_cairo_create_layout (layout_context);
    pango_layout_set_text (layout, text, -1);

    /* Load the font */
    desc = pango_font_description_from_string ("Arial 18");
    if(!desc){
        printf("Pango font not found.\n");
    }
    pango_layout_set_font_description (layout, desc);
    pango_font_description_free (desc);

    /* Get text dimensions and create a context to render to */
    get_text_size (layout, text_width, text_height);
    render_context = create_cairo_context (*text_width,
                                           *text_height,
                                           4,
                                           &surface,
                                           &surface_data);

    /* Render */
    cairo_set_source_rgba (render_context, 0, 0, 0, 1);
    pango_cairo_show_layout (render_context, layout);
    *texture_id = create_texture(*text_width, *text_height, surface_data);

    /* Clean up */
    free (surface_data);
    g_object_unref (layout);
    cairo_destroy (layout_context);
    cairo_destroy (render_context);
    cairo_surface_destroy (surface);
}

static void
redraw(void *data, struct wl_callback *callback, uint32_t time) {
    printf("Redrawing\n");
}

static const struct wl_callback_listener frame_listener = {
        redraw
};

static void
configure_callback(void *data, struct wl_callback *callback, uint32_t  time)
{
    if (callback == NULL)
        redraw(data, NULL, time);
}

static struct wl_callback_listener configure_callback_listener = {
        configure_callback,
};

static void
init_egl() {
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    int i;
    EGLint config_attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
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
        printf("has EGL_EXT_buffer_age and EGL_???_swap_buffers_with_damage\n");
    else
        printf("Not found EGL_EXT_buffer_age and EGL_???_swap_buffers_with_damage \n");
}

static void
deinit_egl() {
    eglTerminate(egl_display);
    eglReleaseThread();
}

static void
create_window() {
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

    if (decoration_manager == NULL) {
        fprintf(stderr, "xdg-decoration not available\n");
    }

    if(zxdg_shell) {
        zxdg_surface =
                zxdg_shell_v6_get_xdg_surface(zxdg_shell,
                                              surface);

        assert(zxdg_surface && "Failed to create zxdg_surface.");

        zxdg_surface_v6_add_listener(zxdg_surface,
                                     &zxdg_surface_listener, NULL);

        zxdg_toplevel =
                zxdg_surface_v6_get_toplevel(zxdg_surface);

        assert(zxdg_toplevel && "Failed to create zxdg_toplevel surface.");

        zxdg_toplevel_v6_add_listener(zxdg_toplevel,
                                      &zxdg_toplevel_listener, NULL);

        zxdg_toplevel_v6_set_title(zxdg_toplevel, window_title);

        if(decoration_manager) {
            /*
            //! Воно ніби zxdg, але хоче xdg...
            if(xdg_shell) {
                xdg_surface = xdg_wm_base_get_xdg_surface(xdg_shell,
                                                          surface);
                xdg_toplevel =
                        xdg_surface_get_toplevel(xdg_surface);
            }
             */

            //! А якщо KDE напряму, то було б щось типу: org_kde_kwin_server_decoration_manager_create
            decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
                    decoration_manager, zxdg_toplevel );
            assert(decoration && "Failed to create decoration");
            zxdg_toplevel_decoration_v1_add_listener(decoration, &decoration_listener,
                                                     NULL);
        }

    } else if(xdg_shell) {
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
        printf("Failed to create shell\n");
        exit(1);
    }
    wait_for_configure = true;
    wl_surface_commit(surface);

    if (eglMakeCurrent(egl_display, egl_surface,
                       egl_surface, egl_context)) {
        fprintf(stderr, "Made current\n");
    } else {
        fprintf(stderr, "Made current failed\n");
    }


}

void draw_window(){
    unsigned int texture_id;
    unsigned int text_width = 0;
    unsigned int text_height = 0;
    unsigned int text_pos_x = 20;
    unsigned int text_pos_y = window_size.height/2;

    render_text(window_message,
                &text_width,
                &text_height,
                &texture_id);

    draw_text_gl(text_pos_x, text_pos_y,
                 text_width, text_height,
                 window_size.width, window_size.height
    );

    EGLint buffer_age = 0;
    EGLint rect[4];
    if (swap_buffers_with_damage)
        eglQuerySurface(egl_display, egl_surface,
                        EGL_BUFFER_AGE_EXT, &buffer_age);

    if (swap_buffers_with_damage && buffer_age > 0) {
        rect[0] = geometry.width / 4 - 1;
        rect[1] = geometry.height / 4 - 1;
        rect[2] = geometry.width / 2 + 2;
        rect[3] = geometry.height / 2 + 2;
        swap_buffers_with_damage(egl_display, egl_surface, rect, 1);
    } else {
        usleep(10000);
        if (eglSwapBuffers(egl_display, egl_surface)) {
            fprintf(stderr, "Swapped buffers\n");
        } else {
            fprintf(stderr, "Swapped buffers failed\n");
        }
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
    if (zxdg_shell)
        zxdg_shell_v6_destroy(zxdg_shell);
    if (xdg_shell)
        xdg_wm_base_destroy(xdg_shell);

    wl_surface_destroy(surface);



    if (callback)
        wl_callback_destroy(callback);
}


static void
get_server_references(void) {

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    printf("connected to display\n");

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

    client_preferred_mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;

    get_server_references();

    create_window();

    callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &configure_callback_listener,
                             NULL);

    struct sigaction sigint;
    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);


    int dispatch_ret = 0;
    while (is_running && dispatch_ret != -1) {
        if (wait_for_configure) {
            dispatch_ret = wl_display_dispatch(display);
        } else {
            dispatch_ret = wl_display_dispatch_pending(display);
            draw_window();
        }
    }

    destroy_window();
    deinit_egl();

    if (compositor)
        wl_compositor_destroy(compositor);

    wl_registry_destroy(registry);
    wl_display_flush(display);

    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}