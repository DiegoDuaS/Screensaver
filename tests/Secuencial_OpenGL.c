#include <SDL2/SDL.h> 
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Constantes de configuración
#define GRID_SIZE 100          // Tamaño de la grilla del terreno
#define SCALE 1.0f             // Escala de cada celda de la grilla
#define DEF_SPHERES 15000000        // Número máximo de esferas
#define GRAVITY -0.02f         // Fuerza de gravedad aplicada a las esferas
#define BOUNCE 0.7f            // Factor de rebote (0-1, donde 1 es rebote perfecto)
#define SPAWN_INTERVAL 1    // Tiempo entre aparición de esferas en milisegundos

// Estructura que define una esfera con física
typedef struct {
    float x,y,z;      // Posición en 3D
    float vx,vz,vy;   // Velocidad en cada eje
    float radius;     // Radio de la esfera
    float r,g,b;      // Color RGB
    int active;       // Bandera para saber si la esfera está activa
} Sphere;

Sphere spheres[DEF_SPHERES];  // Array de todas las esferas

// Variables globales de configuración
int numSpheres = 100000;       
float waveAmplitude = 2.0f;   // Amplitud de las olas
float waveFrequency = 1.0f;   // Frecuencia de las olas
int windowWidth = 1024;
int windowHeight = 768;

// Calcula la altura del terreno ondulado en una posición (x,z) y tiempo t
float waveHeight(float x, float z, float t){
    // Combina múltiples ondas sinusoidales para crear un patrón complejo
    return waveAmplitude * (
        1.5f*sinf(0.3f*x*waveFrequency + t) + 
        1.0f*cosf(0.4f*z*waveFrequency + 0.5f*t) +
        0.7f*sinf(0.2f*(x+z)*waveFrequency + 0.8f*t)
    );
}

// Calcula el color del terreno basado en posición y tiempo (efecto dinámico)
void getTerrainColor(float x, float z, float t, float *r, float *g, float *b){
    *r = 0.2f + 0.1f * sinf(t + x*0.1f);
    *g = 0.5f + 0.3f * sinf(t + z*0.1f);
    *b = 0.7f + 0.2f * cosf(t + (x+z)*0.05f);
}

// Renderiza el terreno ondulado usando quads
void renderTerrain(float t){
    // Recorre cada celda de la grilla
    for(int i=0;i<GRID_SIZE-1;i++){
        for(int j=0;j<GRID_SIZE-1;j++){
            // Calcula la altura en las 4 esquinas del quad
            float h1 = waveHeight(i*SCALE,j*SCALE,t);
            float h2 = waveHeight((i+1)*SCALE,j*SCALE,t);
            float h3 = waveHeight((i+1)*SCALE,(j+1)*SCALE,t);
            float h4 = waveHeight(i*SCALE,(j+1)*SCALE,t);

            // Obtiene el color dinámico para esta celda
            float r,g,b;
            getTerrainColor(i,j,t,&r,&g,&b);
            glColor3f(r,g,b);

            // Dibuja el quad con las alturas calculadas
            glBegin(GL_QUADS);
                glNormal3f(0,1,0);  // Normal hacia arriba
                glVertex3f(i*SCALE,h1,j*SCALE);
                glVertex3f((i+1)*SCALE,h2,j*SCALE);
                glVertex3f((i+1)*SCALE,h3,(j+1)*SCALE);
                glVertex3f(i*SCALE,h4,(j+1)*SCALE);
            glEnd();
        }
    }
}

// Renderiza todas las esferas activas
void renderSpheres(){
    for(int i=0;i<numSpheres;i++){
        if(!spheres[i].active) continue; // Solo dibuja esferas activas
        
        glPushMatrix();
        glTranslatef(spheres[i].x,spheres[i].y,spheres[i].z);

        // Configura el material de la esfera con su color
        GLfloat matDiffuse[] = { spheres[i].r, spheres[i].g, spheres[i].b, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matDiffuse);

        // Crea y dibuja la esfera usando GLU
        GLUquadric* q = gluNewQuadric();
        gluQuadricNormals(q, GLU_SMOOTH);  // Normales suaves para mejor iluminación
        gluSphere(q, spheres[i].radius, 32, 32);  // 32x32 subdivisiones para suavidad
        gluDeleteQuadric(q);

        glPopMatrix();
    }
}

