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
    float x, y, z;
    float vx, vy, vz;
    float radius;
    float r, g, b;
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
float waveHeight(float x, float z, float t) {
    return waveAmplitude * (
        1.5f * sinf(0.3f * x * waveFrequency + t) +
        1.0f * cosf(0.4f * z * waveFrequency + 0.5f * t) +
        0.7f * sinf(0.2f * (x + z) * waveFrequency + 0.8f * t)
    );
}

void project3D(float camX, float camY, float camZ, float lookX, float lookY, float lookZ,
               float x, float y, float z, float *sx, float *sy, float *depth) {
    float rx = x - camX;
    float ry = y - camY;
    float rz = z - camZ;

    float lx = lookX - camX;
    float lz = lookZ - camZ;

    float angle = atan2f(lx, lz);
    float ca = cosf(angle);
    float sa = sinf(angle);
    float tx = ca * rx - sa * rz;
    float tz = sa * rx + ca * rz;
    float ty = ry;

    float fov = 500.0f;
    if (tz <= 0.1f) tz = 0.1f;
    *sx = windowWidth / 2 + tx * fov / tz;
    *sy = windowHeight / 2 - ty * fov / tz;
    *depth = tz;
}

// funciones para gestión de buffers 
void initRenderBuffers() {
    frameBuffer = malloc(windowWidth * windowHeight * sizeof(Uint32));
    zbuffer = malloc(windowWidth * windowHeight * sizeof(float));
}

void freeRenderBuffers() {
    if (frameBuffer) { free(frameBuffer); frameBuffer = NULL; }
    if (zbuffer) { free(zbuffer); zbuffer = NULL; }
}

void resizeRenderBuffers(SDL_Renderer* renderer) {
    freeRenderBuffers();
    
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture);
    }
    screenTexture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING,
        windowWidth, windowHeight);
    
    initRenderBuffers();
}

// -----------------------------------------------------------------------------
// Inicialización de esferas
// -----------------------------------------------------------------------------
void initSpheres(int n) {
    srand((unsigned int)time(NULL));
    if (n > DEF_SPHERES) n = DEF_SPHERES;
    numSpheres = n;

    for (int i = 0; i < numSpheres; i++) {
        spheres[i].x = (rand() % gridSize) * SCALE;
        spheres[i].z = (rand() % gridSize) * SCALE;
        spheres[i].y = 20.0f + ((float)rand() / RAND_MAX) * 60.0f;
        spheres[i].vx = ((rand() % 100) / 100.0f - 0.5f) * 0.2f;
        spheres[i].vz = ((rand() % 100) / 100.0f - 0.5f) * 0.2f;
        spheres[i].vy = 0;
        spheres[i].radius = 0.5f;
        spheres[i].r = 0.3f + ((rand() % 100) / 100.0f) * 0.7f;
        spheres[i].g = 0.3f + ((rand() % 100) / 100.0f) * 0.7f;
        spheres[i].b = 0.3f + ((rand() % 100) / 100.0f) * 0.7f;
        spheres[i].active = 1;
    }
}

