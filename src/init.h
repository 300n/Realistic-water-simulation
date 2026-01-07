// gcc TIPE.c -lSDL2 -lSDL2_ttf -lm -Wno-format -Wall -Werror -Wpedantic && ./a.out 
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


#define matwidth 50
#define matlength 50
#define width 800
#define height 800
#define widthstats 525
#define widthscale 295
#define pradius 3
int x_right = width+widthstats;
int x_left = 0;
int y_up = 0;
int y_down = height;

#define DAMPING_COEFFICIENT 0.9



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
int temperature = 20;
int numof_particle_added = 0;

double const FPSconst = 20; // nombre d'image par seconde 
double FPS = FPSconst;
int xFPS = width+widthstats/30*2+widthstats/4;
int yFPS = height/4+height/20;

double const h = 35; // rayon kernel
double smoothing_radius = h;
int xh = width+widthstats/30*2+widthstats/4;
int yh = height/4+height/20+(height/4-height/10)/5;

double const mconst = 1; // masse de chaque particule
double m = mconst;
int xm = width+widthstats/30*2;
int ym = height/4+height/20+((height/4-height/10)/5)*2;

double const t_dens = 0.005  /*(double) (matwidth * matlength) / (width * height)*/ ; 
double target_density = t_dens; // target density
int x_tdens = width+widthstats/30*2;
int y_tdens = height/4+height/20+((height/4-height/10)/5)*3;


double const k = 300000; 
double pressure_multiplier = k; 
int xk = width+widthstats/30*2+widthstats/4;
int yk = height/4+height/20+((height/4-height/10)/5)*4;

double near_pressure_multiplier = 1;


int const numofseparation = 30;
int particle_visible = 1;
int z = 0;

double const viscosity_strength_const = 0.5;
double viscosity_strength = viscosity_strength_const;
int x_vs = width+widthstats/30*2+widthstats/4;
int y_vs = height/4+height/20+((height/4-height/10)/5)*5;

const double pi = 3.1415926535;
const double g = 9.80665;

static const Uint32 hashK1 = 15823;
static const Uint32 hashK2 = 9737333;


double mouse_force = 50;
bool stat_visual_status = true;

typedef struct {
    double x,y;
}Vect2D;

typedef struct {
    double density;
    double near_density;
    Vect2D position;
    Vect2D predicted_position;
    Vect2D velocity;
    Vect2D force;
    double pressure;
    int* color;
    bool inside_cadre;
    int masse;
    int rayon;
}Particule;

typedef struct matrice {
    int MATlength;
    int MATwidth;
    Particule** data;
    int* particle_on_top;
}Matrice;

Matrice particle_grid;

typedef struct gridsquare {
    int* casee;
    int currentpos;
}Case;

Case** grid;
int subdiv = 30;

typedef struct 
{
    int value;
    int index;
}Couple;

typedef struct
{
    double first_value, second_value;
}double2;



Couple* Spatial_Lookup;
int* start_indices;


void initSDL(void)
/*initialise SDL*/
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width+widthstats, height, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
}

