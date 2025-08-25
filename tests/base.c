#include <SDL2/SDL.h> 
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE 100
#define SCALE 1.0f
#define DEF_SPHERES 150
#define GRAVITY -0.02f
#define BOUNCE 0.7f
#define SPAWN_INTERVAL 500  // tiempo entre spawns en ms

typedef struct {
    float x,y,z;
    float vx,vz,vy;
    float radius;
    float r,g,b;  
    int active;   // nueva bandera de actividad
} Sphere;

Sphere spheres[DEF_SPHERES];

int numSpheres = 150;       
float waveAmplitude = 2.0f; 
float waveFrequency = 1.0f; 
int windowWidth = 1024;
int windowHeight = 768;

float waveHeight(float x, float z, float t){
    return waveAmplitude * (
        1.5f*sinf(0.3f*x*waveFrequency + t) + 
        1.0f*cosf(0.4f*z*waveFrequency + 0.5f*t) +
        0.7f*sinf(0.2f*(x+z)*waveFrequency + 0.8f*t)
    );
}

void getTerrainColor(float x, float z, float t, float *r, float *g, float *b){
    *r = 0.2f + 0.1f * sinf(t + x*0.1f);
    *g = 0.5f + 0.3f * sinf(t + z*0.1f);
    *b = 0.7f + 0.2f * cosf(t + (x+z)*0.05f);
}

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
                glNormal3f(0,1,0);
                glVertex3f(i*SCALE,h1,j*SCALE);
                glVertex3f((i+1)*SCALE,h2,j*SCALE);
                glVertex3f((i+1)*SCALE,h3,(j+1)*SCALE);
                glVertex3f(i*SCALE,h4,(j+1)*SCALE);
            glEnd();
        }
    }
}

void renderSpheres(){
    for(int i=0;i<numSpheres;i++){
        if(!spheres[i].active) continue; // solo esferas activas
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

void initOpenGL(){
    glEnable(GL_DEPTH_TEST);
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

    GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess[] = { 50.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

void reshape(int w, int h){
    if(h == 0) h = 1;
    windowWidth = w;
    windowHeight = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w/(float)h, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char* argv[]){
    if(argc > 1) numSpheres = atof(argv[1]);
    if(argc > 2) waveAmplitude = atof(argv[2]);
    if(argc > 3) waveFrequency = atof(argv[3]);
    if(numSpheres > DEF_SPHERES) numSpheres = DEF_SPHERES;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Olas con Esferas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    reshape(windowWidth, windowHeight);
    initOpenGL();

    float centerX = (GRID_SIZE * SCALE) / 2.0f;
    float centerZ = (GRID_SIZE * SCALE) / 2.0f;
    float camY = 15.0f;
    float radius = 40.0f;
    float yaw = 0.0f;
    float camSpeed = 0.01f;
    float camX, camZ;
    float lookX, lookY, lookZ;

    srand((unsigned int)time(NULL));
    for(int i=0;i<numSpheres;i++){
        spheres[i].x = rand()%GRID_SIZE*SCALE;
        spheres[i].z = rand()%GRID_SIZE*SCALE;
        spheres[i].y = 20.0f + ((float)rand() / RAND_MAX) * 60.0f;
        spheres[i].vx = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vz = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vy = 0.0f;
        spheres[i].radius = 0.5f;
        spheres[i].r = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].g = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].b = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].active = 0; // empiezan inactivas
    }

    int running = 1;
    SDL_Event event;
    float t = 0.0f;

    Uint32 lastTime = SDL_GetTicks();
    Uint32 lastSpawn = lastTime;
    int spawned = 0;

    char title[128];

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) running = 0;
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED){
                reshape(event.window.data1, event.window.data2);
            }
        }

        Uint32 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f; // en segundos
        lastTime = now;

        if(now - lastSpawn >= SPAWN_INTERVAL && spawned < numSpheres){
            spheres[spawned].active = 1;
            spawned++;
            lastSpawn = now;
        }

        yaw += camSpeed;
        camX = centerX + radius * sinf(yaw);
        camZ = centerZ + radius * cosf(yaw);
        lookX = centerX - camX;
        lookY = -camY;
        lookZ = centerZ - camZ;

        for(int i=0;i<numSpheres;i++){
            if(!spheres[i].active) continue;
            spheres[i].x += spheres[i].vx;
            spheres[i].z += spheres[i].vz;
            spheres[i].vy += GRAVITY;
            spheres[i].y += spheres[i].vy;

            float floorY = waveHeight(spheres[i].x, spheres[i].z, t) + spheres[i].radius;
            if(spheres[i].y < floorY){
                spheres[i].y = floorY;
                spheres[i].vy = -spheres[i].vy * BOUNCE;
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

        // Actualizar título con FPS
        float fps = 1.0f / deltaTime;
        sprintf(title, "Olas con Esferas - FPS: %.2f", fps);
        SDL_SetWindowTitle(window, title);

        SDL_Delay(16);
        t += 0.05f;
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