// Inicializa la configuración de OpenGL
void initOpenGL(){
    glEnable(GL_DEPTH_TEST);      // Activar test de profundidad
    glEnable(GL_LIGHTING);        // Activar iluminación
    glEnable(GL_LIGHT0);          // Activar luz 0
    glEnable(GL_COLOR_MATERIAL);  // Permitir que los colores afecten el material

    // Configuración de la luz principal
    GLfloat lightPos[] = { 20.0f, 100.0f, 30.0f, 1.0f };  // Posición de la luz
    GLfloat ambient[]  = { 0.2f, 0.2f, 0.2f, 1.0f };      // Luz ambiental
    GLfloat diffuse[]  = { 0.8f, 0.8f, 0.8f, 1.0f };      // Luz difusa
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };      // Luz especular

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // Configuración del material para reflejos especulares
    GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess[] = { 50.0f };  // Brillantez del material
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

// Maneja el redimensionamiento de la ventana
void reshape(int w, int h){
    if(h == 0) h = 1;  // Evitar división por cero
    windowWidth = w;
    windowHeight = h;
    
    glViewport(0,0,w,h);  // Ajustar viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w/(float)h, 0.1, 200.0);  // Proyección perspectiva
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char* argv[]){
    FILE* logFile = fopen("fps_log.txt", "w");
    Uint32 startTime = SDL_GetTicks();
    int maxDuration = 10000;   
    // Procesamiento de argumentos de línea de comandos
    if(argc > 1) numSpheres = atof(argv[1]);      // Número de esferas
    if(argc > 2) waveAmplitude = atof(argv[2]);   // Amplitud de olas
    if(argc > 3) waveFrequency = atof(argv[3]);   // Frecuencia de olas
    if(numSpheres > DEF_SPHERES) numSpheres = DEF_SPHERES;  // Limitar máximo

    // Inicialización de SDL y creación de ventana
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Olas con Esferas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    reshape(windowWidth, windowHeight);
    initOpenGL();

    // Configuración inicial de la cámara
    float centerX = (GRID_SIZE * SCALE) / 2.0f;  // Centro del terreno en X
    float centerZ = (GRID_SIZE * SCALE) / 2.0f;  // Centro del terreno en Z
    float camY = 15.0f;           // Altura inicial de la cámara
    float radius = 40.0f;         // Radio de órbita para vista orbital
    float yaw = 0.0f;             // Ángulo de rotación orbital
    float camSpeed = 0.01f;       // Velocidad de rotación de cámara
    float camX, camZ;             // Posición actual de la cámara
    float lookX, lookY, lookZ;    // Vector de dirección de vista
    int viewMode = 0;             // Modo de vista: 0=órbita, 1=arriba, 2=lateral

    // Inicialización de esferas con valores aleatorios
    srand((unsigned int)time(NULL));
    for(int i=0;i<numSpheres;i++){
        spheres[i].x = rand()%GRID_SIZE*SCALE;  // Posición X aleatoria
        spheres[i].z = rand()%GRID_SIZE*SCALE;  // Posición Z aleatoria
        spheres[i].y = 20.0f + ((float)rand() / RAND_MAX) * 60.0f;  // Altura inicial aleatoria
        // Velocidades iniciales aleatorias pequeñas
        spheres[i].vx = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vz = ((rand()%100)/100.0f -0.5f)*0.2f;
        spheres[i].vy = 0.0f;     // Sin velocidad vertical inicial
        spheres[i].radius = 0.5f; // Radio fijo
        // Colores aleatorios
        spheres[i].r = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].g = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].b = 0.3f + ((rand()%100)/100.0f)*0.7f;
        spheres[i].active = 0;    // Inicialmente inactivas
    }

    // Variables del bucle principal
    int running = 1;
    SDL_Event event;
    float t = 0.0f;  // Tiempo de animación

    // Variables para control de tiempo y spawn
    Uint32 lastTime = SDL_GetTicks();
    Uint32 lastSpawn = lastTime;
    int spawned = 0;  // Contador de esferas ya spawneadas

    char title[128];  // Buffer para el título de la ventana

    // Bucle principal del programa
    while(running){
        // Procesamiento de eventos
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) running = 0;
            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED){
                reshape(event.window.data1, event.window.data2);
            }
            // Cambio de modo de vista con teclas
            if(event.type == SDL_KEYDOWN){
                switch(event.key.keysym.sym){
                    case SDLK_1: viewMode = 0; break; // Vista orbital
                    case SDLK_2: viewMode = 1; break; // Vista desde arriba
                    case SDLK_3: viewMode = 2; break; // Vista lateral
                }
            }
        }

        // Cálculo de tiempo delta para animación suave
        Uint32 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f;
        lastTime = now;

        // Sistema de spawn progresivo de esferas
        if(now - lastSpawn >= SPAWN_INTERVAL && spawned < numSpheres){
            spheres[spawned].active = 1;  // Activar siguiente esfera
            spawned++;
            lastSpawn = now;
        }

        // Actualización de posición de cámara según el modo seleccionado
        if(viewMode == 0){ // Vista orbital - rota alrededor del centro
            yaw += camSpeed;
            camX = centerX + radius * sinf(yaw);
            camZ = centerZ + radius * cosf(yaw);
            camY = 15.0f;
            lookX = centerX - camX;  // Mira hacia el centro
            lookY = -camY;
            lookZ = centerZ - camZ;
        } else if(viewMode == 1){ // Vista cenital - desde arriba
            camX = centerX; 
            camZ = centerZ;
            camY = 90.0f;
            lookX = 0.0f;
            lookY = -55.0f;  // Mira hacia abajo
            lookZ = -0.8f;
        } else if(viewMode == 2){ // Vista lateral - desde un lado
            camX = -20.0f; // Fuera del grid
            camZ = centerZ;
            camY = 20.0f;
            lookX = centerX + 20.0f;  // Mira hacia el terreno
            lookY = -20.0f;
            lookZ = centerZ - camZ;
        }

        // Simulación física de las esferas
        for(int i=0;i<numSpheres;i++){
            if(!spheres[i].active) continue;
            
            // Actualización de posición basada en velocidad
            spheres[i].x += spheres[i].vx;
            spheres[i].z += spheres[i].vz;
            spheres[i].vy += GRAVITY;  // Aplicar gravedad
            spheres[i].y += spheres[i].vy;

            // Colisión con el terreno ondulado
            float floorY = waveHeight(spheres[i].x, spheres[i].z, t) + spheres[i].radius;
            if(spheres[i].y < floorY){
                spheres[i].y = floorY;
                spheres[i].vy = -spheres[i].vy * BOUNCE;  // Rebote con pérdida de energía
            }

            // Rebote en los bordes del terreno
            if(spheres[i].x < 0 || spheres[i].x > GRID_SIZE*SCALE) spheres[i].vx *= -1;
            if(spheres[i].z < 0 || spheres[i].z > GRID_SIZE*SCALE) spheres[i].vz *= -1;
        }

        // Renderizado de la escena
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        
        // Configurar cámara
        gluLookAt(camX, camY, camZ,                    // Posición de la cámara
            camX + lookX, camY + lookY, camZ + lookZ,  // Punto al que mira
            0,1,0);                                    // Vector "arriba"

        renderTerrain(t);  // Dibujar terreno ondulado
        renderSpheres();   // Dibujar esferas

        SDL_GL_SwapWindow(window);

        // Cálculo y mostrar FPS en el título
        float fps = 1.0f / deltaTime;
        fprintf(logFile, "%.2f\n", fps);
        fflush(logFile);

        sprintf(title, "Olas con Esferas - FPS: %.2f", fps);
        SDL_SetWindowTitle(window, title);

        SDL_Delay(16);  // Limitar a ~60 FPS
        t += 0.05f;     // Avanzar tiempo de animación

        if (now - startTime >= maxDuration) {
            running = 0;  // salir del bucle tras 10 segundos
        }
    }

    fclose(logFile);
    // Limpieza y cierre
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}