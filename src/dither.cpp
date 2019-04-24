#include <algorithm>
#include <random>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define DJ_OPENGL_IMPLEMENTATION 1
#include "dj_opengl.h"

#define WINDOW_SIZE 512
#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);

enum {
    PROGRAM_MIN,
    PROGRAM_INSERT,
    PROGRAM_RENDER,
    PROGRAM_RESET,
    PROGRAM_COUNT
};

enum {
    TEXTURE_DITHER,
    TEXTURE_DENSITY,
    TEXTURE_COUNT
};

struct {
    GLuint textures[TEXTURE_COUNT];
    GLuint programs[PROGRAM_COUNT];
    GLuint buffer;
    GLuint vertexArray;
    GLuint framebuffer;
} g_gl = {0};

#ifndef PATH_TO_SRC_DIRECTORY
#   define PATH_TO_SRC_DIRECTORY "./"
#endif

////////////////////////////////////////////////////////////////////////////////
// Utility functions
//
////////////////////////////////////////////////////////////////////////////////

char *strcat2(char *dst, const char *src1, const char *src2)
{
    strcpy(dst, src1);

    return strcat(dst, src2);
}

int findMSB(int x)
{
    int p = 0;

    while (x > 1) {
        x>>= 1;
        ++p;
    }

    return p;
}

int nextPot(int x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}

////////////////////////////////////////////////////////////////////////////////
// OpenGL Resource Loading
//
////////////////////////////////////////////////////////////////////////////////

