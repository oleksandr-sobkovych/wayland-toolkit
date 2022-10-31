//! This code is based on several sources:
//! Wayland and EGL interaction: https://jan.newmarch.name/Wayland/
//! Font rendering: https://dthompson.us/font-rendering-in-opengl-with-pango-and-cairo.html
//! Cairo is used just for pango. Other drawing is done by OpenGL ES (GLES)
//! XDG interaction: https://gitlab.freedesktop.org/wayland/weston/blob/master/clients/simple-egl.c
//! Texture to screen: https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_01

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pango/pangocairo.h>
#include <linux/input.h>


struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface = NULL;
struct wl_egl_window *egl_window = NULL;
struct wl_region *region = NULL;
struct wl_shell *shell = NULL;
struct wl_shell_surface *shell_surface = NULL;
struct wl_callback *callback = NULL;

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

//! Old style -- without xdg
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
        wl_shell_surface_move(shell_surface,
                              seat, serial);

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
    fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n",
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
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                        const char *interface, uint32_t version)
{
    printf("Got a registry event for %s id %d\n", interface, id);
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry,
                                      id,
                                      &wl_compositor_interface,
                                      1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 1);

    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(registry, id,
                                &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
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

    // Create and compile the vertex shader
    GLuint vertexShader2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader2, 1, &vShaderStr, NULL);
    glCompileShader(vertexShader2);

    // Create and compile the fragment shader
    GLuint fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader2, 1, &fShaderStr, NULL);
    glCompileShader(fragmentShader2);

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
create_window(int width, int height) {
    egl_window = wl_egl_window_create(surface, width, height);
    if (egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl window\n");
    }

    egl_surface =
            eglCreateWindowSurface(egl_display,
            //eglCreatePlatformWindowSurface(egl_display,
                                   egl_conf,
                                   egl_window, NULL);


    if (eglMakeCurrent(egl_display, egl_surface,
                       egl_surface, egl_context)) {
        fprintf(stderr, "Made current\n");
    } else {
        fprintf(stderr, "Made current failed\n");
    }


    unsigned int texture_id;
    unsigned int text_width = 0;
    unsigned int text_height = 0;
    unsigned int text_pos_x = 20;
    unsigned int text_pos_y = height/2;

    render_text("Hello, world!",
                &text_width,
                &text_height,
                &texture_id);

    draw_text_gl(text_pos_x, text_pos_y,
                 text_width, text_height,
                 width, height
    );

    if (eglSwapBuffers(egl_display, egl_surface)) {
        fprintf(stderr, "Swapped buffers\n");
    } else {
        fprintf(stderr, "Swapped buffers failed\n");
    }
}

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

}

static void
get_server_references(void) {

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    printf("connected to display\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL || shell == NULL) {
        fprintf(stderr, "Can't find compositor or shell\n");
        exit(1);
    } else {
        fprintf(stderr, "Found compositor and shell\n");
    }
}

int main(int argc, char **argv) {

    get_server_references();

    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface\n");
        exit(1);
    } else {
        fprintf(stderr, "Created surface\n");
    }

    shell_surface = wl_shell_get_shell_surface(shell, surface);
    wl_shell_surface_set_toplevel(shell_surface);

    int win_size_x = 300;
    int win_size_y = 200;

    create_opaque_region(0, 0, win_size_x, win_size_y);
    init_egl();
    create_window(win_size_x, win_size_y);

    callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &configure_callback_listener,
                             NULL);

    while (is_running && wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}