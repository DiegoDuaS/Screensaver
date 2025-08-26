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
int gridSize = GRID_SIZE;
float waveAmplitude = 2.0f;
float waveFrequency = 1.0f;
int windowWidth = 1024;
int windowHeight = 768;

// Z-buffer
float* zbuffer = NULL;

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
        spheres[i].x = (rand()%gridSize)*SCALE;
        spheres[i].z = (rand()%gridSize)*SCALE;
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
        if(spheres[i].x<0 || spheres[i].x>gridSize*SCALE) spheres[i].vx*=-1;
        if(spheres[i].z<0 || spheres[i].z>gridSize*SCALE) spheres[i].vz*=-1;
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
// Renderizar escena
// -----------------------------------------------------------------------------
// Función auxiliar para renderizar triángulo relleno con z-buffer (versión secuencial)
void drawTriangle(float* zbuffer, Uint32* colorBuffer,
                  float x1, float y1, float z1,
                  float x2, float y2, float z2,
                  float x3, float y3, float z3,
                  Uint32 color) {
    
    // Ordenar vértices por Y (y1 <= y2 <= y3)
    if (y1 > y2) { float tx=x1,ty=y1,tz=z1; x1=x2;y1=y2;z1=z2; x2=tx;y2=ty;z2=tz; }
    if (y2 > y3) { float tx=x2,ty=y2,tz=z2; x2=x3;y2=y3;z2=z3; x3=tx;y3=ty;z3=tz; }
    if (y1 > y2) { float tx=x1,ty=y1,tz=z1; x1=x2;y1=y2;z1=z2; x2=tx;y2=ty;z2=tz; }
    
    int minY = (int)fmaxf(0, ceilf(y1));
    int maxY = (int)fminf(windowHeight-1, floorf(y3));
    
    for (int y = minY; y <= maxY; y++) {
        float xLeft, xRight, zLeft, zRight;
        
        // Calcular intersecciones con los bordes del triángulo
        if (y < y2) {
            // Lado superior del triángulo
            float t1 = (y3 - y1 == 0) ? 0 : (y - y1) / (y3 - y1);
            float t2 = (y2 - y1 == 0) ? 0 : (y - y1) / (y2 - y1);
            
            xLeft = x1 + t1 * (x3 - x1);
            zLeft = z1 + t1 * (z3 - z1);
            xRight = x1 + t2 * (x2 - x1);
            zRight = z1 + t2 * (z2 - z1);
        } else {
            // Lado inferior del triángulo
            float t1 = (y3 - y1 == 0) ? 0 : (y - y1) / (y3 - y1);
            float t2 = (y3 - y2 == 0) ? 0 : (y - y2) / (y3 - y2);
            
            xLeft = x1 + t1 * (x3 - x1);
            zLeft = z1 + t1 * (z3 - z1);
            xRight = x2 + t2 * (x3 - x2);
            zRight = z2 + t2 * (z3 - z2);
        }
        
        // Asegurar que xLeft <= xRight
        if (xLeft > xRight) {
            float temp = xLeft; xLeft = xRight; xRight = temp;
            temp = zLeft; zLeft = zRight; zRight = temp;
        }
        
        int minX = (int)fmaxf(0, ceilf(xLeft));
        int maxX = (int)fminf(windowWidth-1, floorf(xRight));
        
        // Renderizar scanline
        for (int x = minX; x <= maxX; x++) {
            float t = (xRight - xLeft == 0) ? 0 : (x - xLeft) / (xRight - xLeft);
            float z = zLeft + t * (zRight - zLeft);
            
            int idx = y * windowWidth + x;
            if (z < zbuffer[idx]) {
                zbuffer[idx] = z;
                colorBuffer[idx] = color;
            }
        }
    }
}

