// Librerías necesarias
#include <SDL2/SDL.h>          // Biblioteca SDL2 para ventanas, eventos y contexto OpenGL
#include <GL/gl.h>             // Funciones básicas de OpenGL para renderizado 3D
#include <GL/glu.h>            // Utilidades OpenGL (gluPerspective, gluLookAt)
#include "tests/cubes.h"       // Header personalizado con estructuras y funciones de cubos
#include <stdio.h>             // Para printf (mostrar FPS en consola)
#include <stdlib.h>            // Para atoi (conversión de argumentos de línea de comandos)

// Variables globales de la cámara (configuración inicial de la vista)
float camYaw = 0.2f;           // Rotación horizontal de la cámara en radianes (ligeramente girada)
float camPitch = 0.0f;         // Rotación vertical de la cámara (mirando horizontalmente)
float camDist = 15.0f;         // Distancia de la cámara al centro de la escena

/**
 * Función principal del programa
 * Maneja argumentos de línea de comandos, inicialización y bucle principal
 */
int main(int argc, char* argv[]) {
    // Número de cubos por defecto (definido en cubes.h)
    int numCubes = NUM_CUBES;
    
    // Si se proporciona un argumento, usarlo como número de cubos
    if(argc > 1) {
        int n = atoi(argv[1]);                    // Convertir argumento string a entero
        if(n > 0) numCubes = n;                   // Solo usar si es un número positivo válido
    }
    
    // Reservar memoria para el array de cubos según el número especificado
    Cube* cubes = malloc(sizeof(Cube) * numCubes);
    if(!cubes) {
        fprintf(stderr, "Error al asignar memoria para cubos\n");
        return 1;                                 // Salir con código de error si falla malloc
    }
    initCubes(cubes);                            // Inicializar todos los cubos con valores aleatorios
    
    SDL_Init(SDL_INIT_VIDEO);                    // Inicializar subsistema de video de SDL

    SDL_DisplayMode dm;                          // Estructura para información de pantalla
    if(SDL_GetCurrentDisplayMode(0, &dm) != 0) { // Intentar obtener resolución actual
        // Si falla, usar resolución por defecto
        dm.w = 1280;
        dm.h = 720;
    }

    // Calcular tamaño de ventana como 80% del tamaño de pantalla
    int width = dm.w * 0.8;                     // 80% del ancho de pantalla
    int height = dm.h * 0.8;                    // 80% del alto de pantalla
    
    SDL_Window* window = SDL_CreateWindow(
        "Cubo Escalera OpenGL",                  // Título de la ventana
        SDL_WINDOWPOS_CENTERED,                  // Posición X centrada
        SDL_WINDOWPOS_CENTERED,                  // Posición Y centrada
        width, height,                           // Dimensiones calculadas
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE // Flags: OpenGL y redimensionable
    );
    SDL_GLContext glContext = SDL_GL_CreateContext(window); // Crear contexto OpenGL
    
    glEnable(GL_DEPTH_TEST);                     // Habilitar test de profundidad (Z-buffer)
    
    int running = 1;                             // Flag para controlar el bucle principal
    SDL_Event event;                             // Estructura para capturar eventos SDL
    
    Uint32 fpsLastTime = SDL_GetTicks();         // Tiempo del último cálculo de FPS
    Uint32 fpsFrames = 0;                        // Contador de frames renderizados
    
    while(running) {

        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = 0; // Salir si se cierra la ventana
        }

        for(int i = 0; i < numCubes; i++) {
            // Calcular rotación continua del cubo
            float rotStep = 0.03f * cubes[i].rotDirection; // Velocidad de rotación con dirección
            cubes[i].angle += rotStep;                      // Aplicar rotación

            // Actualizar estado de cubo
            updateCube(&cubes[i]);

            // Verificar límite inferior y detener caída si es necesario
            if(cubes[i].pos.y < Y_THRESHOLD) {
                cubes[i].pos.y = Y_THRESHOLD;               // Fijar en el umbral mínimo
                cubes[i].vy = 0;                            // Detener velocidad vertical
                cubes[i].state = 0;                         // Volver a estado de rotación
            }
        }
        
        // Establecer área de renderizado y limpiar buffers
        glViewport(0, 0, width, height);                    // Área de renderizado completa
        glClearColor(0, 0, 0, 1);                           // Color de fondo negro
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpiar buffers

        // Configurar matriz de proyección (perspectiva 3D)
        glMatrixMode(GL_PROJECTION);                        // Cambiar a matriz de proyección
        glLoadIdentity();                                   // Resetear matriz
        gluPerspective(45.0f,                               // Campo de visión (FOV) de 45°
                      (float)width / height,                // Relación de aspecto
                      0.1f, 100.0f);                        // Planos cercano y lejano

        // Configurar matriz de vista (posición de cámara)
        glMatrixMode(GL_MODELVIEW);                         // Cambiar a matriz modelo-vista
        glLoadIdentity();                                   // Resetear matriz
        
        // Posicionar cámara usando coordenadas esféricas
        gluLookAt(camDist * sinf(camYaw),                   // Posición X de la cámara
                  camDist * sinf(camPitch),                 // Posición Y de la cámara
                  camDist * cosf(camYaw),                   // Posición Z de la cámara
                  0, 0, 0,                                  // Punto objetivo (origen)
                  0, 1, 0);                                 // Vector "arriba" (eje Y positivo)

        for(int i = 0; i < numCubes; i++) {
            drawCubeAt(cubes[i].pos,                        // Posición del cubo
                      cubes[i].angle,                       // Rotación actual
                      cubes[i].r, cubes[i].g, cubes[i].b);  // Color RGB del cubo
        }

        // Intercambiar buffers para mostrar el frame renderizado
        SDL_GL_SwapWindow(window);

        fpsFrames++;                                        // Incrementar contador de frames
        Uint32 fpsCurrentTime = SDL_GetTicks();             // Obtener tiempo actual
        
        // Mostrar FPS cada segundo (1000 ms)
        if(fpsCurrentTime - fpsLastTime >= 1000) {
            printf("FPS: %u\n", fpsFrames);                // Mostrar FPS en consola
            fpsFrames = 0;                                  // Resetear contador
            fpsLastTime = fpsCurrentTime;                   // Actualizar tiempo base
        }

        // Pausa para mantener aproximadamente 60 FPS (16ms ≈ 1/60 segundos)
        SDL_Delay(16);
    }

    //  LIMPIEZA Y LIBERACIÓN DE RECURSOS 
    free(cubes);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}