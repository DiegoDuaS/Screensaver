#include "cubes.h"

// Librerías necesarias para SDL2 (ventana/eventos)
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * Actualiza la lógica de un cubo individual en cada frame
 * Maneja la rotación, caída, movimiento horizontal y reinicio
 */
void updateCube(Cube *c) {
    // Estado 0: El cubo está rotando
    if(c->state == 0) { 
        // Calcular el paso de rotación basado en la dirección
        float rotStep = 0.05f * c->rotDirection;
        c->angle += rotStep;                    // Aplicar la rotación
        c->rotAmount += fabs(rotStep);          // Acumular rotación absoluta
        
        // Verificar si ha completado la rotación objetivo (90 grados)
        if(c->rotAmount >= c->rotTarget) {
            c->rotAmount = 0;                   // Resetear contador de rotación
            c->state = 1;                       // Cambiar a estado de caída
            c->vy = 0;                          // Inicializar velocidad vertical
            c->targetY = c->pos.y - c->stepHeight; // Calcular altura objetivo
        }
    } 
    // Estado 1: El cubo está cayendo
    else if(c->state == 1) { 
        c->vy += G;                             // Aplicar gravedad (acelerar hacia abajo)
        c->pos.y -= c->vy;                      // Mover verticalmente según velocidad
        
        // Verificar si ha llegado a la altura objetivo
        if(c->pos.y <= c->targetY) {
            c->pos.y = c->targetY;              // Fijar posición exacta
            c->vy = 0;                          // Detener movimiento vertical
            c->state = 0;                       // Volver a estado de rotación
        }
    }
    // Estado 2: cubo estático
    else if(c->state == 2) {
        // No hacer nada: cubo completamente estático
    }


    // Movimiento horizontal continuo (independiente del estado)
    c->pos.x += cosf(c->moveAngle) * c->speed;  // Componente X del movimiento
    c->pos.z += sinf(c->moveAngle) * c->speed;  // Componente Z del movimiento

    // Reiniciar cubo si cae demasiado bajo (efecto de bucle infinito)
    if(c->pos.y < Y_THRESHOLD) {
        // Nueva posición aleatoria en la parte superior
        c->pos.y = MAX_HEIGHT + ((float)rand()/RAND_MAX)*5.0f; // Altura inicial aleatoria
        c->pos.x = ((float)rand()/RAND_MAX)*20.0f - 10.0f;     // Posición X aleatoria (-10 a 10)
        c->pos.z = ((float)rand()/RAND_MAX)*20.0f - 10.0f;     // Posición Z aleatoria (-10 a 10)
        c->angle = ((float)rand()/RAND_MAX)*2*M_PI;             // Ángulo inicial aleatorio
        c->rotAmount = 0;                                       // Resetear rotación acumulada
        c->rotDirection = (rand()%2)?1:-1;                      // Nueva dirección de rotación aleatoria
        c->state = 0;
    }
}

/**
 * Dibuja un cubo 3D en una posición específica con rotación y color dados
 * Utiliza OpenGL para renderizar las 6 caras del cubo
 */
void drawCubeAt(Vec3 pos, float angle, float r, float g, float b) {
    glPushMatrix();                                    // Guardar matriz de transformación actual
    glTranslatef(pos.x, pos.y, pos.z);                // Mover a la posición del cubo
    glRotatef(angle * 180.0f / M_PI, 0.0f, 0.0f, 1.0f); // Rotar en eje Z (convertir radianes a grados)

    glColor3f(r, g, b);                                // Establecer color del cubo

    // Comenzar a dibujar cuádruples (caras del cubo)
    glBegin(GL_QUADS);

    // Cara frontal (hacia el espectador)
    glVertex3f(-0.5f,-0.5f,0.5f);  // Vértice inferior izquierdo
    glVertex3f(0.5f,-0.5f,0.5f);   // Vértice inferior derecho
    glVertex3f(0.5f,0.5f,0.5f);    // Vértice superior derecho
    glVertex3f(-0.5f,0.5f,0.5f);   // Vértice superior izquierdo

    // Cara trasera (alejada del espectador)
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

    glEnd();                                           // Terminar de dibujar
    glPopMatrix();                                     // Restaurar matriz de transformación
}

/**
 * Inicializa todos los cubos con propiedades aleatorias
 * Establece posiciones, velocidades, colores y otros parámetros iniciales
 */
