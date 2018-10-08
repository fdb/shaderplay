#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <sys/stat.h>


long file_modification_time(const char*filename) {
    struct stat buf;
    if (stat(filename, &buf) != -1) {
        return (long)buf.st_mtime;
    } else {
        return 0;
    }
}

void ngl_check_gl_error(const char *file, int line) {
    GLenum err = glGetError();
    int has_error = 0;
    while (err != GL_NO_ERROR) {
        has_error = 1;
        char *msg = NULL;
        switch(err) {
            case GL_INVALID_OPERATION:
            msg = "GL_INVALID_OPERATION";
            break;
            case GL_INVALID_ENUM:
            msg = "GL_INVALID_ENUM";
            fprintf(stderr, "OpenGL error: GL_INVALID_ENUM\n");
            break;
            case GL_INVALID_VALUE:
            msg = "GL_INVALID_VALUE";
            fprintf(stderr, "OpenGL error: GL_INVALID_VALUE\n");
            break;
            case GL_OUT_OF_MEMORY:
            msg = "GL_OUT_OF_MEMORY";
            fprintf(stderr, "OpenGL error: GL_OUT_OF_MEMORY\n");
            break;
            default:
            msg = "UNKNOWN_ERROR";
        }
        fprintf(stderr, "OpenGL error: %s - %s:%d\n", msg, file, line);
        err = glGetError();
    }
    if (has_error) {
        exit(-1);
    }
}

void ngl_check_compile_error(GLuint shader) {
    const int LOG_MAX_LENGTH = 2048;
    GLint status = -1;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char infoLog[LOG_MAX_LENGTH];
        glGetShaderInfoLog(shader, LOG_MAX_LENGTH, NULL, infoLog);
        fprintf(stderr, "Shader %d compile error: %s\n", shader, infoLog);
        exit(EXIT_FAILURE);
    }
}

void ngl_check_link_error(GLuint program) {
    const int LOG_MAX_LENGTH = 2048;
    GLint status = -1;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char infoLog[LOG_MAX_LENGTH];
        glGetProgramInfoLog(program, LOG_MAX_LENGTH, NULL, infoLog);
        fprintf(stderr, "Shader link error: %s\n", infoLog);
        exit(EXIT_FAILURE);
    }
}


char *nfile_read(const char* fname) {
    FILE *fp = fopen(fname, "rb");
    if (!fp) {
        perror(fname);
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // Allocate memory for entire content
    char *buffer = (char*) calloc(1, size + 1);
    if (!buffer) {
        fclose(fp);
        fputs("ERR: nfile_read: failed to allocate memory.", stderr);
        exit(EXIT_FAILURE);
    }

    // Copy the file into the buffer
    if (fread(buffer, size, 1, fp) != 1) {
        fclose(fp);
        free(buffer);
        fputs("ERR: nfile_read: failed to read file.", stderr);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    return buffer;
}

#define NGL_CHECK_ERROR() ngl_check_gl_error(__FILE__, __LINE__)

GLint u_time, u_resolution, u_mouse;
GLint g_program = 0;
GLuint g_vertex_shader = 0;
GLuint g_fragment_shader = 0;

void compile_shader_program() {
  if (g_program != 0) {
    glDetachShader(g_program, g_vertex_shader);
    glDeleteShader(g_vertex_shader);
    glDetachShader(g_program, g_fragment_shader);
    glDeleteShader(g_fragment_shader);
    glDeleteProgram(g_program);
  }


  char* vertex_shader_source =
    "#version 150\n"
    "in vec2 in_position;\n"
    "void main() {\n"
    "  gl_Position = vec4(in_position, 0, 1);\n"
    "}\n";

  g_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(g_vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(g_vertex_shader);
  ngl_check_compile_error(g_vertex_shader);
  NGL_CHECK_ERROR();

  char* fragment_shader_source = nfile_read("default.frag");
    // "#version 150\n"
    // "uniform float u_time;\n"
    // "uniform vec2 u_resolution;\n"
    // "out vec4 out_color;\n"
    // "void main() {\n"
    // "  vec2 uv = gl_FragCoord.xy / u_resolution.xy;\n"
    // "  out_color = vec4(sin(uv.x * 80.0 + u_time * 5.0) * 2.0, cos(uv.y + u_time * 2.0), 1.0, 1.0);\n"
    // "}\n";

  g_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(g_fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(g_fragment_shader);
  ngl_check_compile_error(g_fragment_shader);
  NGL_CHECK_ERROR();

  g_program = glCreateProgram();
  glAttachShader(g_program, g_vertex_shader);
  glAttachShader(g_program, g_fragment_shader);
  glLinkProgram(g_program);
  ngl_check_link_error(g_program);
  NGL_CHECK_ERROR();

  u_time = glGetUniformLocation(g_program, "u_time");
  // assert(u_time >= 0);

  u_resolution = glGetUniformLocation(g_program, "u_resolution");
  // assert(u_resolution >= 0);

  u_mouse = glGetUniformLocation(g_program, "u_mouse");
  // assert(u_mouse >= 0);
}

int main(void) {
  GLFWwindow *window;

  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(640, 480, "gltest", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);


  int vertex_count = 6;
  int component_count = 2;
  float positions[] = { -1, 1, 1, 1, -1, -1, 1, 1, 1, -1, -1, -1 };
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * component_count * vertex_count, positions, GL_STATIC_DRAW);
  NGL_CHECK_ERROR();

  GLuint vao;
  glGenVertexArrays(1, &vao);
  NGL_CHECK_ERROR();
  glBindVertexArray(vao);
  NGL_CHECK_ERROR();
  glEnableVertexAttribArray(0);
  NGL_CHECK_ERROR();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  NGL_CHECK_ERROR();
  glVertexAttribPointer(0, component_count, GL_FLOAT, GL_FALSE, 0, 0);
  NGL_CHECK_ERROR();

  compile_shader_program();

  long shader_modification_time = file_modification_time("default.frag");

  double time = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
     long new_modification_time = file_modification_time("default.frag");
     if (shader_modification_time != new_modification_time) {
      shader_modification_time = new_modification_time;
      printf("MODIFIED\n");
      compile_shader_program();
     }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    int win_width, win_height;
    glfwGetWindowSize(window, &win_width, &win_height);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float mouse_x = (float) xpos;
    float mouse_y = (float) ypos;
    //printf("%.2f %.2f\n", mouse_x, mouse_y );

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    NGL_CHECK_ERROR();

    glUseProgram(g_program);
    NGL_CHECK_ERROR();
    glUniform1f(u_time, glfwGetTime());
    glUniform2f(u_resolution, width, height);
    glUniform2f(u_mouse, mouse_x, mouse_y);

    glBindVertexArray(vao);
    NGL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    NGL_CHECK_ERROR();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
