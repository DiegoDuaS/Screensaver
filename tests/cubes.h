#ifndef CUBES_H
#define CUBES_H

// Librerías necesarias OpenGL (gráficos 3D) y funciones matemáticas
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

// Constantes
#define NUM_CUBES 100       // Número total de cubos en la escena
#define G 0.01f             // Constante de gravedad (aceleración hacia abajo)
#define Y_THRESHOLD -5.0f   // Umbral Y bajo el cual los cubos se reinician
#define MAX_HEIGHT 5.0f     // Altura máxima para el reinicio de cubos

// Estructura para representar un vector 3D (posición en el espacio)
typedef struct {
    float x, y, z;  // Coordenadas X, Y, Z
} Vec3;

// Estructura que define las propiedades de cada cubo
typedef struct {
    Vec3 pos;         // Posición actual del cubo en el espacio 3D
    float angle;      // Ángulo de rotación actual del cubo (en radianes)
    float speed;      // Velocidad de movimiento horizontal del cubo
    float moveAngle;  // Dirección de movimiento del cubo (en radianes)
    int state;        // Estado del cubo: 0 = rotando, 1 = cayendo
    float stepHeight; // Altura del "escalón" que baja cada vez
    float rotAmount;  // Cantidad de rotación acumulada en el ciclo actual
    float rotTarget;  // Objetivo de rotación (cuánto debe rotar antes de caer)
    float vy;         // Velocidad vertical (para la caída con gravedad)
    float targetY;    // Altura objetivo a la que debe llegar al caer
    float r, g, b;    // Componentes RGB del color del cubo (0.0 a 1.0)
    int rotDirection; // Dirección de rotación: 1 (horario) o -1 (antihorario)
} Cube;

// Funciones públicas
void initCubes(Cube cubes[NUM_CUBES]);
void updateCube(Cube *c);
void drawCubeAt(Vec3 pos, float angle, float r, float g, float b);

#endif