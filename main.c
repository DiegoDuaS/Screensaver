#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "tests/cubes.h"

#define WIDTH 1800
#define HEIGHT 950

float camYaw = 0.0f;
float camPitch = 0.0f;
float camDist = 15.0f;

int main(int argc, char* argv[]) {
    Cube cubes[NUM_CUBES];
    initCubes(cubes);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Cubo Escalera OpenGL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);

    glEnable(GL_DEPTH_TEST);

    int running = 1;
    SDL_Event event;

    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = 0;
        }

        for(int i=0; i<NUM_CUBES; i++) updateCube(&cubes[i]);

        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(camDist * sinf(camYaw),
                  camDist * sinf(camPitch),
                  camDist * cosf(camYaw),
                  0, 0, 0, 0, 1, 0);

        for(int i=0; i<NUM_CUBES; i++) {
            drawCubeAt(cubes[i].pos, cubes[i].angle, cubes[i].r, cubes[i].g, cubes[i].b);
        }

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
