#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define NUM_HEX 900
#define GRID_SIZE 20
#define HEX_SIZE 1.0f
#define G 0.02f
#define COEF_REBOTE 0.6f
#define RADIUS 1.2f   // radio aproximado de cada hexágono para colisiones

typedef struct {
    float x, y, z;
    float vy;
    float r, g, b;
    int active;  
    int settled; // nuevo campo
} Hexagon;

Hexagon hexs[NUM_HEX];
float heightMap[GRID_SIZE][GRID_SIZE];
int nextHex = 0;

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h<4 ? x : y;
    float v = h<4 ? y : x;
    return ((h&1)? -u : u) + ((h&2)? -2.0f*v : 2.0f*v);
}

int p[512];

void initPerlin() {
    for(int i=0;i<256;i++) p[i]=i;
    for(int i=0;i<256;i++){
        int j = rand()%256;
        int tmp = p[i]; p[i]=p[j]; p[j]=tmp;
        p[i+256] = p[i];
    }
}

float perlin2D(float x, float y){
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = p[p[X]+Y];
    int ab = p[p[X]+Y+1];
    int ba = p[p[X+1]+Y];
    int bb = p[p[X+1]+Y+1];

    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf-1, yf), u);
    float x2 = lerp(grad(ab, xf, yf-1), grad(bb, xf-1, yf-1), u);

    return lerp(x1, x2, v);
}

void generateHeightMap() {
    srand(time(NULL));
    initPerlin();

    int octaves = 4;
    float persistence = 0.5f;
    float scale = 0.1f;

    for(int i=0;i<GRID_SIZE;i++){
        for(int j=0;j<GRID_SIZE;j++){
            float x = i * scale;
            float y = j * scale;

            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noise = 0.0f;

            for(int o=0;o<octaves;o++){
                noise += amplitude * perlin2D(x * frequency, y * frequency);
                amplitude *= persistence;
                frequency *= 2.0f;
            }

            noise = (noise + 1.0f)/2.0f;
            heightMap[i][j] = noise * 10.0f;
        }
    }
}

void initHexagons() {
    srand(time(NULL));
    int placed = 0;
    for (int i = 0; i < GRID_SIZE && placed < NUM_HEX; i++) {
        for (int j = 0; j < GRID_SIZE && placed < NUM_HEX; j++) {
            float x = (i - GRID_SIZE/2) * (HEX_SIZE * 1.5f);
            float z = (j - GRID_SIZE/2) * (HEX_SIZE * sqrt(3)) 
                      + ((i % 2) ? HEX_SIZE * sqrt(3)/2 : 0);

            hexs[placed].x = x;
            hexs[placed].z = z;
            hexs[placed].y = 20.0f + (rand()%10);
            hexs[placed].vy = 0.0f;
            hexs[placed].active = 0;
            hexs[placed].settled = 0;

            float normH = heightMap[i][j] / 10.0f;
            if(normH < 0.2f){ hexs[placed].r=0; hexs[placed].g=0.2f; hexs[placed].b=0.5f; }
            else if(normH < 0.3f){ hexs[placed].r=0; hexs[placed].g=0.3f; hexs[placed].b=0.6f; }
            else if(normH < 0.4f){ hexs[placed].r=0.8f; hexs[placed].g=0.7f; hexs[placed].b=0.5f; }
            else if(normH < 0.6f){ hexs[placed].r=0.1f; hexs[placed].g=0.6f; hexs[placed].b=0.1f; }
            else if(normH < 0.8f){ hexs[placed].r=0.5f; hexs[placed].g=0.35f; hexs[placed].b=0.2f; }
            else { hexs[placed].r=0.9f; hexs[placed].g=0.9f; hexs[placed].b=0.9f; }

            placed++;
        }
    }
}

void drawHexPrism(float x,float y,float z,float size,float height,
                  float r,float g,float b) {
    int i;
    float angle;
    float top[6][3], bottom[6][3];

    for(i=0;i<6;i++){
        angle = M_PI/3.0f*i;
        float vx = x + cos(angle)*size;
        float vz = z + sin(angle)*size;
        top[i][0] = vx; top[i][1] = y+height/2; top[i][2] = vz;
        bottom[i][0] = vx; bottom[i][1] = y-height/2; bottom[i][2] = vz;
    }

    glColor3f(r,g,b);

    glBegin(GL_POLYGON);
    for(i=0;i<6;i++) glVertex3fv(top[i]);
    glEnd();

    glBegin(GL_POLYGON);
    for(i=0;i<6;i++) glVertex3fv(bottom[i]);
    glEnd();

    glBegin(GL_QUADS);
    for(i=0;i<6;i++){
        int j=(i+1)%6;
        glVertex3fv(top[i]);
        glVertex3fv(top[j]);
        glVertex3fv(bottom[j]);
        glVertex3fv(bottom[i]);
    }
    glEnd();
}

