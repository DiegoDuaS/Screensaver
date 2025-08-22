#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE 70
#define SCALE 1.0f
#define NUM_SPHERES 150
#define GRAVITY -0.02f
#define BOUNCE 0.7f

typedef struct {
    float x,y,z;
    float vx,vz,vy;
    float radius;
    float r,g,b;  // color de la esfera
} Sphere;

Sphere spheres[NUM_SPHERES];

float waveHeight(float x, float z, float t){
    return 1.5f*sinf(0.3f*x + t) + 1.0f*cosf(0.4f*z + 0.5f*t)
         + 0.7f*sinf(0.2f*(x+z) + 0.8f*t);
}

void getTerrainColor(float x, float z, float t, float *r, float *g, float *b){
    *r = 0.2f + 0.1f * sinf(t + x*0.1f);
    *g = 0.5f + 0.3f * sinf(t + z*0.1f);
    *b = 0.7f + 0.2f * cosf(t + (x+z)*0.05f);
}

// --- Renderizar terreno ---
void renderTerrain(float t){
    for(int i=0;i<GRID_SIZE-1;i++){
        for(int j=0;j<GRID_SIZE-1;j++){
            float h1 = waveHeight(i*SCALE,j*SCALE,t);
            float h2 = waveHeight((i+1)*SCALE,j*SCALE,t);
            float h3 = waveHeight((i+1)*SCALE,(j+1)*SCALE,t);
            float h4 = waveHeight(i*SCALE,(j+1)*SCALE,t);

            float r,g,b;
            getTerrainColor(i,j,t,&r,&g,&b);
            glColor3f(r,g,b);

            glBegin(GL_QUADS);
                glNormal3f(0,1,0); // normal aproximada
                glVertex3f(i*SCALE,h1,j*SCALE);
                glVertex3f((i+1)*SCALE,h2,j*SCALE);
                glVertex3f((i+1)*SCALE,h3,(j+1)*SCALE);
                glVertex3f(i*SCALE,h4,(j+1)*SCALE);
            glEnd();
        }
    }
}

// --- Renderizar esferas ---
void renderSpheres(){
    for(int i=0;i<NUM_SPHERES;i++){
        glPushMatrix();
        glTranslatef(spheres[i].x,spheres[i].y,spheres[i].z);

        GLfloat matDiffuse[] = { spheres[i].r, spheres[i].g, spheres[i].b, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matDiffuse);

        GLUquadric* q = gluNewQuadric();
        gluQuadricNormals(q, GLU_SMOOTH);
        gluSphere(q, spheres[i].radius, 32, 32);
        gluDeleteQuadric(q);

        glPopMatrix();
    }
}

// --- Inicialización de OpenGL ---
void initOpenGL(){
    glEnable(GL_DEPTH_TEST);

    // iluminación
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightPos[] = { 0.0f, 50.0f, 50.0f, 1.0f };
    GLfloat ambient[]  = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuse[]  = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // Material con brillo
    GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess[] = { 50.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

int main(int argc, char* argv[]){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Olas con Esferas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1024.0/768.0, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);

    initOpenGL(); 

    // --- Cámara ---
    float centerX = (GRID_SIZE * SCALE) / 2.0f;
    float centerZ = (GRID_SIZE * SCALE) / 2.0f;
    float centerY = 0.0f;
    float camY = 15.0f;
    float radius = 40.0f;
    float yaw = 0.0f;
    float camSpeed = 0.01f;
    float camX, camZ;
    float lookX, lookY, lookZ;

    srand((unsigned int)time(NULL));
    for(int i=0;i<NUM_SPHERES;i++){
        spheres[i].x = rand()%GRID_SIZE*SCALE;
        spheres[i].z = rand()%GRID_SIZE*SCALE;
        spheres[i].y = 10.0f + rand()%5;
        spheres[i].vx = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vz = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vy = 0.0f;
        spheres[i].radius = 0.5f;

        // colores aleatorios vivos
        spheres[i].r = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].g = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].b = 0.3f + ((rand()%100)/100.0f)*0.7f;
    }

    int running = 1;
    SDL_Event event;
    float t = 0.0f;

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) running = 0;
        }

        // cámara girando
        yaw += camSpeed;
        camX = centerX + radius * sinf(yaw);
        camZ = centerZ + radius * cosf(yaw);

        lookX = centerX - camX;
        lookY = centerY - camY;
        lookZ = centerZ - camZ;

        // actualizar esferas
        for(int i=0;i<NUM_SPHERES;i++){
            spheres[i].x += spheres[i].vx;
            spheres[i].z += spheres[i].vz;
            spheres[i].vy += GRAVITY;
            spheres[i].y += spheres[i].vy;

            float floorY = waveHeight(spheres[i].x, spheres[i].z, t) + spheres[i].radius;
            if(spheres[i].y < floorY){
                spheres[i].y = floorY;
                spheres[i].vy = -spheres[i].vy * BOUNCE;
            }

            // colisiones entre esferas
            for(int j=i+1;j<NUM_SPHERES;j++){
                float dx = spheres[j].x - spheres[i].x;
                float dz = spheres[j].z - spheres[i].z;
                float dist = sqrtf(dx*dx + dz*dz);
                float minDist = spheres[i].radius + spheres[j].radius;
                if(dist < minDist && dist > 0.0f){
                    float overlap = 0.5f*(minDist - dist)/dist;
                    spheres[i].x -= dx*overlap; spheres[i].z -= dz*overlap;
                    spheres[j].x += dx*overlap; spheres[j].z += dz*overlap;
                }
            }

            if(spheres[i].x < 0 || spheres[i].x > GRID_SIZE*SCALE) spheres[i].vx *= -1;
            if(spheres[i].z < 0 || spheres[i].z > GRID_SIZE*SCALE) spheres[i].vz *= -1;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        gluLookAt(camX, camY, camZ,
                  camX + lookX, camY + lookY, camZ + lookZ,
                  0,1,0);

        renderTerrain(t);
        renderSpheres();

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
        t += 0.05f;
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