int initTTF(void)
/*initialise la police d'ecriture*/
{
    if (TTF_Init() != 0) {
        printf("Erreur : %s\n", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    font = TTF_OpenFont("font.ttf", 18);
    if (!font) {
        printf("Erreur : impossible d'ouvrir le fichier de police\n");
        TTF_Quit();
        SDL_Quit();
    }
    smallfont = TTF_OpenFont("font.ttf", 12);
    if (!smallfont) {
        printf("Erreur : impossible d'ouvrir le fichier de police\n");
        TTF_Quit();
        SDL_Quit();
    }
    return 0;
}

void initmat(void)
{
    particle_grid.MATlength = matlength;
    particle_grid.MATwidth = matwidth;
    particle_grid.data = (Particule**)malloc(sizeof(Particule*)*particle_grid.MATlength);
    for (int i = 0; i < particle_grid.MATlength; i++) {
        particle_grid.data[i] = (Particule*)malloc(sizeof(Particule)*particle_grid.MATwidth);
    }

    Spatial_Lookup = (Couple*)malloc(sizeof(Couple)*(particle_grid.MATlength*particle_grid.MATwidth));
    start_indices = (int*)malloc(sizeof(int)*particle_grid.MATlength*particle_grid.MATwidth);

    particle_grid.particle_on_top = (int*)calloc(numofseparation, sizeof(int));  
    
    srand(time(0));             
    
    // Calculer l'espacement et la taille de la grille
    double spacing = 8;  // Espacement entre particules
    double grid_width = (matlength - 1) * spacing;
    double grid_height = (matwidth - 1) * spacing;
    
    // Calculer le décalage pour centrer la grille
    double offset_x = x_left + (x_right - x_left - grid_width) / 2.0;
    double offset_y = y_up + (y_down - y_up - grid_height) / 2.0;
    
    // S'assurer que le décalage est positif
    if (offset_x < x_left + pradius) {
        offset_x = x_left + pradius;
        spacing = (x_right - x_left - 2 * pradius) / (double)(matlength - 1);
    }
    if (offset_y < y_up + pradius) {
        offset_y = y_up + pradius;
        spacing = fmin(spacing, (y_down - y_up - 2 * pradius) / (double)(matwidth - 1));
    }
    
    for (int i = 0; i < particle_grid.MATlength; i++) {
        for (int j = 0; j < particle_grid.MATwidth; j++) {
            // Position avec espacement adaptatif
            particle_grid.data[i][j].position.x = offset_x + i * spacing;
            particle_grid.data[i][j].position.y = offset_y + j * spacing;
            
            // Clamp pour garantir qu'on reste dans les limites
            if (particle_grid.data[i][j].position.x < x_left + pradius) {
                particle_grid.data[i][j].position.x = x_left + pradius;
            }
            if (particle_grid.data[i][j].position.x > x_right - pradius) {
                particle_grid.data[i][j].position.x = x_right - pradius;
            }
            if (particle_grid.data[i][j].position.y < y_up + pradius) {
                particle_grid.data[i][j].position.y = y_up + pradius;
            }
            if (particle_grid.data[i][j].position.y > y_down - pradius) {
                particle_grid.data[i][j].position.y = y_down - pradius;
            }
            
            particle_grid.data[i][j].velocity.x = 0;
            particle_grid.data[i][j].velocity.y = 0;
            particle_grid.data[i][j].inside_cadre = true;
            particle_grid.data[i][j].color = (int*)malloc(sizeof(int)*3);
            particle_grid.data[i][j].color[0] = 0;
            particle_grid.data[i][j].color[1] = 0;
            particle_grid.data[i][j].color[2] = 255;
            particle_grid.data[i][j].force.y = 3;
            particle_grid.data[i][j].masse = 0;
            particle_grid.data[i][j].rayon = 0;
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


void init_Palette(void)
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


void init_prog(void)
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


void reset_const(void)
{
    pressure_multiplier = k;
    smoothing_radius = h;
    m = mconst;
    FPS = FPSconst;
    target_density = t_dens;
    viscosity_strength = viscosity_strength_const;

    xk = width+widthstats/30*2+widthstats/4;
    xh = width+widthstats/30*2+widthstats/4;
    xm = width+widthstats/30*2;
    xFPS = width+widthstats/30*2+widthstats/4;
    x_tdens = width+widthstats/30*2;
    x_vs = width+widthstats/30*2+widthstats/4;

}


Vect2D Vect2D_zero(void)
{
    Vect2D out;
    out.x = 0;
    out.y = 0;
    return out;
}

Vect2D Vect2D_cpy(Vect2D input)
{
    Vect2D out;
    out.x = input.x;
    out.y = input.y;
    return out;
}

Vect2D Vect2D_add(Vect2D a, Vect2D b)
{
    Vect2D out;
    out.x = a.x + b.x;
    out.y = a.y + b.y;
    return out;
}

double2 double2_cpy(double a, double b)
{
    double2 out;
    out.first_value = a;
    out.second_value = b;
    return out;
}
