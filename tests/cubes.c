#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

// Vertices del cubo (8 puntos en 3D)
typedef struct {
    float x, y, z;
} Vec3;

// Proyección simple 3D a 2D (perspectiva)
void project(Vec3 v, int *x, int *y, float angle) {
    // Rotación simple en Y
    float cosA = cos(angle);
    float sinA = sin(angle);

    float x_rot = v.x * cosA - v.z * sinA;
    float z_rot = v.x * sinA + v.z * cosA;
    float y_rot = v.y;

    float distance = 3.0f;  // distancia cámara
    float z_proj = 1.0f / (distance - z_rot);

    *x = (int)(WIDTH / 2 + x_rot * z_proj * WIDTH);
    *y = (int)(HEIGHT / 2 - y_rot * z_proj * HEIGHT);
}

// Dibuja línea entre dos puntos
void draw_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Cubo 3D Wireframe",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, 0);
    if (!window) {
        printf("Error SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Error SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Definimos los 8 vértices del cubo (tamaño 1)
    Vec3 vertices[8] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f}
    };

    // Aristas del cubo (pares de índices de vertices)
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // cara trasera
        {4,5},{5,6},{6,7},{7,4}, // cara frontal
        {0,4},{1,5},{2,6},{3,7}  // conexiones entre caras
    };

    int running = 1;
    SDL_Event event;
    float angle = 0.0f;

    while (running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // negro
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // blanco para líneas

        // Proyectar y dibujar aristas
        for (int i = 0; i < 12; i++) {
            int x1, y1, x2, y2;
            project(vertices[edges[i][0]], &x1, &y1, angle);
            project(vertices[edges[i][1]], &x2, &y2, angle);
            draw_line(renderer, x1, y1, x2, y2);
        }

        SDL_RenderPresent(renderer);

        angle += 0.01f;  // rotar lentamente

        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
