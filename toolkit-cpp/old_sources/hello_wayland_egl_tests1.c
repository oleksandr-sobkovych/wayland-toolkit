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


struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;


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
create_opaque_region() {
    region = wl_compositor_create_region(compositor);
    wl_region_add(region, 0, 0,
                  480,
                  360);
    wl_surface_set_opaque_region(surface, region);
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
    cairo_set_source_rgba (render_context, 1, 1, 1, 1);
    pango_cairo_show_layout (render_context, layout);
    *texture_id = create_texture(*text_width, *text_height, surface_data);

    /* Clean up */
    free (surface_data);
    //g_object_unref (layout);
    cairo_destroy (layout_context);
    cairo_destroy (render_context);
    cairo_surface_destroy (surface);
}

// Shader sources
const GLchar* vertexSource =
        "attribute vec4 position;    \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = vec4(position.xyz, 1.0);  \n"
        "}                            \n";
const GLchar* fragmentSource =
        "precision mediump float;\n"
        "void main()                                  \n"
        "{                                            \n"
        "  gl_FragColor = vec4 (1.0, 1.0, 1.0, 1.0 );\n"
        "}                                            \n";

const GLchar* vShaderStr =
        "#version 300 es                            \n"
        "layout(location = 0) in vec4 a_position;   \n"
        "layout(location = 1) in vec2 a_texCoord;   \n"
        "out vec2 v_texCoord;                       \n"
        "void main()                                \n"
        "{                                          \n"
        "   gl_Position = a_position;               \n"
        "   v_texCoord = a_texCoord;                \n"
        "}                                          \n";

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
create_window() {

    egl_window = wl_egl_window_create(surface,
                                      480, 360);
    if (egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl window\n");
    }

    egl_surface =
            eglCreateWindowSurface(egl_display,
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

#if 0
    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLfloat vertices[] = {0.0f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f};

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    // glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    // Clear the screen to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);



    // Draw a triangle from the 3 vertices
    glDrawArrays(GL_TRIANGLES, 0, 3);
#endif
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


    render_text("Hello, world!",
                &texture_id,
                &text_width,
                &text_height);

    printf("Text w: %i, h: %i\n", text_width, text_height);
/*
    GLfloat vVertices[] = { -0.5f,  0.5f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0
                            -0.5f, -0.5f, 0.0f,  // Position 1
                            0.0f, 1,        // TexCoord 1
                            0.5f, -0.5f, 0.0f,  // Position 2
                            1,  1,        // TexCoord 2
                            0.5f,  0.5f, 0.0f,  // Position 3
                            1   , 0.0f,           // TexCoord 3
    };
*/
    GLfloat vVertices[] = { 0.0f,  0.5f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0
                            0.0f, 0.0f, 0.0f,  // Position 1
                            0.0f, 1,        // TexCoord 1
                            0.5f, 0.0f, 0.0f,  // Position 2
                            1,  1,        // TexCoord 2
                            0.5f,  0.5f, 0.0f,  // Position 3
                            1   , 0.0f,           // TexCoord 3
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

    // Get the sampler location
    GLint samplerLoc = glGetUniformLocation ( shaderProgram2, "s_texture" );
    glUniform1i ( samplerLoc, 0 );

    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

    glFlush();

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

    create_opaque_region();
    init_egl();
    create_window();

    while (wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}