void initCubes(Cube cubes[NUM_CUBES]) {
    srand(time(NULL));                                 // Inicializar generador de números aleatorios
    
    for(int i=0; i<NUM_CUBES; i++) {
        // Posición inicial aleatoria
        cubes[i].pos.x = ((float)rand()/RAND_MAX)*20.0f - 10.0f; // X entre -10 y 10
        cubes[i].pos.y = ((float)rand()/RAND_MAX)*5.0f;           // Y entre 0 y 5 (altura inicial)
        cubes[i].pos.z = ((float)rand()/RAND_MAX)*60.0f - 50.0f; // Z entre -50 y 10 (más profundidad)
        
        // Propiedades de rotación y movimiento
        cubes[i].angle = 0.0f;                                    // Todos empiezan sin rotación
        cubes[i].speed = 0.01f + ((float)rand()/RAND_MAX)*0.05f; // Velocidad entre 0.01 y 0.06
        cubes[i].moveAngle = ((float)rand()/RAND_MAX)*2*M_PI;     // Dirección aleatoria (0 a 2π)
        
        // Propiedades del estado y escalón
        cubes[i].state = 0;                                       // Empezar en estado de rotación
        cubes[i].stepHeight = 0.5f + ((float)rand()/RAND_MAX)*1.5f; // Altura de escalón (0.5 a 2.0)
        cubes[i].rotAmount = 0.0f;                                // Sin rotación acumulada inicial
        cubes[i].rotTarget = M_PI/2;                              // Objetivo: 90 grados (π/2 radianes)
        cubes[i].vy = 0.0f;                                       // Sin velocidad vertical inicial
        cubes[i].targetY = cubes[i].pos.y - cubes[i].stepHeight;  // Calcular primera altura objetivo
        
        // Color aleatorio para cada cubo
        cubes[i].r = (float)rand()/RAND_MAX;                      // Componente rojo (0.0 a 1.0)
        cubes[i].g = (float)rand()/RAND_MAX;                      // Componente verde (0.0 a 1.0)
        cubes[i].b = (float)rand()/RAND_MAX;                      // Componente azul (0.0 a 1.0)
        
        // Dirección de rotación aleatoria
        cubes[i].rotDirection = (rand()%2)?1:-1;                  // 50% probabilidad cada dirección
    }
}

#ifdef TEST_CUBES
#include <SDL2/SDL.h>

// Constantes de configuración del programa
#define WIDTH 1800          // Ancho de la ventana en píxeles
#define HEIGHT 950          // Alto de la ventana en píxeles  

// Variables globales de la cámara
float camYaw = 0.0f;    // Rotación horizontal de la cámara (en radianes)
float camPitch = 0.0f;  // Rotación vertical de la cámara (en radianes)
float camDist = 15.0f;  // Distancia de la cámara al centro de la escena

/**
 * Función principal del programa
 * Inicializa SDL/OpenGL, maneja el bucle principal de renderizado y eventos
 */
int main(int argc, char* argv[]) {

    // Crear y configurar array de cubos
    Cube cubes[NUM_CUBES];
    initCubes(cubes);

    // Inicialización de SDL2
    SDL_Init(SDL_INIT_VIDEO);                                     // Inicializar subsistema de video
    SDL_Window* window = SDL_CreateWindow("Cubo Escalera OpenGL", // Crear ventana
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,           // Posición centrada
        WIDTH, HEIGHT,                                            // Dimensiones
        SDL_WINDOW_OPENGL);                                       // Habilitar contexto OpenGL
    SDL_GLContext glContext = SDL_GL_CreateContext(window);       // Crear contexto OpenGL

    // Configuración básica de OpenGL
    glEnable(GL_DEPTH_TEST);                                      // Habilitar test de profundidad (Z-buffer)

    // Variables para el bucle principal
    int running = 1;                                              // Flag para mantener el programa corriendo
    SDL_Event event;                                              // Estructura para eventos SDL

    // Bucle principal del programa
    while(running) {
        // Procesamiento de eventos (cerrar ventana, teclado, etc.)
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = 0;               // Salir si se cierra la ventana
        }

        // Actualizar lógica de todos los cubos
        for(int i=0; i<NUM_CUBES; i++) {
            updateCube(&cubes[i]);
        }

        // Configuración del viewport y limpieza de buffers
        glViewport(0, 0, WIDTH, HEIGHT);                          // Establecer área de renderizado
        glClearColor(0, 0, 0, 1);                                 // Color de fondo negro
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // Limpiar buffers de color y profundidad

        // Configuración de la matriz de proyección (perspectiva)
        glMatrixMode(GL_PROJECTION);                              // Cambiar a matriz de proyección
        glLoadIdentity();                                         // Resetear matriz
        gluPerspective(45.0f,                                     // Ángulo de campo de visión (FOV)
                      (float)WIDTH/(float)HEIGHT,                 // Relación de aspecto
                      0.1f, 100.0f);                              // Planos cercano y lejano

        // Configuración de la matriz de vista (cámara)
        glMatrixMode(GL_MODELVIEW);                               // Cambiar a matriz de modelo-vista
        glLoadIdentity();                                         // Resetear matriz
        
        // Posicionar cámara usando coordenadas esféricas
        gluLookAt(camDist * sinf(camYaw),                         // Posición X de la cámara
                  camDist * sinf(camPitch),                       // Posición Y de la cámara  
                  camDist * cosf(camYaw),                         // Posición Z de la cámara
                  0, 0, 0,                                        // Punto al que mira (origen)
                  0, 1, 0);                                       // Vector "arriba"

        // Renderizado de todos los cubos
        for(int i=0; i<NUM_CUBES; i++) {
            drawCubeAt(cubes[i].pos,                              // Posición del cubo
                      cubes[i].angle,                             // Rotación del cubo
                      cubes[i].r, cubes[i].g, cubes[i].b);        // Color del cubo
        }

        // Intercambiar buffers para mostrar el frame renderizado
        SDL_GL_SwapWindow(window);
        
        // Pausa para mantener ~60 FPS (16ms ≈ 1/60 segundos)
        SDL_Delay(16);
    }

    // Limpieza y liberación de recursos
    SDL_GL_DeleteContext(glContext);                              // Eliminar contexto OpenGL
    SDL_DestroyWindow(window);                                    // Destruir ventana
    SDL_Quit();                                                   // Finalizar SDL
    return 0;                                                     // Retorno exitoso
}

#endif