void renderScene(SDL_Renderer* renderer, float t,
                          float lightX,float lightY,float lightZ,
                          float camX,float camY,float camZ,
                          float lookX,float lookY,float lookZ)
{
    // Buffer único para versión secuencial
    float* zbuffer = malloc(windowWidth*windowHeight*sizeof(float));
    Uint32* colorBuffer = malloc(windowWidth*windowHeight*sizeof(Uint32));
    
    // Inicializar buffers
    for(int i=0; i<windowWidth*windowHeight; i++){
        zbuffer[i] = 1e30f;
        colorBuffer[i] = 0;
    }

    // -----------------------------
    // Render Terrain secuencial con triángulos
    // -----------------------------
    for(int i=0; i<gridSize-1; i++){
        for(int j=0; j<gridSize-1; j++){
            // Calcular posiciones de los 4 vértices del quad
            float wx1 = i*SCALE, wz1 = j*SCALE;
            float wx2 = (i+1)*SCALE, wz2 = j*SCALE;
            float wx3 = (i+1)*SCALE, wz3 = (j+1)*SCALE;
            float wx4 = i*SCALE, wz4 = (j+1)*SCALE;
            
            float wy1 = waveHeight(wx1, wz1, t);
            float wy2 = waveHeight(wx2, wz2, t);
            float wy3 = waveHeight(wx3, wz3, t);
            float wy4 = waveHeight(wx4, wz4, t);

            // Proyectar los 4 vértices
            float sx1, sy1, d1, sx2, sy2, d2, sx3, sy3, d3, sx4, sy4, d4;
            project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                      wx1, wy1, wz1, &sx1, &sy1, &d1);
            project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                      wx2, wy2, wz2, &sx2, &sy2, &d2);
            project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                      wx3, wy3, wz3, &sx3, &sy3, &d3);
            project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                      wx4, wy4, wz4, &sx4, &sy4, &d4);

            // Calcular normal del triángulo para iluminación
            float centerX = (wx1 + wx2 + wx3 + wx4) * 0.25f;
            float centerZ = (wz1 + wz2 + wz3 + wz4) * 0.25f;
            float centerY = (wy1 + wy2 + wy3 + wy4) * 0.25f;
            
            float hL = waveHeight(centerX-0.1f, centerZ, t);
            float hR = waveHeight(centerX+0.1f, centerZ, t);
            float hD = waveHeight(centerX, centerZ-0.1f, t);
            float hU = waveHeight(centerX, centerZ+0.1f, t);
            
            float nx = hL - hR, ny = 2.0f, nz = hD - hU;
            float len = sqrtf(nx*nx + ny*ny + nz*nz);
            nx/=len; ny/=len; nz/=len;

            float lx = lightX - centerX, ly = lightY - centerY, lz = lightZ - centerZ;
            float llen = sqrtf(lx*lx+ly*ly+lz*lz);
            lx/=llen; ly/=llen; lz/=llen;

            float diff = fmaxf(0.0f, nx*lx+ny*ly+nz*lz);

            // Calcular color
            float wave = 0.5f + 0.5f*sinf(t*0.3f + (i+j)*0.05f);
            Uint8 r = 10;
            Uint8 g = (Uint8)((50+150*diff)*wave);
            Uint8 b = (Uint8)((100+100*diff)*(1-0.3f*wave));
            Uint32 color = (r<<16)|(g<<8)|b;

            // Renderizar dos triángulos que forman el quad
            // Triángulo 1: (1,2,3)
            drawTriangle(zbuffer, colorBuffer,
                        sx1, sy1, d1,
                        sx2, sy2, d2,
                        sx3, sy3, d3,
                        color);
            
            // Triángulo 2: (1,3,4)
            drawTriangle(zbuffer, colorBuffer,
                        sx1, sy1, d1,
                        sx3, sy3, d3,
                        sx4, sy4, d4,
                        color);
        }
    }

    // -----------------------------
    // Render Esferas secuencial
    // -----------------------------
    for(int i=0; i<numSpheres; i++){
        if(!spheres[i].active) continue;

        float sx, sy, depth;
        project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                  spheres[i].x,spheres[i].y,spheres[i].z,&sx,&sy,&depth);

        int rad = (int)(spheres[i].radius*500/depth);
        for(int dx=-rad; dx<=rad; dx++){
            for(int dy=-rad; dy<=rad; dy++){
                if(dx*dx + dy*dy <= rad*rad){
                    int ix = (int)sx + dx;
                    int iy = (int)sy + dy;
                    if(ix>=0 && ix<windowWidth && iy>=0 && iy<windowHeight){
                        int idx = iy*windowWidth + ix;
                        if(depth < zbuffer[idx]){
                            zbuffer[idx] = depth;

                            float nx = dx/(float)rad;
                            float ny = -dy/(float)rad;
                            float nz = sqrtf(fmaxf(0.0f,1-nx*nx-ny*ny));
                            float px = spheres[i].x + nx*spheres[i].radius;
                            float py = spheres[i].y + ny*spheres[i].radius;
                            float pz = spheres[i].z + nz*spheres[i].radius;

                            float lx = lightX-px, ly = lightY-py, lz = lightZ-pz;
                            float len = sqrtf(lx*lx+ly*ly+lz*lz);
                            lx/=len; ly/=len; lz/=len;

                            float diff = fmaxf(0.0f, nx*lx + ny*ly + nz*lz);
                            Uint8 r = (Uint8)(spheres[i].r*255*diff);
                            Uint8 g = (Uint8)(spheres[i].g*255*diff);
                            Uint8 b = (Uint8)(spheres[i].b*255*diff);
                            colorBuffer[idx] = (r<<16)|(g<<8)|b;
                        }
                    }
                }
            }
        }
    }

    // -----------------------------
    // Renderizar al SDL
    // -----------------------------
    for(int i=0; i<windowWidth*windowHeight; i++){
        Uint8 r = (colorBuffer[i]>>16)&0xFF;
        Uint8 g = (colorBuffer[i]>>8)&0xFF;
        Uint8 b = colorBuffer[i]&0xFF;
        SDL_SetRenderDrawColor(renderer,r,g,b,255);
        int x = i%windowWidth;
        int y = i/windowWidth;
        SDL_RenderDrawPoint(renderer,x,y);
    }

    // Liberar memoria
    free(zbuffer);
    free(colorBuffer);
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
    if(numSpheres<=0) numSpheres=DEF_SPHERES;

    if(argc>2) gridSize=atof(argv[2]);
    if (gridSize<GRID_SIZE) gridSize=GRID_SIZE;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Pseudo 3D SDL con Luz",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        windowWidth,windowHeight,SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    initSpheres(numSpheres);

    float centerX = gridSize*SCALE/2;
    float centerZ = gridSize*SCALE/2;
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

    zbuffer = malloc(windowWidth * windowHeight * sizeof(float));

    char title[128];  // Buffer para el título de la ventana

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) running=0;
            if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_1) viewMode=1;
                if(event.key.keysym.sym==SDLK_2) viewMode=2;
                if(event.key.keysym.sym==SDLK_3) viewMode=3;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;

                zbuffer = realloc(zbuffer, windowWidth * windowHeight * sizeof(float));
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

        renderScene(renderer,t,lightX,lightY,lightZ,camX,camY,camZ,lookX,lookY,lookZ);

        SDL_RenderPresent(renderer);
        
        // Cálculo y mostrar FPS en el título
        float fps = 1.0f / deltaTime;
        sprintf(title, "Olas con Esferas - FPS: %.2f", fps);
        SDL_SetWindowTitle(window, title);

        //SDL_Delay(16);  // Limitar a ~60 FPS
        t += 0.05f;     // Avanzar tiempo de animación
    }

    free(zbuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
