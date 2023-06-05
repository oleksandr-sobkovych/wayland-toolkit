// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdio.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>


#include <pango/pangocairo.h>

#include "hello_wayland_egl_draw.h"

void change_viewport(int dx, int dy, int width, int height){
    glViewport(0, 0, width, height);
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

// Шейдер вершин. Задає, де відображати.
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

static void
get_text_size (PangoLayout *layout,
               unsigned int *width,
               unsigned int *height)
{
    pango_layout_get_size (layout, width, height);
    /* Divide by pango scale to get dimensions in pixels. */
    *width /= PANGO_SCALE;
    *height /= PANGO_SCALE;
}

static cairo_t*
create_layout_context ()
{
    cairo_surface_t *temp_surface;
    cairo_t *context;

    temp_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 0, 0);
    context = cairo_create (temp_surface);

    cairo_surface_destroy (temp_surface);

    return context;
}

static cairo_t*
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

void draw_window(struct rect_t window_size, const char* window_message)
{
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

}
