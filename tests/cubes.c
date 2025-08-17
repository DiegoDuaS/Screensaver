#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 1800
#define HEIGHT 950    
#define NUM_CUBES 100
#define G 0.01f
#define Y_THRESHOLD -5.0f   
#define MAX_HEIGHT 5.0f

// Cámara
float camYaw = 0.0f;
float camPitch = 0.0f;
float camDist = 15.0f;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 pos;
    float angle;
    float speed;
    float moveAngle;
    int state;       // 0 = rotando, 1 = cayendo
    float stepHeight;
    float rotAmount;
    float rotTarget;
    float vy;
    float targetY;
    float r, g, b;   // color
    int rotDirection; // 1 o -1
} Cube;

// Actualiza un cubo individual
void updateCube(Cube *c) {
    if(c->state == 0) { // rotando
        float rotStep = 0.05f * c->rotDirection;
        c->angle += rotStep;
        c->rotAmount += fabs(rotStep);
        if(c->rotAmount >= c->rotTarget) {
            c->rotAmount = 0;
            c->state = 1;
            c->vy = 0;
            c->targetY = c->pos.y - c->stepHeight;
        }
    } else if(c->state == 1) { // cayendo
        c->vy += G;
        c->pos.y -= c->vy;
        if(c->pos.y <= c->targetY) {
            c->pos.y = c->targetY;
            c->vy = 0;
            c->state = 0;
        }
    }

    // Avance
    c->pos.x += cosf(c->moveAngle) * c->speed;
    c->pos.z += sinf(c->moveAngle) * c->speed;

    // Reinicio si pasa el umbral
    if(c->pos.y < Y_THRESHOLD) {
        c->pos.y = MAX_HEIGHT + ((float)rand()/RAND_MAX)*5.0f; // altura inicial aleatoria
        c->pos.x = ((float)rand()/RAND_MAX)*20.0f - 10.0f;
        c->pos.z = ((float)rand()/RAND_MAX)*20.0f - 10.0f;
        c->angle = ((float)rand()/RAND_MAX)*2*M_PI;
        c->rotAmount = 0;
        c->rotDirection = (rand()%2)?1:-1;
    }
}

// Dibuja un cubo en una posición dada, con rotación y color
void drawCubeAt(Vec3 pos, float angle, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(angle * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);

    glColor3f(r, g, b); // color uniforme

    glBegin(GL_QUADS);

    // Cara frontal
    glVertex3f(-0.5f,-0.5f,0.5f);
    glVertex3f(0.5f,-0.5f,0.5f);
    glVertex3f(0.5f,0.5f,0.5f);
    glVertex3f(-0.5f,0.5f,0.5f);

    // Cara trasera
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f,0.5f,-0.5f);
    glVertex3f(0.5f,0.5f,-0.5f);
    glVertex3f(0.5f,-0.5f,-0.5f);

    // Cara izquierda
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f,0.5f);
    glVertex3f(-0.5f,0.5f,0.5f);
    glVertex3f(-0.5f,0.5f,-0.5f);

    // Cara derecha
    glVertex3f(0.5f,-0.5f,-0.5f);
    glVertex3f(0.5f,0.5f,-0.5f);
    glVertex3f(0.5f,0.5f,0.5f);
    glVertex3f(0.5f,-0.5f,0.5f);

    // Cara superior
    glVertex3f(-0.5f,0.5f,-0.5f);
    glVertex3f(-0.5f,0.5f,0.5f);
    glVertex3f(0.5f,0.5f,0.5f);
    glVertex3f(0.5f,0.5f,-0.5f);

    // Cara inferior
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(0.5f,-0.5f,-0.5f);
    glVertex3f(0.5f,-0.5f,0.5f);
    glVertex3f(-0.5f,-0.5f,0.5f);

    glEnd();
    glPopMatrix();
}

// Inicializa cubos con posiciones y velocidades aleatorias
void initCubes(Cube cubes[NUM_CUBES]) {
    srand(time(NULL));
    for(int i=0;i<NUM_CUBES;i++) {
        cubes[i].pos.x = ((float)rand()/RAND_MAX)*20.0f - 10.0f; // -10 a 10
        cubes[i].pos.y = ((float)rand()/RAND_MAX)*5.0f;           // altura inicial
        cubes[i].pos.z = ((float)rand()/RAND_MAX)*60.0f - 50.0f; // -20 a 20, más profundidad
        cubes[i].angle = 0.0f; // todos comienzan en la misma orientación
        cubes[i].speed = 0.01f + ((float)rand()/RAND_MAX)*0.05f;
        cubes[i].moveAngle = ((float)rand()/RAND_MAX)*2*M_PI;
        cubes[i].state = 0;
        cubes[i].stepHeight = 0.5f + ((float)rand()/RAND_MAX)*1.5f; // 0.5 a 2.0
        cubes[i].rotAmount = 0.0f;
        cubes[i].rotTarget = M_PI/2;
        cubes[i].vy = 0.0f;
        cubes[i].targetY = cubes[i].pos.y - cubes[i].stepHeight;
        cubes[i].r = (float)rand()/RAND_MAX;
        cubes[i].g = (float)rand()/RAND_MAX;
        cubes[i].b = (float)rand()/RAND_MAX;
        cubes[i].rotDirection = (rand()%2)?1:-1; // sentido de giro
    }
}


int main(int argc, char* argv[]) {

    Cube cubes[NUM_CUBES];
    initCubes(cubes);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Cubo Escalera OpenGL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);

    glEnable(GL_DEPTH_TEST);

    int running = 1;
    SDL_Event event;

    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = 0;
        }

        // Actualizar todos los cubos
        for(int i=0;i<NUM_CUBES;i++) {
            updateCube(&cubes[i]);
        }

        glViewport(0,0,WIDTH,HEIGHT);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // Cámara
        gluLookAt(camDist * sinf(camYaw), camDist * sinf(camPitch), camDist * cosf(camYaw),
                  0,0,0,
                  0,1,0);

        // Dibujar todos los cubos
        for(int i=0;i<NUM_CUBES;i++) {
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
