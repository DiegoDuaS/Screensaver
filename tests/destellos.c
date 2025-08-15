#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

typedef struct {
    int x, y;           // posición
    int size;           // tamaño del destello
    SDL_Color color;    // color del destello
} Star;

void draw_star(SDL_Renderer *renderer, Star s) {
    SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b, 255);
    for (int dx = -s.size; dx <= s.size; dx++) {
        for (int dy = -s.size; dy <= s.size; dy++) {
            if (dx*dx + dy*dy <= s.size * s.size) { // círculo simple
                SDL_RenderDrawPoint(renderer, s.x + dx, s.y + dy);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <num_destellos>\n", argv[0]);
        return 1;
    }
    int num_stars = atoi(argv[1]);
    if (num_stars <= 0) num_stars = 10;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Destellos ✨", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    srand(time(NULL));

    Uint64 last_time = SDL_GetPerformanceCounter();
    double fps = 0.0;

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }

        // Calcular FPS
        Uint64 current_time = SDL_GetPerformanceCounter();
        double delta_time = (double)(current_time - last_time) / SDL_GetPerformanceFrequency();
        fps = 1.0 / delta_time;
        last_time = current_time;

        // Limpiar pantalla
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Dibujar destellos
        for (int i = 0; i < num_stars; i++) {
            Star s;
            s.x = rand() % 800;
            s.y = rand() % 600;
            s.size = rand() % 5 + 2; // tamaño 2-6 px
            s.color.r = rand() % 256;
            s.color.g = rand() % 256;
            s.color.b = rand() % 256;
            draw_star(renderer, s);
        }

        // Presentar en pantalla
        SDL_RenderPresent(renderer);

        // Mostrar FPS en consola
        printf("\rFPS: %.2f   ", fps);
        fflush(stdout);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
