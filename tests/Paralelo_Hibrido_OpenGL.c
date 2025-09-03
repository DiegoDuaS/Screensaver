#include <SDL2/SDL.h> 
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Constantes de configuración
#define GRID_SIZE 100          
#define SCALE 1.0f             
#define DEF_SPHERES 15000000        
#define GRAVITY -0.02f         
#define BOUNCE 0.7f            
#define SPAWN_INTERVAL 1       

// ← SIMPLIFICADO: Solo optimizaciones que realmente funcionan
#define COLLISION_CHUNK_SIZE 64    // Tamaño de chunk para balance de carga
#define TERRAIN_CHUNK_SIZE 16      // Chunks para terreno

// Estructura que define una esfera con física
typedef struct {
    float x,y,z;      
    float vx,vz,vy;   
    float radius;     
    float r,g,b;      
    int active;
} Sphere;

// ← OPTIMIZADO: Estructura ligera para pre-cálculos del terreno
typedef struct {
    float h1, h2, h3, h4;  // Alturas de las 4 esquinas
    float r, g, b;         // Color del quad
} TerrainQuad;

Sphere spheres[DEF_SPHERES];  
TerrainQuad terrainData[GRID_SIZE-1][GRID_SIZE-1];

// Variables globales de configuración
int numSpheres = 100000;       
float waveAmplitude = 2.0f;   
float waveFrequency = 1.0f;   
int windowWidth = 1024;
int windowHeight = 768;

// ← AÑADIDO: Variables para métricas simples
double time_terrain = 0.0;
double time_physics = 0.0;
double time_collisions = 0.0;
int collision_checks = 0;
int detected_collisions = 0;

// ← OPTIMIZADO: Versión más eficiente de waveHeight
float waveHeight(float x, float z, float t){
    // Pre-calcular términos comunes para evitar recálculo
    float freq_x = x * waveFrequency;
    float freq_z = z * waveFrequency;
    
    return waveAmplitude * (
        1.5f * sinf(0.3f * freq_x + t) + 
        1.0f * cosf(0.4f * freq_z + 0.5f * t) +
        0.7f * sinf(0.2f * (freq_x + freq_z) + 0.8f * t)
    );
}

// ← OPTIMIZADO: Cálculo de color más eficiente
void getTerrainColor(int i, int j, float t, float *r, float *g, float *b){
    // Usar operaciones enteras cuando sea posible
    float phase = t + (i + j) * 0.1f;
    *r = 0.2f + 0.1f * sinf(phase);
    *g = 0.5f + 0.3f * sinf(phase + 1.0f);
    *b = 0.7f + 0.2f * cosf(phase + 2.0f);
}

// ← SIMPLIFICADO: Pre-cálculo eficiente sin complejidad innecesaria
void updateTerrainData(float t){
    double start_time = omp_get_wtime();
    
    // ← OPTIMIZACIÓN CLAVE: schedule(static) con chunks grandes para reducir overhead
    #pragma omp parallel for collapse(2) schedule(static, TERRAIN_CHUNK_SIZE)
    for(int i = 0; i < GRID_SIZE - 1; i++) {
        for(int j = 0; j < GRID_SIZE - 1; j++) {
            TerrainQuad* quad = &terrainData[i][j];
            
            // Calcular alturas de las 4 esquinas del quad
            quad->h1 = waveHeight(i * SCALE, j * SCALE, t);
            quad->h2 = waveHeight((i + 1) * SCALE, j * SCALE, t);
            quad->h3 = waveHeight((i + 1) * SCALE, (j + 1) * SCALE, t);
            quad->h4 = waveHeight(i * SCALE, (j + 1) * SCALE, t);

            // Calcular color del quad
            getTerrainColor(i, j, t, &quad->r, &quad->g, &quad->b);
        }
    }
    
    time_terrain = omp_get_wtime() - start_time;
}

