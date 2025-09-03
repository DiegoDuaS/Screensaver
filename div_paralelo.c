#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define GRID_SIZE 40
#define SCALE 1.0f
#define DEF_SPHERES 100000
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

// Variables para SDL Texture
SDL_Texture* screenTexture = NULL;
Uint32* frameBuffer = NULL;
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

// funciones para gestión de buffers 
void initRenderBuffers() {
    frameBuffer = malloc(windowWidth * windowHeight * sizeof(Uint32));
    zbuffer = malloc(windowWidth * windowHeight * sizeof(float));
}

void freeRenderBuffers() {
    if(frameBuffer) { free(frameBuffer); frameBuffer = NULL; }
    if(zbuffer) { free(zbuffer); zbuffer = NULL; }
}

void resizeRenderBuffers(SDL_Renderer* renderer) {
    // Liberar buffers antiguos
    freeRenderBuffers();
    
    // Recrear textura con nuevo tamaño
    if(screenTexture) {
        SDL_DestroyTexture(screenTexture);
    }
    screenTexture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING,
        windowWidth, windowHeight);
    
    // Recrear buffers con nuevo tamaño
    initRenderBuffers();
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
        spheres[i].active = 1;
    }
}

// -----------------------------------------------------------------------------
// Física de esferas y colisiones
// -----------------------------------------------------------------------------
void updatePhysics(float t){
    // Movimiento y rebotes
    #pragma omp parallel for schedule(static)
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

    omp_lock_t *locks = malloc(numSpheres * sizeof(omp_lock_t));
    for (int i = 0; i < numSpheres; i++) {
        omp_init_lock(&locks[i]);
    }

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < numSpheres; i++) {
        for (int j = i + 1; j < numSpheres; j++) {
            if (!spheres[i].active || !spheres[j].active) continue;

            float dx = spheres[j].x - spheres[i].x;
            float dy = spheres[j].y - spheres[i].y;
            float dz = spheres[j].z - spheres[i].z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            float minDist = spheres[i].radius + spheres[j].radius;

            if (dist < minDist && dist > 0.0f) {
                // bloquear acceso concurrente a las dos esferas
                omp_set_lock(&locks[i]);
                omp_set_lock(&locks[j]);

                // normal de colisión
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;

                // separar esferas para evitar penetración
                float overlap = minDist - dist;
                spheres[i].x -= nx * overlap * 0.5f;
                spheres[i].y -= ny * overlap * 0.5f;
                spheres[i].z -= nz * overlap * 0.5f;
                spheres[j].x += nx * overlap * 0.5f;
                spheres[j].y += ny * overlap * 0.5f;
                spheres[j].z += nz * overlap * 0.5f;

                // calcular velocidad proyectada en la normal
                float viDot = spheres[i].vx * nx + spheres[i].vy * ny + spheres[i].vz * nz;
                float vjDot = spheres[j].vx * nx + spheres[j].vy * ny + spheres[j].vz * nz;
                float avg = (viDot + vjDot) * 0.5f;

                // actualizar velocidades (choque elástico)
                spheres[i].vx += (avg - viDot) * nx;
                spheres[i].vy += (avg - viDot) * ny;
                spheres[i].vz += (avg - viDot) * nz;

                spheres[j].vx += (avg - vjDot) * nx;
                spheres[j].vy += (avg - vjDot) * ny;
                spheres[j].vz += (avg - vjDot) * nz;

                omp_unset_lock(&locks[j]);
                omp_unset_lock(&locks[i]);
            }
        }
    }

}