void initTextures(int w, int h)
{
    std::vector<float> texelData(w * h, 0.f);

    glGenTextures(TEXTURE_COUNT, g_gl.textures);

    for (int i = 0; i < TEXTURE_COUNT; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, g_gl.textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0,
                     GL_RED, GL_FLOAT, &texelData[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

void initFramebuffer()
{
    glGenFramebuffers(1, &g_gl.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        g_gl.textures[TEXTURE_DENSITY],
        0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        LOG("=> Failure <=\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initBuffer(int w, int h, int w2, int h2)
{
    int cnt = w2 * h2;
    struct ivec4 {int x, y, z, w;};
    std::vector<ivec4> bufferData(2 * cnt);

    // set data at last MIP
    for (int j = 0; j < h2; ++j) {
        for (int i = 0; i < w2; ++i) {
            bufferData[cnt + i + w2 * j].x = i;
            bufferData[cnt + i + w2 * j].y = j;
            bufferData[cnt + i + w2 * j].z = cnt + i + w2 * j;
            bufferData[cnt + i + w2 * j].w = (i < w && j < h) ? 0 : 1;
        }
    }

    bufferData[1].z = cnt;
    bufferData[1].w = 1;

    glGenBuffers(1, &g_gl.buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_gl.buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(bufferData[0]) * bufferData.size(),
                 &bufferData[0],
                 GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_gl.buffer);
}

void initVertexArray()
{
    glGenVertexArrays(1, &g_gl.vertexArray);
    glBindVertexArray(g_gl.vertexArray);
    glBindVertexArray(0);
}

void initMinProgram()
{
    djg_program *djp = djgp_create();
    GLuint *program = &g_gl.programs[PROGRAM_MIN];

    LOG("Loading {Min-Program}\n");
    djgp_push_file(djp, PATH_TO_SRC_DIRECTORY "shaders/min.glsl");

    if (!djgp_to_gl(djp, 450, false, true, program)) {
        LOG("=> Failure <=\n");
    }
    djgp_release(djp);


    glProgramUniform1i(
        g_gl.programs[PROGRAM_MIN],
        glGetUniformLocation(g_gl.programs[PROGRAM_MIN], "u_DensitySampler"),
        TEXTURE_DENSITY
    );
}

void initInsertProgram(int w, int h)
{
    djg_program *djp = djgp_create();
    GLuint *program = &g_gl.programs[PROGRAM_INSERT];

    LOG("Loading {Insert-Program}\n");
    djgp_push_file(djp, PATH_TO_SRC_DIRECTORY "shaders/insert.glsl");

    if (!djgp_to_gl(djp, 450, false, true, program)) {
        LOG("=> Failure <=\n");
    }
    djgp_release(djp);

    glProgramUniform2i(
        g_gl.programs[PROGRAM_INSERT],
        glGetUniformLocation(g_gl.programs[PROGRAM_INSERT], "u_ImageResolution"),
        w, h
    );
}

void initRenderProgram()
{
    djg_program *djp = djgp_create();
    GLuint *program = &g_gl.programs[PROGRAM_RENDER];

    LOG("Loading {Render-Program}\n");
    djgp_push_file(djp, PATH_TO_SRC_DIRECTORY "shaders/render.glsl");

    if (!djgp_to_gl(djp, 450, false, true, program)) {
        LOG("=> Failure <=\n");
    }
    djgp_release(djp);

    glProgramUniform1i(
        g_gl.programs[PROGRAM_RENDER],
        glGetUniformLocation(g_gl.programs[PROGRAM_RENDER], "u_DitherSampler"),
        TEXTURE_DITHER
    );
}

void init(int w, int h, int w2, int h2)
{
    initTextures(w2, h2);
    initInsertProgram(w, h);
    initMinProgram();
    initRenderProgram();
    initVertexArray();
    initFramebuffer();
    initBuffer(w, h, w2, h2);
}


////////////////////////////////////////////////////////////////////////////////
// Computations
//
////////////////////////////////////////////////////////////////////////////////

// compute the minimum value in a custom MIP map fashion
void findMin(int w, int h)
{
    struct { int passID, randomOffset;} location = {
        glGetUniformLocation(g_gl.programs[PROGRAM_MIN], "u_PassID"),
        glGetUniformLocation(g_gl.programs[PROGRAM_MIN], "u_RandomOffset")
    };
    int it = findMSB(w * h);

    glUseProgram(g_gl.programs[PROGRAM_MIN]);

    while (--it >= 0) {
        glUniform1ui(location.passID, it);
        glUniform1ui(location.randomOffset, rand());
        glDispatchCompute(1 << it, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
}

// single pass of dither mask generation
void computeDitherOnce(int w, int h, int w2, int h2, float value)
{
    // splat density
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl.framebuffer);
    glViewport(0, 0, w, h);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBindImageTexture(0, g_gl.textures[TEXTURE_DITHER], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    glUseProgram(g_gl.programs[PROGRAM_INSERT]);
    glUniform1f(glGetUniformLocation(g_gl.programs[PROGRAM_INSERT], "u_Value"),
                value);
    glBindVertexArray(g_gl.vertexArray);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // syncs imageStore
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // buildMipmap
    findMin(w2, h2);

}

////////////////////////////////////////////////////////////////////////////////
// Rendering
//
////////////////////////////////////////////////////////////////////////////////

void render(int w, int h, float value)
{
    glViewport(0, 0, WINDOW_SIZE, WINDOW_SIZE * h / w);
    glUseProgram(g_gl.programs[PROGRAM_RENDER]);
    glUniform1f(glGetUniformLocation(g_gl.programs[PROGRAM_RENDER], "u_Offset"),
                1.0f - value);
    glBindVertexArray(g_gl.vertexArray);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


////////////////////////////////////////////////////////////////////////////////
// main
//
////////////////////////////////////////////////////////////////////////////////

void usage(const char *app)
{
    printf("%s -- Blue-Noise Dither Mask Tile Generator\n", app);
    printf("usage: %s\n", app);
    printf("options:\n"
           "    --help\n"
           "        Print help\n"
           "\n"
           "    --res x y\n"
           "        Specifies the mask resolution (default is 256 256)\n"
           "\n"
           "    --seed value\n"
           "        Specifies the seed of the random generator\n");
}

int main(int argc, char **argv)
{
    struct {
        int seed;
        struct {int x, y;} res;
        struct {int x, y;} res2;
    } args = {12345, {256, 256}, {256, 256}};

    // parse args
    for (int i = 1; i < argc; ++i) {
        if (!strcmp("--res", argv[i])) {
            args.res.x = atoi(argv[++i]);
            args.res.y = atoi(argv[++i]);
        } else if (!strcmp("--seed", argv[i])) {
            args.seed = atoi(argv[++i]);
        } else if (!strcmp("--help", argv[i])) {
            usage(argv[0]);
            return 0;
        }
    }

    // setup
    srand(args.seed);
    args.res2.x = nextPot(args.res.x);
    args.res2.y = nextPot(args.res.y);

    // init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // Create the window
    LOG("Loading {Window-Main}\n");
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_SIZE, WINDOW_SIZE, "DitherGPU", NULL, NULL
    );
    if (window == NULL) {
        LOG("=> Failure <=\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL functions
    LOG("Loading {OpenGL}\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG("gladLoadGLLoader failed\n");
        return -1;
    }

    // run demo
    LOG("-- Begin -- Init\n");
    init(args.res.x, args.res.y, args.res2.x, args.res2.y);
    LOG("-- End -- Init\n");

    int i = 0;
    int cnt = args.res.x * args.res.y;
    while (!glfwWindowShouldClose(window) && i < cnt) {
        float val = i / float(cnt);

        glfwPollEvents();


#if 0 // invert halfway through (this is not mandatory)
        if (i >= cnt / 2) {

            if (i == cnt / 2) {
                glBindFramebuffer(GL_FRAMEBUFFER, g_gl.framebuffer);
                glClearColor(0.0, 0.0, 0.0, 0.0);
                glClear(GL_COLOR_BUFFER_BIT);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                findMin(args.res2.x, args.res2.y);
            }

            val = 1.0f - (i - cnt / 2) / float(cnt);
        }
#endif
        computeDitherOnce(args.res.x,
                          args.res.y,
                          args.res2.x,
                          args.res2.y,
                          val);
        ++i;

        // print progress
        if (i % args.res.x == 0)
            LOG("Progress: %.2f / 100\n", 100.f * (i / float(cnt)));

        render(args.res2.x, args.res2.y, val);
        glfwSwapBuffers(window);
    }

    // export texture
    {
        std::vector<uint8_t> pngData(cnt);
        std::vector<float> imgData(args.res2.x * args.res2.y);
        glActiveTexture(GL_TEXTURE0 + TEXTURE_DITHER);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &imgData[0]);

        for (int j = 0; j < args.res.y; ++j) {
            for (int i = 0; i < args.res.x; ++i) {
                pngData[i + args.res.x * j] =
                    imgData[i + args.res2.x * j] * 255.f;
            }
        }

        stbi_write_png("mask.png", args.res.x, args.res.y, 1, &pngData[0], 0);
    }

    // kill
    glfwTerminate();
}