// -----------------------------------------------------------------------------
// Física de esferas y colisiones SECUENCIAL
// -----------------------------------------------------------------------------
void updatePhysics(float t) {
    // Movimiento y rebotes SECUENCIAL
    for (int i = 0; i < numSpheres; i++) {
        if (!spheres[i].active) continue;
        
        spheres[i].x += spheres[i].vx;
        spheres[i].z += spheres[i].vz;
        spheres[i].vy += GRAVITY;
        spheres[i].y += spheres[i].vy;

        float floorY = waveHeight(spheres[i].x, spheres[i].z, t) + spheres[i].radius;
        
        if (spheres[i].y < floorY) {
            spheres[i].y = floorY;
            spheres[i].vy *= -BOUNCE;
        }
        
        if (spheres[i].x < 0 || spheres[i].x > gridSize * SCALE) 
            spheres[i].vx *= -1;
        if (spheres[i].z < 0 || spheres[i].z > gridSize * SCALE) 
            spheres[i].vz *= -1;
    }

    // Colisiones entre esferas SECUENCIAL
    for (int i = 0; i < numSpheres; i++) {
        if (!spheres[i].active) continue;
        
        for (int j = i + 1; j < numSpheres; j++) {
            if (!spheres[j].active) continue;

            float dx = spheres[j].x - spheres[i].x;
            float dy = spheres[j].y - spheres[i].y;
            float dz = spheres[j].z - spheres[i].z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            float minDist = spheres[i].radius + spheres[j].radius;

            if (dist < minDist && dist > 0.0f) {
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
                float overlap = minDist - dist;

                spheres[i].x -= nx * overlap * 0.5f;
                spheres[i].y -= ny * overlap * 0.5f;
                spheres[i].z -= nz * overlap * 0.5f;
                spheres[j].x += nx * overlap * 0.5f;
                spheres[j].y += ny * overlap * 0.5f;
                spheres[j].z += nz * overlap * 0.5f;

                float viDot = spheres[i].vx * nx + spheres[i].vy * ny + spheres[i].vz * nz;
                float vjDot = spheres[j].vx * nx + spheres[j].vy * ny + spheres[j].vz * nz;
                float avg = (viDot + vjDot) * 0.5f;

                spheres[i].vx += (avg - viDot) * nx;
                spheres[i].vy += (avg - viDot) * ny;
                spheres[i].vz += (avg - viDot) * nz;

                spheres[j].vx += (avg - vjDot) * nx;
                spheres[j].vy += (avg - vjDot) * ny;
                spheres[j].vz += (avg - vjDot) * nz;
            }
        }
    }
}

// Reset Z-buffer SECUENCIAL
void resetZBuffer() {
    for (int i = 0; i < windowWidth * windowHeight; i++) {
        zbuffer[i] = 1e30f;
        frameBuffer[i] = 0;
    }
}

// -----------------------------------------------------------------------------
// Actualizar posición de la cámara
// -----------------------------------------------------------------------------
void updateCamera(float centerX, float centerZ, float radius, float *camX, float *camY, float *camZ,
                  float *lookX, float *lookY, float *lookZ, float *yaw, float camSpeed) {
    *yaw += camSpeed;
    *camX = centerX + radius * sinf(*yaw);
    *camZ = centerZ + radius * cosf(*yaw);
    *camY = 15.0f;
    *lookX = centerX - *camX;
    *lookY = -(*camY);
    *lookZ = centerZ - *camZ;
}

// Versión de drawTriangle que recorta a un cuadrante
void drawTriangleClipped(int x1, int y1, float z1,
                         int x2, int y2, float z2,
                         int x3, int y3, float z3,
                         Uint32 color,
                         int minX, int maxX, int minY, int maxY) {
    int minTx = fmax(minX, fmin(x1, fmin(x2, x3)));
    int maxTx = fmin(maxX - 1, fmax(x1, fmax(x2, x3)));
    int minTy = fmax(minY, fmin(y1, fmin(y2, y3)));
    int maxTy = fmin(maxY - 1, fmax(y1, fmax(y2, y3)));

    float denom = (float)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));

    for (int y = minTy; y <= maxTy; y++) {
        for (int x = minTx; x <= maxTx; x++) {
            float w1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / denom;
            float w2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / denom;
            float w3 = 1.0f - w1 - w2;

            if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                float depth = w1 * z1 + w2 * z2 + w3 * z3;
                int idx = y * windowWidth + x;
                if (depth < zbuffer[idx]) {
                    zbuffer[idx] = depth;
                    frameBuffer[idx] = color;
                }
            }
        }
    }
}

