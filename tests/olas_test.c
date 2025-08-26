#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define GRID_SIZE 40
#define SCALE 1.0f
#define DEF_SPHERES 10000
#define GRAVITY -0.02f
#define BOUNCE 0.7f
#define SPAWN_INTERVAL 1

typedef struct {
    float x,y,z;
    float vx,vy,vz;
    float radius;
    float r,g,b;
    int active;
} Sphere;

Sphere spheres[DEF_SPHERES];

int numSpheres = 1;
float waveAmplitude = 2.0f;
float waveFrequency = 1.0f;
int windowWidth = 1024;
int windowHeight = 768;

// Z-buffer
float zbuffer[1024*768];

// -----------------------------------------------------------------------------
// Funciones de utilidades
// -----------------------------------------------------------------------------
float waveHeight(float x,float z,float t){
    return waveAmplitude*(
        1.5f*sinf(0.3f*x*waveFrequency + t) +
        1.0f*cosf(0.4f*z*waveFrequency + 0.5f*t) +
        0.7f*sinf(0.2f*(x+z)*waveFrequency + 0.8f*t)
    );
}

void project3D(float camX,float camY,float camZ,float lookX,float lookY,float lookZ,
               float x,float y,float z,float *sx,float *sy,float *depth) {
    float rx = x - camX;
    float ry = y - camY;
    float rz = z - camZ;

    float lx = lookX - camX;
    float lz = lookZ - camZ;

    float angle = atan2f(lx,lz);
    float ca = cosf(angle);
    float sa = sinf(angle);
    float tx = ca*rx - sa*rz;
    float tz = sa*rx + ca*rz;
    float ty = ry;

    float fov = 500.0f;
    if(tz <= 0.1f) tz = 0.1f;
    *sx = windowWidth/2 + tx * fov / tz;
    *sy = windowHeight/2 - ty * fov / tz;
    *depth = tz;
}

// -----------------------------------------------------------------------------
// Inicialización de esferas
// -----------------------------------------------------------------------------
void initSpheres(int n){
    srand((unsigned int)time(NULL));
    if(n>DEF_SPHERES) n=DEF_SPHERES;
    numSpheres = n;

    for(int i=0;i<numSpheres;i++){
        spheres[i].x = (rand()%GRID_SIZE)*SCALE;
        spheres[i].z = (rand()%GRID_SIZE)*SCALE;
        spheres[i].y = 20.0f + ((float)rand()/RAND_MAX)*60.0f;
        spheres[i].vx = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vz = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vy = 0;
        spheres[i].radius = 0.5f;
        spheres[i].r = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].g = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].b = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].active = 0;
    }
}