// Reset Z-buffer 
void resetZBuffer() {
    // Usar memset para mayor velocidad
    #pragma omp parallel
    {
        int chunk = (windowWidth * windowHeight) / omp_get_num_threads();
        int start = omp_get_thread_num() * chunk;
        int end = (omp_get_thread_num() == omp_get_num_threads()-1) ? 
                 windowWidth * windowHeight : start + chunk;
        
        for(int i = start; i < end; i++) {
            zbuffer[i] = 1e30f;
            frameBuffer[i] = 0;
        }
    }
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

// Versión de drawTriangle que recorta a un cuadrante
void drawTriangleClipped(int x1,int y1,float z1,
                         int x2,int y2,float z2,
                         int x3,int y3,float z3,
                         Uint32 color,
                         int minX,int maxX,int minY,int maxY)
{
    int minTx = fmax(minX, fmin(x1, fmin(x2, x3)));
    int maxTx = fmin(maxX-1, fmax(x1, fmax(x2, x3)));
    int minTy = fmax(minY, fmin(y1, fmin(y2, y3)));
    int maxTy = fmin(maxY-1, fmax(y1, fmax(y2, y3)));

    float denom = (float)((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));

    for(int y = minTy; y <= maxTy; y++){
        for(int x = minTx; x <= maxTx; x++){
            float w1 = ((y2 - y3)*(x - x3) + (x3 - x2)*(y - y3)) / denom;
            float w2 = ((y3 - y1)*(x - x3) + (x1 - x3)*(y - y3)) / denom;
            float w3 = 1.0f - w1 - w2;

            if(w1 >= 0 && w2 >= 0 && w3 >= 0){
                float depth = w1*z1 + w2*z2 + w3*z3;
                int idx = y*windowWidth + x;
                if(depth < zbuffer[idx]){
                    zbuffer[idx] = depth;
                    frameBuffer[idx] = color;
                }
            }
        }
    }
}

// --- RENDER DE TERRENO Y ESFERAS ---
void renderSceneQuadrant(int minX,int maxX,int minY,int maxY,
                         SDL_Renderer* renderer,float t,
                         float lightX,float lightY,float lightZ,
                         float camX,float camY,float camZ,
                         float lookX,float lookY,float lookZ,
                         float* precomputedHeights)
{
    // --- TERRENO PARCIALMENTE VECTORIZADO ---
    for (int i = 0; i < gridSize - 1; i++) {
        for (int j = 0; j < gridSize - 1; j += 2) {
            int maxK = (gridSize - j < 2) ? (gridSize - j) : 2;
            #pragma omp simd
            for (int k = 0; k < maxK; k++) {
                int current_j = j + k;
                
                float x0 = i * SCALE, z0 = current_j * SCALE;
                float x1 = (i + 1) * SCALE, z1 = current_j * SCALE;
                float x2 = i * SCALE, z2 = (current_j + 1) * SCALE;
                float x3 = (i + 1) * SCALE, z3 = (current_j + 1) * SCALE;

                float y0 = precomputedHeights[i * gridSize + current_j];
                float y1 = precomputedHeights[(i + 1) * gridSize + current_j];
                float y2 = precomputedHeights[i * gridSize + (current_j + 1)];
                float y3 = precomputedHeights[(i + 1) * gridSize + (current_j + 1)];

                float centerX = (x0 + x1 + x2 + x3) * 0.25f;
                float centerY = (y0 + y1 + y2 + y3) * 0.25f;
                float centerZ = (z0 + z1 + z2 + z3) * 0.25f;

                float dx = centerX - camX;
                float dy = centerY - camY;
                float dz = centerZ - camZ;
                float dist2 = dx * dx + dy * dy + dz * dz;
                if (dist2 < 1.0f) continue;

                float sx0, sy0, sz0, sx1, sy1, sz1, sx2, sy2, sz2, sx3, sy3, sz3;
                project3D(camX, camY, camZ, camX + lookX, camY + lookY, camZ + lookZ,
                         x0, y0, z0, &sx0, &sy0, &sz0);
                project3D(camX, camY, camZ, camX + lookX, camY + lookY, camZ + lookZ,
                         x1, y1, z1, &sx1, &sy1, &sz1);
                project3D(camX, camY, camZ, camX + lookX, camY + lookY, camZ + lookZ,
                         x2, y2, z2, &sx2, &sy2, &sz2);
                project3D(camX, camY, camZ, camX + lookX, camY + lookY, camZ + lookZ,
                         x3, y3, z3, &sx3, &sy3, &sz3);

                float hL = (i > 0) ? precomputedHeights[(i - 1) * gridSize + current_j] : y0;
                float hR = (i < gridSize - 2) ? precomputedHeights[(i + 2) * gridSize + current_j] : y1;
                float hD = (current_j > 0) ? precomputedHeights[i * gridSize + (current_j - 1)] : y0;
                float hU = (current_j < gridSize - 2) ? precomputedHeights[i * gridSize + (current_j + 2)] : y2;

                float nx = hL - hR, ny = 2.0f, nz = hD - hU;
                float len = sqrtf(nx * nx + ny * ny + nz * nz);
                if (len > 0.0f) {
                    nx /= len; ny /= len; nz /= len;
                }

                float lx = lightX - centerX, ly = lightY - centerY, lz = lightZ - centerZ;
                float llen = sqrtf(lx * lx + ly * ly + lz * lz);
                lx /= llen; ly /= llen; lz /= llen;

                float diff = fmaxf(0.0f, nx * lx + ny * ly + nz * lz);
                float wave = 0.5f + 0.5f * sinf(t * 0.3f + (i + current_j) * 0.05f);
                
                Uint8 r = 10;
                Uint8 g = (Uint8)((50 + 150 * diff) * wave);
                Uint8 b = (Uint8)((100 + 100 * diff) * (1 - 0.3f * wave));
                Uint32 color = (r << 16) | (g << 8) | b;

                drawTriangleClipped(sx0, sy0, sz0, sx1, sy1, sz1, sx2, sy2, sz2, 
                                  color, minX, maxX, minY, maxY);
                drawTriangleClipped(sx1, sy1, sz1, sx3, sy3, sz3, sx2, sy2, sz2, 
                                  color, minX, maxX, minY, maxY);
            }
        }
    }

    // --- ESFERAS ---
    for(int i=0; i<numSpheres; i++){
        if(!spheres[i].active) continue;

        float sx,sy,depth;
        project3D(camX, camY, camZ, camX+lookX, camY+lookY, camZ+lookZ,
                  spheres[i].x,spheres[i].y,spheres[i].z,&sx,&sy,&depth);

        int radius = (int)(spheres[i].radius * windowWidth / (2*depth+1));

        for(int dy=-radius; dy<=radius; dy++){
            for(int dx=-radius; dx<=radius; dx++){
                int px = sx+dx;
                int py = sy+dy;
                if(px<minX || px>=maxX || py<minY || py>=maxY) continue;
                if(dx*dx + dy*dy <= radius*radius){
                    int idx = py*windowWidth + px;
                    float z = depth;
                    if(z < zbuffer[idx]){
                        zbuffer[idx] = z;
                        
                        float nx = dx/(float)radius;
                        float ny = -dy/(float)radius;
                        float nz = sqrtf(fmaxf(0.0f,1-nx*nx-ny*ny));
                        float px3D = spheres[i].x + nx*spheres[i].radius;
                        float py3D = spheres[i].y + ny*spheres[i].radius;
                        float pz3D = spheres[i].z + nz*spheres[i].radius;

                        float lx = lightX-px3D, ly = lightY-py3D, lz = lightZ-pz3D;
                        float len = sqrtf(lx*lx+ly*ly+lz*lz);
                        lx/=len; ly/=len; lz/=len;

                        float diff = fmaxf(0.0f, nx*lx + ny*ly + nz*lz);
                        Uint8 r = (Uint8)(spheres[i].r*255*diff);
                        Uint8 g = (Uint8)(spheres[i].g*255*diff);
                        Uint8 b = (Uint8)(spheres[i].b*255*diff);
                        frameBuffer[idx] = (r<<16)|(g<<8)|b;
                    }
                }
            }
        }
    }
}

void renderScene(SDL_Renderer* renderer,float t,
                 float lightX,float lightY,float lightZ,
                 float camX,float camY,float camZ,
                 float lookX,float lookY,float lookZ,
                float* precomputedHeights)
{
    int numThreads = omp_get_max_threads();

    // Calcular divisiones aproximadas: filas y columnas
    int nCols = 2;  // Empezar con 2 columnas
    int nRows = omp_get_max_threads() / nCols;
    if (nRows * nCols < omp_get_max_threads()) nRows++;

    // Usar chunks más grandes para reducir overhead
    #pragma omp parallel for collapse(2) schedule(static, 1)
    for(int row=0; row<nRows; row++){
        for(int col=0; col<nCols; col++){
            int tid = row*nCols + col;
            if(tid >= numThreads) continue; // si hay más cuadrantes que hilos

            int minX = col * windowWidth / nCols;
            int maxX = (col+1) * windowWidth / nCols;
            int minY = row * windowHeight / nRows;
            int maxY = (row+1) * windowHeight / nRows;

            renderSceneQuadrant(minX, maxX, minY, maxY,
                                renderer, t,
                                lightX, lightY, lightZ,
                                camX, camY, camZ,
                                lookX, lookY, lookZ,
                            precomputedHeights);
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
            *camX = centerX - 20.0f;    // Posicionada a la izquierda
            *camY = 35.0f;              // Alta enough para ver todo
            *camZ = centerZ - 20.0f;    // Posicionada atrás
            
            // Mirar hacia el centro del terreno con ángulo oblicuo
            *lookX = centerX - *camX;
            *lookY = 5.0f - *camY;      // Mirar ligeramente hacia abajo
            *lookZ = centerZ - *camZ;
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

    FILE* logFile = fopen("fps_log_paralelo.txt", "w");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Olas - SDL Texture",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        windowWidth,windowHeight,SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    // INICIALIZAR TEXTURA Y BUFFERS PERSISTENTES
    screenTexture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING,
        windowWidth, windowHeight);
    
    initRenderBuffers();  // Crear buffers una sola vez
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
                resizeRenderBuffers(renderer);  // Manejar redimensionamiento
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

        float* precomputedHeights = malloc(gridSize * gridSize * sizeof(float));
        #pragma omp parallel for collapse(2)
        for(int i = 0; i < gridSize; i++) {
            for(int j = 0; j < gridSize; j++) {
                float x = i * SCALE;
                float z = j * SCALE;
                precomputedHeights[i * gridSize + j] = waveHeight(x, z, t);
            }
        }

        renderScene(renderer,t,lightX,lightY,lightZ,
                    camX,camY,camZ,lookX,lookY,lookZ,
                    precomputedHeights);

        free(precomputedHeights);

        // Actualizar textura con el framebuffer
        SDL_UpdateTexture(screenTexture, NULL, frameBuffer, windowWidth * sizeof(Uint32));
        SDL_RenderCopy(renderer, screenTexture, NULL, NULL);

        SDL_RenderPresent(renderer);

        // Cálculo y mostrar FPS en el título
        float fps = 1.0f / deltaTime;
        fprintf(logFile, "%.2f\n", fps);
        fflush(logFile);
        sprintf(title, "Olas PARALELO - FPS: %.2f - Esferas: %d", fps, spawned);
        SDL_SetWindowTitle(window, title);

        SDL_Delay(16);  // Limitar a ~60 FPS
        t += 0.05f;     // Avanzar tiempo de animación
    }

    fclose(logFile);
    freeRenderBuffers();
    if(screenTexture) SDL_DestroyTexture(screenTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}