// --- RENDER DE TERRENO Y ESFERAS SECUENCIAL ---
void renderSceneQuadrant(int minX, int maxX, int minY, int maxY,
                         SDL_Renderer* renderer, float t,
                         float lightX, float lightY, float lightZ,
                         float camX, float camY, float camZ,
                         float lookX, float lookY, float lookZ,
                         float* precomputedHeights) {
    // --- TERRENO SECUENCIAL ---
    for (int i = 0; i < gridSize - 1; i++) {
        for (int j = 0; j < gridSize - 1; j++) {
            float x0 = i * SCALE, z0 = j * SCALE;
            float x1 = (i + 1) * SCALE, z1 = j * SCALE;
            float x2 = i * SCALE, z2 = (j + 1) * SCALE;
            float x3 = (i + 1) * SCALE, z3 = (j + 1) * SCALE;

            float y0 = precomputedHeights[i * gridSize + j];
            float y1 = precomputedHeights[(i + 1) * gridSize + j];
            float y2 = precomputedHeights[i * gridSize + (j + 1)];
            float y3 = precomputedHeights[(i + 1) * gridSize + (j + 1)];

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

            float hL = (i > 0) ? precomputedHeights[(i - 1) * gridSize + j] : y0;
            float hR = (i < gridSize - 2) ? precomputedHeights[(i + 2) * gridSize + j] : y1;
            float hD = (j > 0) ? precomputedHeights[i * gridSize + (j - 1)] : y0;
            float hU = (j < gridSize - 2) ? precomputedHeights[i * gridSize + (j + 2)] : y2;

            float nx = hL - hR, ny = 2.0f, nz = hD - hU;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0.0f) {
                nx /= len; ny /= len; nz /= len;
            }

            float lx = lightX - centerX, ly = lightY - centerY, lz = lightZ - centerZ;
            float llen = sqrtf(lx * lx + ly * ly + lz * lz);
            lx /= llen; ly /= llen; lz /= llen;

            float diff = fmaxf(0.0f, nx * lx + ny * ly + nz * lz);
            float wave = 0.5f + 0.5f * sinf(t * 0.3f + (i + j) * 0.05f);
            
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

    // --- ESFERAS SECUENCIAL ---
    for (int i = 0; i < numSpheres; i++) {
        if (!spheres[i].active) continue;

        float sx, sy, depth;
        project3D(camX, camY, camZ, camX + lookX, camY + lookY, camZ + lookZ,
                  spheres[i].x, spheres[i].y, spheres[i].z, &sx, &sy, &depth);

        int radius = (int)(spheres[i].radius * windowWidth / (2 * depth + 1));

        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int px = sx + dx;
                int py = sy + dy;
                if (px < minX || px >= maxX || py < minY || py >= maxY) continue;
                
                if (dx * dx + dy * dy <= radius * radius) {
                    int idx = py * windowWidth + px;
                    if (depth < zbuffer[idx]) {
                        zbuffer[idx] = depth;
                        
                        float nx = dx / (float)radius;
                        float ny = -dy / (float)radius;
                        float nz = sqrtf(fmaxf(0.0f, 1 - nx * nx - ny * ny));
                        float px3D = spheres[i].x + nx * spheres[i].radius;
                        float py3D = spheres[i].y + ny * spheres[i].radius;
                        float pz3D = spheres[i].z + nz * spheres[i].radius;

                        float lx = lightX - px3D, ly = lightY - py3D, lz = lightZ - pz3D;
                        float len = sqrtf(lx * lx + ly * ly + lz * lz);
                        lx /= len; ly /= len; lz /= len;

                        float diff = fmaxf(0.0f, nx * lx + ny * ly + nz * lz);
                        Uint8 r = (Uint8)(spheres[i].r * 255 * diff);
                        Uint8 g = (Uint8)(spheres[i].g * 255 * diff);
                        Uint8 b = (Uint8)(spheres[i].b * 255 * diff);
                        frameBuffer[idx] = (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Render scene completo SECUENCIAL
// -----------------------------------------------------------------------------
void renderScene(SDL_Renderer* renderer, float t,
                 float lightX, float lightY, float lightZ,
                 float camX, float camY, float camZ,
                 float lookX, float lookY, float lookZ,
                 float* precomputedHeights) {
    // Renderizar todo el framebuffer secuencialmente
    renderSceneQuadrant(0, windowWidth, 0, windowHeight,
                       renderer, t,
                       lightX, lightY, lightZ,
                       camX, camY, camZ,
                       lookX, lookY, lookZ,
                       precomputedHeights);
}

// -----------------------------------------------------------------------------
// Actualizar posición de la cámara según el modo
// -----------------------------------------------------------------------------
void updateCameraView(int viewMode, float centerX, float centerZ, float radius, 
                     float *camX, float *camY, float *camZ,
                     float *lookX, float *lookY, float *lookZ, float *yaw) {
    switch(viewMode) {
        case 1: // Rotando alrededor del centro
            *yaw += 0.01f;
            *camX = centerX + radius * sinf(*yaw);
            *camZ = centerZ + radius * cosf(*yaw);
            *camY = 10.0f;
            *lookX = centerX - *camX;
            *lookY = -(*camY);
            *lookZ = centerZ - *camZ;
            break;
            
        case 2: // Vista desde el cielo
            *camX = centerX - 20.0f;
            *camY = 35.0f;
            *camZ = centerZ - 20.0f;
            *lookX = centerX - *camX;
            *lookY = 5.0f - *camY;
            *lookZ = centerZ - *camZ;
            break;
            
        case 3: // Vista lateral fija
            *camX = -20.0f;
            *camY = 10.0f;
            *camZ = centerZ;
            *lookX = centerX + 20.0f;
            *lookY = -(*camY);
            *lookZ = centerZ - *camZ;
            break;
            
        default: // fallback
            *camX = centerX + radius * sinf(*yaw);
            *camZ = centerZ + radius * cosf(*yaw);
            *camY = 15.0f;
            *lookX = centerX - *camX;
            *lookY = -(*camY);
            *lookZ = centerZ - *camZ;
            break;
    }
}

// -----------------------------------------------------------------------------
// MAIN SECUENCIAL
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc > 1) numSpheres = atoi(argv[1]);
    if (numSpheres <= 0) numSpheres = DEF_SPHERES;

    if (argc > 2) gridSize = atof(argv[2]);
    if (gridSize < GRID_SIZE) gridSize = GRID_SIZE;

    FILE* logFile = fopen("fps_log_secuencial.txt", "w");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Olas - SDL Texture (SECUENCIAL)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    screenTexture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING,
        windowWidth, windowHeight);
    
    initRenderBuffers();
    initSpheres(numSpheres);

    float centerX = gridSize * SCALE / 2;
    float centerZ = gridSize * SCALE / 2;
    float radius = 10.0f;
    float yaw = 0.0f;
    float camX, camY, camZ;
    float lookX, lookY, lookZ;

    float lightX = centerX + 30.0f;
    float lightY = 25.0f;
    float lightZ = centerZ + 30.0f;

    int running = 1;
    SDL_Event event;
    float t = 0;
    Uint32 lastTime = SDL_GetTicks();
    Uint32 lastSpawn = lastTime;
    int spawned = 0;

    int viewMode = 1;

    char title[128];

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_1) viewMode = 1;
                if (event.key.keysym.sym == SDLK_2) viewMode = 2;
                if (event.key.keysym.sym == SDLK_3) viewMode = 3;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;
                resizeRenderBuffers(renderer);
            }
        }

        Uint32 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f;
        lastTime = now;

        if (now - lastSpawn >= SPAWN_INTERVAL && spawned < numSpheres) {
            spheres[spawned].active = 1;
            spawned++;
            lastSpawn = now;
        }

        updateCameraView(viewMode, centerX, centerZ, radius, &camX, &camY, &camZ, 
                        &lookX, &lookY, &lookZ, &yaw);
        
        updatePhysics(t);
        resetZBuffer();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Precalculo de alturas SECUENCIAL
        float* precomputedHeights = malloc(gridSize * gridSize * sizeof(float));
        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                float x = i * SCALE;
                float z = j * SCALE;
                precomputedHeights[i * gridSize + j] = waveHeight(x, z, t);
            }
        }

        renderScene(renderer, t, lightX, lightY, lightZ,
                   camX, camY, camZ, lookX, lookY, lookZ,
                   precomputedHeights);

        free(precomputedHeights);

        SDL_UpdateTexture(screenTexture, NULL, frameBuffer, windowWidth * sizeof(Uint32));
        SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
        SDL_RenderPresent(renderer);

        float fps = 1.0f / deltaTime;
        fprintf(logFile, "%.2f\n", fps);
        fflush(logFile);
        sprintf(title, "Olas SECUENCIAL - FPS: %.2f - Esferas: %d", fps, spawned);
        SDL_SetWindowTitle(window, title);

        SDL_Delay(16);
        t += 0.05f;
    }

    fclose(logFile);
    freeRenderBuffers();
    if (screenTexture) SDL_DestroyTexture(screenTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}