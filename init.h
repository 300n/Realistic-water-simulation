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


#define matwidth 35
#define matlength 35
#define width 1000
#define height 1000
#define pradius 3
SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font* font;
TTF_Font* smallfont;
SDL_Rect square;
SDL_Rect rect_texte;
SDL_Surface* surface_texte;
SDL_Texture* texture_texte;

SDL_Color white = { 255, 255, 255, SDL_ALPHA_OPAQUE};
char texte[100];
clock_t starttime;
int O = 0;
char comm[256];


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
            mat.data[i][j].ax = 0;
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


typedef struct Couleur
{
    char* name;
    int R;
    int G;
    int B;
}couleur;




couleur* Palette;


void init_Palette()
{
    Palette = (couleur*)malloc(sizeof(couleur)*7);
    for (int i = 0; i<7; i++) {
        Palette[i].name = (char*)malloc(sizeof(char)*20);
    }
    strcpy(Palette[0].name,"PRed"); Palette[0].R = 255; Palette[0].G = 102; Palette[0].B = 102;
    strcpy(Palette[1].name,"POrange"); Palette[1].R = 255; Palette[1].G = 178; Palette[1].B = 102;
    strcpy(Palette[2].name,"PYellow"); Palette[2].R = 255; Palette[2].G = 255; Palette[2].B = 102;
    strcpy(Palette[3].name,"PGreen"); Palette[3].R = 102; Palette[3].G = 255; Palette[3].B = 102;
    strcpy(Palette[4].name,"PCyan"); Palette[4].R = 102; Palette[4].G = 255; Palette[4].B = 255;
    strcpy(Palette[5].name,"PBlue"); Palette[5].R = 102; Palette[5].G = 102; Palette[5].B = 255;
    strcpy(Palette[6].name,"PPink"); Palette[6].R = 255; Palette[6].G = 102; Palette[6].B = 178;
}


typedef struct Proc
{
    long int PID;
    char id[256];
    float cpu_usage;
    couleur color;
}proc;




proc* prog;

unsigned long total_time_all_procs;
unsigned long total_time;
int ppid;
unsigned long utime;
unsigned long stime;
long int tab[40];
char comm[256];
char stat_file_path[256];
int a = 0;


void init_prog()
{
    prog = (proc*)malloc(sizeof(proc)*20);
    for (int i = 0; i<20; i++) {
        prog[i].color.name = (char*)malloc(sizeof(char)*20);
    }
}


char* endptr;
long pid;
unsigned long total_time;
unsigned long delta_time;
float cpu_usage;
float cpu_tot;