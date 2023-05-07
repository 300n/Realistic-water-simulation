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
#define Mparticle 1000/(matlength*matwidth)
#define FPS 100 
const double pi = 2*acos(0);
const double g =9.80665;
SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font* font;
int O = 0;
char texte[100];
clock_t starttime;
Uint32 last_time;
Uint32 current_time;
SDL_Rect square;
SDL_Rect rect_texte;
SDL_Surface* surface_texte;
SDL_Texture* texture_texte;
SDL_Color couleur = { 255, 255, 255, SDL_ALPHA_OPAQUE};




typedef struct point {
    double x;
    double y;
    int xdirection;
    int ydirection;
}Point;

typedef struct matrice {
    int MATlength;
    int MATwidth;
    Point** data;
}Matrice;

Matrice mat;

void initSDL()
/*initialise SDL*/
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
    SDL_Surface* image = NULL;
    SDL_Texture* texture = NULL; 
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
}

int initTTF()
/*initialise la police d'ecriture*/
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

void initmat()
/*initialise la matrice*/
{
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
            mat.data[i][j].xdirection = 1;
            mat.data[i][j].ydirection = 1;
        }
    }
}


void drawgrid()
{
    int nbofd = 20;
    SDL_SetRenderDrawColor(renderer,50,50,50,SDL_ALPHA_TRANSPARENT);
    for (int i = 1; i<nbofd ;i++) {
        SDL_RenderDrawLine(renderer,(height/40)+(((height-(height/20))/nbofd)*i),(width/40),((height/40)+(((height-(height/20))/nbofd)*i)),width-(width/40));
        SDL_RenderDrawLine(renderer,(height/40),(width/40)+(((width-(width/20))/nbofd)*i),height-(height/40),(width/40)+(((width-(width/20))/nbofd)*i));
    }
    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,(height/40),(width/40),(height/40),width-(width/40));
    SDL_RenderDrawLine(renderer,(height/40),(width/40),height-(height/40),(width/40));
    SDL_RenderDrawLine(renderer,(height/40),width-(width/40),height-(height/40),width-(width/40));
    SDL_RenderDrawLine(renderer,height-(height/40),(width/40),height-(height/40),width-(width/40));

}




double gettime()
{
    return (double)(clock() - starttime) / CLOCKS_PER_SEC;
}

void stataff(int fps)
/*affichage des statistiques*/ 
{
    sprintf(texte, "fps : %d", fps);
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = (height/40)+5;
    rect_texte.y = (width/40);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "nombre d'operations par frame : %d", O);
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y = (width/40)+15;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "temps ecoule : %-2f s", gettime());
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y = (width/40)+30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    O = 0;
} 

/*
void collision(double currentxpos, double currentypos, double isgoingtomoovetox, double isgoingtomoovetoy)
{
    for (int k = 0; k<isgoingtomoovetox; k++) {
        if (mat.data[][currentypos].x)
    }
    for (int l = 0; l<isgoingtomoovetoy; l++) {
        if (mat.data[i][j].y)
    }
}*/

double calcule_viscosite_eau(int T)
/*calcule de la viscosité de l'eau en fonction de la temperature*/
{
    double viscositer;
    viscositer = pow(1.79*10,-3)*(1/1+0.03368*T+T*0.000211);
    return viscositer ;
}

void particleoutofthegrid()
/*Détecte une sortie de l'écran*/
{
    for (int i = 0; i<matlength; i++) {
        for (int j = 0 ; j<matwidth; j++) {
            if (mat.data[i][j].x<(width/40) || mat.data[i][j].x>width-(width/40)) {
                mat.data[i][j].xdirection *= -1;
                O++;
            }
            if (mat.data[i][j].y<(height/40) || mat.data[i][j].y>height-(height/40)) {
                mat.data[i][j].ydirection *= -1;
                O++;
            }
        }
    }
} 

void update()
/*mise à jour des positions de chaque particule*/
{
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = 0;
    square.y = 0;
    square.h = height;
    square.w = width;
    SDL_RenderFillRect(renderer, &square);
    drawgrid();
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
    particleoutofthegrid();
    for (int i = 0 ; i < matlength ; i++) {
        for (int j = 0 ; j < matwidth ; j++) {      
            mat.data[i][j].y += ((g*Mparticle)*mat.data[i][j].ydirection);
            /*if (mat.data[i][j].xdirection == -1) {
                mat.data[i][j].xdirection *= -1;
            }
            if (mat.data[i][j].ydirection == -1) {
                mat.data[i][j].ydirection *= -1;
            }*/
            O++;
            SDL_RenderDrawPoint(renderer,mat.data[i][j].x,mat.data[i][j].y);
        }
    }
}

void aff()
/*affichage des particules*/
{
    int a, x, y, running = 1;
    SDL_Event Event;
    initSDL();
    initTTF();
    Uint32 start_time, end_time, elapsed_time;
    initmat();
    starttime = clock();
    while (running) {
        start_time = SDL_GetTicks();
        last_time = current_time;
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_QUIT||Event.type == SDL_SCANCODE_ESCAPE) {
                    running = 0;
            }
        }
        
        update();
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
    return 0;
}
