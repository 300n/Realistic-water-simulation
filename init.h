#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h> 


#define matwidth 30
#define matlength 30
#define width 1000
#define height 1000
#define pradius 2
SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font* font;
TTF_Font* smallfont;
SDL_Rect square;
SDL_Rect rect_texte;
SDL_Surface* surface_texte;
SDL_Texture* texture_texte;


typedef struct point {
    double x;
    double y;
    double vx;
    double vy;
    double ax;
    double ay;
    int xdirection;
    int ydirection;
    int* color;
}Point;

typedef struct matrice {
    int MATlength;
    int MATwidth;
    Point** data;
}Matrice;

Matrice mat;

typedef struct gridsquare {
    int* casee;
    int currentpos;
}Case;

Case** grid;
int subdiv = 20;


void initSDL()
/*initialise SDL*/
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width+600, height, 0, &window, &renderer);
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
    smallfont = TTF_OpenFont("/home/bastien/2022-2023/Info/TIPE/font.ttf", 12);
    if (!smallfont) {
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

    grid = (Case**)malloc(sizeof(Case*)*subdiv);
    for (int i = 0; i < subdiv; i++) {
        grid[i] = (Case*)malloc(sizeof(Case)*subdiv);
    }

    for (int i = 0; i<subdiv; i++) {
        for (int j = 0; j<subdiv; j++) {
            grid[i][j].casee = (int*)malloc(sizeof(int)*(matwidth*matlength)*2); 
            grid[i][j].currentpos = 0;
        }
    }


    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            mat.data[i][j].x = (width-matlength*2*1.5*pradius)/2+i*pradius*2*1.5;
            mat.data[i][j].y = (height-matwidth*2*1.5*pradius)/2+j*pradius*2*1.5; 
            mat.data[i][j].xdirection = 1;
            mat.data[i][j].ydirection = 1;

            mat.data[i][j].vx = 0;
            mat.data[i][j].vy = 0;
            mat.data[i][j].ax = 5;
            mat.data[i][j].ay = 0;

            mat.data[i][j].color = (int*)malloc(sizeof(int)*3);
            mat.data[i][j].color[0] = 0;
            mat.data[i][j].color[1] = 0;
            mat.data[i][j].color[2] = 255;

            grid[(int)mat.data[i][j].y/(subdiv*2)][(int)mat.data[i][j].x/(subdiv*2)].casee[grid[(int)mat.data[i][j].y/(subdiv*2)][(int)mat.data[i][j].x/(subdiv*2)].currentpos] = i;
            grid[(int)mat.data[i][j].y/(subdiv*2)][(int)mat.data[i][j].x/(subdiv*2)].casee[grid[(int)mat.data[i][j].y/(subdiv*2)][(int)mat.data[i][j].x/(subdiv*2)].currentpos+1] = j;
            grid[(int)mat.data[i][j].y/(subdiv*2)][(int)mat.data[i][j].x/(subdiv*2)].currentpos += 2;

        }
    }
}