// ← HÍBRIDO: Física básica optimizada sin overhead innecesario  
void updateBasicPhysics(float t){
    #pragma omp parallel for schedule(static, 512)
    for(int i = 0; i < numSpheres; i++){
        if(!spheres[i].active) continue;
        
        // Actualización de posición vectorizada
        spheres[i].x += spheres[i].vx;
        spheres[i].z += spheres[i].vz;
        spheres[i].vy += GRAVITY;
        spheres[i].y += spheres[i].vy;

        // Colisión con el terreno ondulado
        float floorY = waveHeight(spheres[i].x, spheres[i].z, t) + spheres[i].radius;
        if(spheres[i].y < floorY){
            spheres[i].y = floorY;
            spheres[i].vy = -spheres[i].vy * BOUNCE;
        }

        // Rebote en los bordes del terreno
        if(spheres[i].x < 0.0f || spheres[i].x > GRID_SIZE*SCALE) spheres[i].vx *= -1.0f;
        if(spheres[i].z < 0.0f || spheres[i].z > GRID_SIZE*SCALE) spheres[i].vz *= -1.0f;
    }
}

// ← MEJORADO: Algoritmo de colisiones híbrido (simple pero efectivo)
void updateCollisions(){
    double start_time = omp_get_wtime();
    collision_checks = 0;
    detected_collisions = 0;
    
    // ← ESTRATEGIA HÍBRIDA: Paralelizar por chunks con menos overhead
    #pragma omp parallel for schedule(dynamic, COLLISION_CHUNK_SIZE) \
        reduction(+:collision_checks, detected_collisions)
    for(int i = 0; i < numSpheres - 1; i++){
        if(!spheres[i].active) continue;
        
        // ← OPTIMIZACIÓN: Límite de búsqueda local para reducir complejidad
        int max_j = fmin(numSpheres, i + 200);  // Solo verificar las siguientes 200 esferas
        
        for(int j = i + 1; j < max_j; j++){
            if(!spheres[j].active) continue;
            
            collision_checks++;
            
            // ← OPTIMIZACIÓN: Pre-filtro por distancia rápida
            float dx = spheres[j].x - spheres[i].x;
            float dz = spheres[j].z - spheres[i].z;
            float dist_sq_2d = dx*dx + dz*dz;
            
            // Solo calcular distancia 3D si están cerca en 2D
            if(dist_sq_2d < 4.0f) {  // Pre-filtro: solo si están a menos de 2 unidades en XZ
                float dy = spheres[j].y - spheres[i].y;
                float dist_sq = dist_sq_2d + dy*dy;
                float minDist = spheres[i].radius + spheres[j].radius;
                float minDist_sq = minDist * minDist;

                if(dist_sq < minDist_sq && dist_sq > 0.0001f) {
                    detected_collisions++;
                    
                    float dist = sqrtf(dist_sq);
                    float nx = dx / dist;
                    float ny = dy / dist;
                    float nz = dz / dist;
                    float overlap = minDist - dist;

                    // ← SIN BUFFERS: Aplicar cambios directamente (más rápido)
                    float pos_correction = overlap * 0.5f;
                    spheres[i].x -= nx * pos_correction;
                    spheres[i].y -= ny * pos_correction;
                    spheres[i].z -= nz * pos_correction;
                    spheres[j].x += nx * pos_correction;
                    spheres[j].y += ny * pos_correction;
                    spheres[j].z += nz * pos_correction;

                    // Intercambio de velocidades simplificado
                    float viDot = spheres[i].vx*nx + spheres[i].vy*ny + spheres[i].vz*nz;
                    float vjDot = spheres[j].vx*nx + spheres[j].vy*ny + spheres[j].vz*nz;
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
    
    time_collisions = omp_get_wtime() - start_time;
}

// ← SIMPLIFICADO: Pipeline simple y efectivo
void updatePhysics(float t) {
    double start_time = omp_get_wtime();
    
    // Ejecutar en secuencia optimizada (evitar overhead de sincronización)
    updateBasicPhysics(t);
    updateCollisions();
    
    time_physics = omp_get_wtime() - start_time;
}

// Renderiza el terreno usando los datos pre-calculados
void renderTerrain(){
    for(int i = 0; i < GRID_SIZE - 1; i++) {
        for(int j = 0; j < GRID_SIZE - 1; j++) {
            TerrainQuad* quad = &terrainData[i][j];
            
            glColor3f(quad->r, quad->g, quad->b);
            glBegin(GL_QUADS);
                glNormal3f(0, 1, 0);
                glVertex3f(i * SCALE, quad->h1, j * SCALE);
                glVertex3f((i + 1) * SCALE, quad->h2, j * SCALE);
                glVertex3f((i + 1) * SCALE, quad->h3, (j + 1) * SCALE);
                glVertex3f(i * SCALE, quad->h4, (j + 1) * SCALE);
            glEnd();
        }
    }
}

// ← OPTIMIZADO: Renderizado de esferas con menos subdivisiones
void renderSpheres(){
    for(int i=0;i<numSpheres;i++){
        if(!spheres[i].active) continue;
        
        glPushMatrix();
        glTranslatef(spheres[i].x,spheres[i].y,spheres[i].z);

        GLfloat matDiffuse[] = { spheres[i].r, spheres[i].g, spheres[i].b, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matDiffuse);

        GLUquadric* q = gluNewQuadric();
        gluQuadricNormals(q, GLU_SMOOTH);
        gluSphere(q, spheres[i].radius, 12, 12);  // ← REDUCIDO: de 32x32 a 12x12
        gluDeleteQuadric(q);

        glPopMatrix();
    }
}

// Inicializa la configuración de OpenGL
void initOpenGL(){
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightPos[] = { 20.0f, 100.0f, 30.0f, 1.0f };
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
    FILE* logFile = fopen("fps_log_paralelo.txt", "w");
    Uint32 startTime = SDL_GetTicks();
    int maxDuration = 10000;   
    if(argc > 1) numSpheres = atof(argv[1]);
    if(argc > 2) waveAmplitude = atof(argv[2]);
    if(argc > 3) waveFrequency = atof(argv[3]);
    if(numSpheres > DEF_SPHERES) numSpheres = DEF_SPHERES;

    printf("=== CONFIGURACIÓN HÍBRIDA OPTIMIZADA ===\n");
    printf("Hilos OpenMP: %d\n", omp_get_max_threads());
    printf("Esferas: %d\n", numSpheres);
    printf("Chunk colisiones: %d\n", COLLISION_CHUNK_SIZE);
    printf("Chunk terreno: %d\n", TERRAIN_CHUNK_SIZE);
    printf("========================================\n");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Olas con Esferas - PARALELO HÍBRIDO",
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
    int viewMode = 0;

    // ← OPTIMIZADO: Inicialización paralela más eficiente
    srand((unsigned int)time(NULL));
    #pragma omp parallel for schedule(static, 1024)
    for(int i=0;i<numSpheres;i++){
        unsigned int seed = (unsigned int)time(NULL) + i + omp_get_thread_num() * 12345;
        
        spheres[i].x = (rand_r(&seed) % GRID_SIZE) * SCALE;
        spheres[i].z = (rand_r(&seed) % GRID_SIZE) * SCALE;
        spheres[i].y = 20.0f + ((float)rand_r(&seed) / RAND_MAX) * 60.0f;
        spheres[i].vx = ((rand_r(&seed)%100)/100.0f -0.5f)*0.2f;
        spheres[i].vz = ((rand_r(&seed)%100)/100.0f -0.5f)*0.2f;
        spheres[i].vy = 0.0f;
        spheres[i].radius = 0.5f;
        spheres[i].r = 0.3f + ((rand_r(&seed)%100)/100.0f)*0.7f;
        spheres[i].g = 0.3f + ((rand_r(&seed)%100)/100.0f)*0.7f;
        spheres[i].b = 0.3f + ((rand_r(&seed)%100)/100.0f)*0.7f;
        spheres[i].active = 0;
    }

    // Pre-calcular datos iniciales del terreno
    updateTerrainData(0.0f);

    int running = 1;
    SDL_Event event;
    float t = 0.0f;

    Uint32 lastTime = SDL_GetTicks();
    Uint32 lastSpawn = lastTime;
    int spawned = 0;

    char title[256];
    int frame_count = 0;
    double total_physics_time = 0.0;
    double total_terrain_time = 0.0;

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) running = 0;
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED){
                reshape(event.window.data1, event.window.data2);
            }
            if(event.type == SDL_KEYDOWN){
                switch(event.key.keysym.sym){
                    case SDLK_1: viewMode = 0; break;
                    case SDLK_2: viewMode = 1; break;
                    case SDLK_3: viewMode = 2; break;
                    case SDLK_p: 
                        printf("\n=== ESTADÍSTICAS HÍBRIDAS ===\n");
                        printf("Tiempo terreno: %.3f ms\n", time_terrain * 1000.0);
                        printf("Tiempo física: %.3f ms\n", time_physics * 1000.0);
                        printf("Tiempo colisiones: %.3f ms\n", time_collisions * 1000.0);
                        printf("Verificaciones: %d\n", collision_checks);
                        printf("Colisiones detectadas: %d\n", detected_collisions);
                        printf("Eficiencia: %.2f%%\n", 
                               collision_checks > 0 ? (100.0 * detected_collisions / collision_checks) : 0.0);
                        printf("============================\n");
                        break;
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f;
        lastTime = now;

        if(now - lastSpawn >= SPAWN_INTERVAL && spawned < numSpheres){
            spheres[spawned].active = 1;
            spawned++;
            lastSpawn = now;
        }

        // Actualización de cámara según modo
        if(viewMode == 0){
            yaw += camSpeed;
            camX = centerX + radius * sinf(yaw);
            camZ = centerZ + radius * cosf(yaw);
            camY = 15.0f;
            lookX = centerX - camX;
            lookY = -camY;
            lookZ = centerZ - camZ;
        } else if(viewMode == 1){
            camX = centerX; 
            camZ = centerZ;
            camY = 90.0f;
            lookX = 0.0f;
            lookY = -55.0f;
            lookZ = -0.8f;
        } else if(viewMode == 2){
            camX = -20.0f;
            camZ = centerZ;
            camY = 20.0f;
            lookX = centerX + 20.0f;
            lookY = -20.0f;
            lookZ = centerZ - camZ;
        }

        // ← ACTUALIZACIÓN CADA 3 FRAMES para reducir overhead
        if(frame_count % 3 == 0) {
            updateTerrainData(t);
            total_terrain_time += time_terrain;
        }
        
        // Física en cada frame
        updatePhysics(t);
        total_physics_time += time_physics;
        frame_count++;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        
        gluLookAt(camX, camY, camZ,
            camX + lookX, camY + lookY, camZ + lookZ,
            0,1,0);

        renderTerrain();
        renderSpheres();

        SDL_GL_SwapWindow(window);

        // Mostrar estadísticas optimizadas
        float fps = 1.0f / deltaTime;
        double avg_physics = (frame_count > 0) ? (total_physics_time / frame_count) : 0.0;
        
        sprintf(title, "HÍBRIDO - FPS: %.1f | Física: %.2fms | Colisiones: %d/%d | Hilos: %d", 
                fps, avg_physics * 1000.0, detected_collisions, collision_checks, omp_get_max_threads());
        SDL_SetWindowTitle(window, title);
        fprintf(logFile, "%.2f\n", fps);
        fflush(logFile);

        SDL_Delay(16);
        t += 0.05f;
        if (now - startTime >= maxDuration) {
        running = 0;  // salir del bucle tras 10 segundos
        }
    }

    fclose(logFile);


    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}