// -----------------------------------------------------------------------------
// Física de esferas y colisiones
// -----------------------------------------------------------------------------
void updatePhysics(float t){
    // Movimiento y rebotes
    for(int i=0;i<numSpheres;i++){
        if(!spheres[i].active) continue;
        spheres[i].x += spheres[i].vx;
        spheres[i].z += spheres[i].vz;
        spheres[i].vy += GRAVITY;
        spheres[i].y += spheres[i].vy;

        float floorY = waveHeight(spheres[i].x,spheres[i].z,t)+spheres[i].radius;
        if(spheres[i].y<floorY){
            spheres[i].y=floorY;
            spheres[i].vy*=-BOUNCE;
        }
        if(spheres[i].x<0 || spheres[i].x>GRID_SIZE*SCALE) spheres[i].vx*=-1;
        if(spheres[i].z<0 || spheres[i].z>GRID_SIZE*SCALE) spheres[i].vz*=-1;
    }

    // Colisiones entre esferas
    for(int i=0;i<numSpheres;i++){
        if(!spheres[i].active) continue;
        for(int j=i+1;j<numSpheres;j++){
            if(!spheres[j].active) continue;

            float dx = spheres[j].x - spheres[i].x;
            float dy = spheres[j].y - spheres[i].y;
            float dz = spheres[j].z - spheres[i].z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            float minDist = spheres[i].radius + spheres[j].radius;

            if(dist < minDist && dist > 0.0f){
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
                float overlap = minDist - dist;

                spheres[i].x -= nx*overlap*0.5f;
                spheres[i].y -= ny*overlap*0.5f;
                spheres[i].z -= nz*overlap*0.5f;
                spheres[j].x += nx*overlap*0.5f;
                spheres[j].y += ny*overlap*0.5f;
                spheres[j].z += nz*overlap*0.5f;

                float viDot = spheres[i].vx*nx + spheres[i].vy*ny + spheres[i].vz*nz;
                float vjDot = spheres[j].vx*nx + spheres[j].vy*ny + spheres[j].vz*nz;
                float avg = (viDot + vjDot) * 0.5f;

                spheres[i].vx += (avg - viDot)*nx;
                spheres[i].vy += (avg - viDot)*ny;
                spheres[i].vz += (avg - viDot)*nz;

                spheres[j].vx += (avg - vjDot)*nx;
                spheres[j].vy += (avg - vjDot)*ny;
                spheres[j].vz += (avg - vjDot)*nz;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Reset Z-buffer
// -----------------------------------------------------------------------------
void resetZBuffer(){
    for(int i=0;i<windowWidth*windowHeight;i++) zbuffer[i] = 1e30f;
}

// -----------------------------------------------------------------------------
// Actualizar posición de la cámara
// -----------------------------------------------------------------------------
void updateCamera(float centerX,float centerZ,float radius,float *camX,float *camY,float *camZ,
                  float *lookX,float *lookY,float *lookZ,float *yaw,float camSpeed){
    *yaw += camSpeed;
    *camX = centerX + radius*sinf(*yaw);
    *camZ = centerZ + radius*cosf(*yaw);
    *camY = 15.0f;
    *lookX = centerX - *camX;
    *lookY = -(*camY);
    *lookZ = centerZ - *camZ;
}

// -----------------------------------------------------------------------------
// Renderizar terreno con iluminación y Z-buffer
// -----------------------------------------------------------------------------
void renderTerrain(SDL_Renderer* renderer,float t,float lightX,float lightY,float lightZ,
                   float camX,float camY,float camZ,float lookX,float lookY,float lookZ){
    float step = 0.06f;
    for(float i=0; i<GRID_SIZE; i+=step){
        for(float j=0; j<GRID_SIZE; j+=step){
            float y = waveHeight(i*SCALE, j*SCALE, t);

            // normal aproximada
            float hL = waveHeight(i*SCALE-0.1f, j*SCALE, t);
            float hR = waveHeight(i*SCALE+0.1f, j*SCALE, t);
            float hD = waveHeight(i*SCALE, j*SCALE-0.1f, t);
            float hU = waveHeight(i*SCALE, j*SCALE+0.1f, t);
            float nx = hL - hR;
            float ny = 2.0f;
            float nz = hD - hU;
            float len = sqrtf(nx*nx + ny*ny + nz*nz);
            nx/=len; ny/=len; nz/=len;

            float lx = lightX - i*SCALE;
            float ly = lightY - y;
            float lz = lightZ - j*SCALE;
            float llen = sqrtf(lx*lx + ly*ly + lz*lz);
            lx/=llen; ly/=llen; lz/=llen;

            float diff = nx*lx + ny*ly + nz*lz;
            if(diff<0) diff=0;

            float sx,sy,d;
            project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                      i*SCALE, y, j*SCALE, &sx,&sy,&d);

            int ix = (int)sx;
            int iy = (int)sy;
            if(ix>=0 && ix<windowWidth && iy>=0 && iy<windowHeight){
                int idx = iy*windowWidth + ix;
                if(d < zbuffer[idx]){
                    zbuffer[idx] = d;

                    // Colores verde-azul suaves y difusos
                    float wave = 0.5f + 0.5f*sinf(t*0.3f + (i+j)*0.05f); // 0..1
                    Uint8 r = 10; // rojo casi constante
                    Uint8 g = (Uint8)((50 + 150*diff) * wave); // verde modulada
                    Uint8 b = (Uint8)((100 + 100*diff) * (1.0f - 0.3f*wave)); // azul complementario

                    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                    SDL_RenderDrawPoint(renderer, ix, iy);
                }
            }
        }
    }
}




// -----------------------------------------------------------------------------
// Renderizar esferas con iluminación y Z-buffer
// -----------------------------------------------------------------------------
void renderSpheres(SDL_Renderer* renderer,float lightX,float lightY,float lightZ,
                   float camX,float camY,float camZ,float lookX,float lookY,float lookZ){
    for(int i=0;i<numSpheres;i++){
        if(!spheres[i].active) continue;

        float sx,sy,d;
        project3D(camX,camY,camZ,camX+lookX,camY+lookY,camZ+lookZ,
                  spheres[i].x,spheres[i].y,spheres[i].z,&sx,&sy,&d);

        int rad = (int)(spheres[i].radius*500/d);
        for(int dx=-rad; dx<=rad; dx++){
            for(int dy=-rad; dy<=rad; dy++){
                if(dx*dx+dy*dy <= rad*rad){
                    int ix = (int)sx + dx;
                    int iy = (int)sy + dy;
                    if(ix>=0 && ix<windowWidth && iy>=0 && iy<windowHeight){
                        int idx = iy*windowWidth + ix;
                        if(d < zbuffer[idx]){
                            zbuffer[idx] = d;

                            float nx = dx/(float)rad;
                            float ny = -dy/(float)rad;
                            float nz = sqrtf(fmaxf(0.0f, 1.0f - nx*nx - ny*ny));

                            float px = spheres[i].x + nx*spheres[i].radius;
                            float py = spheres[i].y + ny*spheres[i].radius;
                            float pz = spheres[i].z + nz*spheres[i].radius;

                            float lx = lightX - px;
                            float ly = lightY - py;
                            float lz = lightZ - pz;
                            float len = sqrtf(lx*lx + ly*ly + lz*lz);
                            lx/=len; ly/=len; lz/=len;

                            float diff = nx*lx + ny*ly + nz*lz;
                            if(diff<0) diff=0;

                            Uint8 r = (Uint8)(spheres[i].r*255*diff);
                            Uint8 g = (Uint8)(spheres[i].g*255*diff);
                            Uint8 b = (Uint8)(spheres[i].b*255*diff);

                            SDL_SetRenderDrawColor(renderer,r,g,b,255);
                            SDL_RenderDrawPoint(renderer, ix, iy);
                        }
                    }
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Actualizar posición de la cámara según el modo
// -----------------------------------------------------------------------------
void updateCameraView(int viewMode, float centerX,float centerZ,float radius,float *camX,float *camY,float *camZ,
                      float *lookX,float *lookY,float *lookZ,float *yaw){
    switch(viewMode){
        case 1: // Rotando alrededor del centro
            *yaw += 0.01f;
            *camX = centerX + radius*sinf(*yaw);
            *camZ = centerZ + radius*cosf(*yaw);
            *camY = 10.0f;
            *lookX = centerX - *camX;
            *lookY = -(*camY);
            *lookZ = centerZ - *camZ;
            break;
        case 2: // Vista desde el cielo
            *camX = centerX;
            *camY = 40.0f;
            *camZ = centerZ;
            *lookX = centerX;
            *lookY = 90.0f;
            *lookZ = centerZ;  
            break;
        case 3: // Vista lateral fija
            *camX = -20.0f;
            *camY = 10.0f;
            *camZ = centerZ;
            *lookX = centerX + 20.0f; // mirar al centro
            *lookY = -(*camY);
            *lookZ = centerZ - *camZ;
            break;
        default: // fallback
            *camX = centerX + radius*sinf(*yaw);
            *camZ = centerZ + radius*cosf(*yaw);
            *camY = 15.0f;
            *lookX = centerX - *camX;
            *lookY = -(*camY);
            *lookZ = centerZ - *camZ;
            break;
    }
}


int main(int argc, char* argv[]){
    if(argc>1) numSpheres=atoi(argv[1]);
    if(numSpheres>DEF_SPHERES) numSpheres=DEF_SPHERES;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Pseudo 3D SDL con Luz",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        windowWidth,windowHeight,SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    initSpheres(numSpheres);

    float centerX = GRID_SIZE*SCALE/2;
    float centerZ = GRID_SIZE*SCALE/2;
    float radius = 10.0f;
    float yaw = 0.0f;
    float camX, camY, camZ;
    float lookX, lookY, lookZ;

    float lightX = centerX + 30.0f;
    float lightY = 25.0f;
    float lightZ = centerZ + 30.0f;

    int running = 1;
    SDL_Event event;
    float t=0;
    Uint32 lastTime = SDL_GetTicks();
    Uint32 lastSpawn = lastTime;
    int spawned = 0;

    int viewMode = 1; // Inicio con cámara rotando

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) running=0;
            if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_1) viewMode=1;
                if(event.key.keysym.sym==SDLK_2) viewMode=2;
                if(event.key.keysym.sym==SDLK_3) viewMode=3;
            }
        }

        Uint32 now = SDL_GetTicks();
        float deltaTime = (now - lastTime)/1000.0f;
        lastTime = now;

        if(now - lastSpawn >= SPAWN_INTERVAL && spawned<numSpheres){
            spheres[spawned].active=1;
            spawned++;
            lastSpawn=now;
        }

        updateCameraView(viewMode, centerX, centerZ, radius, &camX,&camY,&camZ, &lookX,&lookY,&lookZ, &yaw);
        updatePhysics(t);
        resetZBuffer();

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);

        renderTerrain(renderer,t,lightX,lightY,lightZ,camX,camY,camZ,lookX,lookY,lookZ);
        renderSpheres(renderer,lightX,lightY,lightZ,camX,camY,camZ,lookX,lookY,lookZ);

        SDL_RenderPresent(renderer);
        t+=0.05f;
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