void updateHexagons(){
    for(int i=0;i<NUM_HEX;i++){
        if(!hexs[i].active) continue;

        hexs[i].vy -= G;
        hexs[i].y += hexs[i].vy;

        // Rebote con el terreno
        int gx = (int)((hexs[i].x/HEX_SIZE)+GRID_SIZE/2);
        int gz = (int)((hexs[i].z/HEX_SIZE)+GRID_SIZE/2);
        if(gx<0) gx=0; if(gx>=GRID_SIZE) gx=GRID_SIZE-1;
        if(gz<0) gz=0; if(gz>=GRID_SIZE) gz=GRID_SIZE-1;
        float targetY = heightMap[gx][gz];

        if(hexs[i].y <= targetY){
            hexs[i].y = targetY;
            hexs[i].vy = 0;   // detener con el terreno, no lo marcamos como settled
        }

        // Rebote con otros hexágonos
        for(int j=0;j<NUM_HEX;j++){
            if(i==j || !hexs[j].active) continue;

            float dx = hexs[j].x - hexs[i].x;
            float dz = hexs[j].z - hexs[i].z;
            float distSq = dx*dx + dz*dz;
            if(distSq < RADIUS*RADIUS){
                float dy = hexs[j].y - hexs[i].y;
                if(dy > 0 && dy < 2.0f){  // solo rebote si el otro está arriba
                    float dist = sqrt(distSq);
                    if(dist < 0.001f) dist = 0.001f;
                    float overlap = RADIUS - dist;
                    dx/=dist; dz/=dist;
                    hexs[i].x -= dx*overlap/2.0f; hexs[i].z -= dz*overlap/2.0f;
                    hexs[j].x += dx*overlap/2.0f; hexs[j].z += dz*overlap/2.0f;

                    float tempVy = hexs[i].vy;
                    hexs[i].vy = -hexs[j].vy * COEF_REBOTE;
                    hexs[j].vy = -tempVy * COEF_REBOTE;

                    // Si después del rebote está muy cerca del otro, lo marcamos settled
                    if(fabs(hexs[i].vy) < 0.05f){
                        hexs[i].vy = 0;
                        hexs[i].settled = 1;
                    }
                }
            }
        }
    }
}


int main(int argc,char *argv[]){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Hexagonos sobre relieve",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1024,768,SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,1024.0/768.0,0.1,100.0);
    glMatrixMode(GL_MODELVIEW);

    generateHeightMap();
    initHexagons();

    int running=1;
    float camX=0.0f, camY=15.0f, camZ=30.0f;
    float yaw=0.0f, pitch=0.0f;
    float speed=0.5f;
    float lookX, lookY, lookZ;

    void updateCameraDirection(){
        lookX = cosf(pitch)*sinf(yaw);
        lookY = sinf(pitch);
        lookZ = -cosf(pitch)*cosf(yaw);
    }

    SDL_Event event;
    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) running=0;
            if(event.type==SDL_KEYDOWN){
                switch(event.key.keysym.sym){
                    case SDLK_w: camX+=lookX*speed; camY+=lookY*speed; camZ+=lookZ*speed; break;
                    case SDLK_s: camX-=lookX*speed; camY-=lookY*speed; camZ-=lookZ*speed; break;
                    case SDLK_a: camX+=cosf(yaw)*speed; camZ+=sinf(yaw)*speed; break;
                    case SDLK_d: camX-=cosf(yaw)*speed; camZ-=sinf(yaw)*speed; break;
                    case SDLK_q: pitch-=0.1f; break;
                    case SDLK_e: pitch+=0.1f; break;
                    case SDLK_LEFT: yaw-=0.1f; break;
                    case SDLK_RIGHT: yaw+=0.1f; break;
                }
            }
        }

        updateCameraDirection();

        // --- Spawn gradual: solo si el anterior ya settled ---
        if(nextHex < NUM_HEX){
            if(nextHex == 0 || hexs[nextHex-1].settled){
                hexs[nextHex].active = 1;
                nextHex++;
            }
        }

        updateHexagons();

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        gluLookAt(camX,camY,camZ, camX+lookX, camY+lookY, camZ+lookZ, 0,1,0);

        for(int i=0;i<NUM_HEX;i++){
            if(!hexs[i].active) continue;
            drawHexPrism(hexs[i].x, hexs[i].y, hexs[i].z, HEX_SIZE, 2.0f,
                         hexs[i].r, hexs[i].g, hexs[i].b);
        }

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
