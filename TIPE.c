#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <float.h>
#include <stdint.h>


#define width 800
#define height 800
#define matwidth 50
#define matlength 50
#define Mparticle = 1/(tabwidth*tabheight)
#define FPS 120 
const double pi = 2*acos(0);
const double g = 9.81;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Surface* surface_texte;
SDL_Texture* texture_texte;
TTF_Font* font;
int O = 0;
Uint32 last_time;
Uint32 current_time;



typedef struct point {
    double x;
    double y;
}Point;

typedef struct matrice {
    int MATlength;
    int MATwidth;
    Point** data;
}Matrice;




void initSDL()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
    SDL_Surface* image = NULL;
    SDL_Texture* texture = NULL; 
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
}

int initTTF()
{
    if (TTF_Init() != 0) {
        printf("Erreur : %s\n", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    font = TTF_OpenFont("/home/bastien/2022-2023/Info/TIPE/font.ttf", 18);
    if (!font) {
        printf("Erreur : impossible d'ouvrir le fichier de police\n");
        TTF_Quit();
        SDL_Quit();
    }
}

Matrice initmat()
{
    int x,y;
    Matrice mat;
    mat.MATlength = matlength;
    mat.MATwidth = matwidth;
    mat.data = (Point**)malloc(sizeof(Point*)*matlength);
    for (int i = 0; i < matlength; i++) {
        mat.data[i] = (Point*)malloc(sizeof(Point)*matwidth);
    }

    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            mat.data[i][j].x = (width-matlength*2)/2+i*2;
            mat.data[i][j].y = (height-matwidth*2)/4+j*2; 
        }
    }
    for (int i = 0 ; i < matlength ; i++) {
        for (int j = 0 ; j < matwidth ; j++) {      
            x = mat.data[i][j].x;
            y = mat.data[i][j].y;
            SDL_RenderDrawPoint(renderer,x,y);
        }
    }
}



void stataff(int fps) 
{
    char texte[100];
    sprintf(texte, "fps : %d", fps);
    SDL_Color couleur = { 255, 255, 255, SDL_ALPHA_OPAQUE};
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    SDL_Rect rect_texte = {0, 0, surface_texte->w, surface_texte->h};
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "nombre d'operations par frame: %d", O);
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    SDL_Rect rect_texte2 = {0, 30, surface_texte->w, surface_texte->h};
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte2);
    O = 0;

} 

void update(Matrice mat)
{
    float x, y;
    SDL_Rect square;
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = 0;
    square.y = 0;
    square.h = height;
    square.w = width;
    SDL_RenderFillRect(renderer, &square);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
    for (int i = 0 ; i < matlength ; i++) {
        for (int j = 0 ; j < matwidth ; j++) {      
            x = mat.data[i][j].x;
            y = mat.data[i][j].y;
            SDL_RenderDrawPoint(renderer,x,y);
        }
    }
}

void aff()
{
    int a, x, y, running = 1;
    SDL_Event Event;
    initSDL();
    initTTF();
    Uint32 start_time, end_time, elapsed_time;
    
    Matrice mat = initmat();
    while (running) {
        start_time = SDL_GetTicks();
        last_time = current_time;
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_QUIT||Event.type == SDL_SCANCODE_ESCAPE) {
                    running = 0;
            }
        }
        
        ///update(mat);
        end_time = SDL_GetTicks();
        elapsed_time = end_time - start_time;
        if (elapsed_time < 1000 / FPS) {
            SDL_Delay((1000 / FPS) - elapsed_time);
        }
        stataff(FPS);
        SDL_RenderPresent(renderer);
    }


    SDL_DestroyTexture(texture_texte);
    SDL_FreeSurface(surface_texte);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}



int main()
{
    aff();
}