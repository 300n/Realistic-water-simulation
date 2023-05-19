#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <float.h>
#include <stdint.h>

#define width 1000
#define height 1000
#define matwidth 35
#define matlength 35
#define Mparticle 20
double FPS = 240;
const double pi = 3.1415926535;
const double g = 9.80665;
SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font* font;
TTF_Font* smallfont;
int O = 0;
char texte[100];
int Width = width;
int Height = height;
int pradius = 4;
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

void initSDL()
/*initialise SDL*/
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(Width+600, Height, 0, &window, &renderer);
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

    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            mat.data[i][j].x = (width-matlength*2*pradius)/2+i*pradius*2*1.5;
            mat.data[i][j].y = (height-matwidth*2*pradius)/2+j*pradius*2*1.5; 
            mat.data[i][j].xdirection = 1;
            mat.data[i][j].ydirection = 1;

            mat.data[i][j].vx = 0;
            mat.data[i][j].vy = (g*Mparticle);
            mat.data[i][j].ax = 0;
            mat.data[i][j].ay = 0;

            mat.data[i][j].color = (int*)malloc(sizeof(int)*3);
            mat.data[i][j].color[0] = 0;
            mat.data[i][j].color[1] = 0;
            mat.data[i][j].color[2] = 255;

        }
    }
}

void draw_Cartesian_axes()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,height/40+(((height-(height/20))/20)),width-(width*3/40)-(width/150),height/40+(((height-(height/20))/20)*2),width-(width*3/40)-(width/150));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*18),(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*19));

    /*tip of arrows*/
    SDL_RenderDrawLine(renderer,height/40+(((height-(height/20))/20)*2),width-(width*3/40)-(width/150),(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)-(width/140));
    SDL_RenderDrawLine(renderer,height/40+(((height-(height/20))/20)*2),width-(width*3/40)-(width/150),(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)-(width/140),(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*18),(height/40*3)-(height/275)-(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*18),(height/40*3)-(height/275)+(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275)-(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140),(height/40*3)-(height/275)+(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140));
    /*txt*/
    sprintf(texte, "1m");
    surface_texte = TTF_RenderText_Blended(smallfont, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = (height/40+(((height-(height/20))/20))+height/40+(((height-(height/20))/20)*2))/2-height/80;
    rect_texte.y = width-(width*3/40)+width/300;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    rect_texte.x = (height/40*3)-1-height/40;
    rect_texte.y = ((width/40)+(((width-(width/20))/20)*(18+19)))/2+width/140;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
}

void draw_grid()
{
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = 0;
    square.y = 0;
    square.h = height;
    square.w = width;
    SDL_RenderFillRect(renderer, &square);
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
    draw_Cartesian_axes();

}

void clear_grid()
{
    SDL_SetRenderDrawColor(renderer,22,22,22,SDL_ALPHA_OPAQUE);
    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            SDL_RenderDrawPoint(renderer,mat.data[i][j].x,mat.data[i][j].y);
        }
    }
}

void drawCircle(int X, int Y, int radius) 
{
    for (int x = X - radius; x <= X + radius; x++) {
        for (int y = Y - radius; y <= Y + radius; y++) {
            if (pow(x - X, 2) + pow(y - Y, 2) <= pow(radius, 2)) {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

void drawstatgrid()
{
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = width;
    square.y = 0;
    square.h = height;
    square.w = width+600;
    SDL_RenderFillRect(renderer, &square);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,width+20,height/40,width+560,height/40);
    SDL_RenderDrawLine(renderer,width+20,height/40,width+20,height-height/40);
    SDL_RenderDrawLine(renderer,width+20,height-height/40,width+560,height-height/40);
    SDL_RenderDrawLine(renderer,width+560,height-height/40,width+560,height/40);

    
}

double get_time()
{
    return (double)(clock() - starttime) / CLOCKS_PER_SEC;
}

int get_number_of_particle()
{
    int out = 0;
    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++ ) {
            if (mat.data[i][j].x>=(width/30)-10 && mat.data[i][j].x<=width-(width/30)+10 && mat.data[i][j].y>=(height/30)-10 && mat.data[i][j].y<=height-(height/30)+10) {
                out++;
            }
        }
    }
    return out;
}

char* complexity()
{
    double n = matwidth*matlength;
    if (O>=n && O<pow(n,2)) {
        return "O(n)";
    } else if (O>=pow(n,2) && O<pow(n,3)) {
        return "O(n^2)";
    } else if (O>=pow(n,3)) {
        return "O(n^3)";
    }
}

void stat_aff(int fps)
/*affichage des statistiques*/ 
{
    sprintf(texte, "fps : %d", fps);
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = width+30;
    rect_texte.y = (width/40)+10;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "nombre d'operations par frame : %d", O);
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "temps ecoule : %-2f s", get_time());
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "nombre de particule dans le cadre : %d", get_number_of_particle());
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "Complexite : T(n) = %s", complexity());
    surface_texte = TTF_RenderText_Blended(font, texte, couleur);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    O = 0;
} 



/*   

                    


*/


void gradiant(double x, double y, double* dx, double* dy)
{
    1;
}
/*
void FSumUpdate()
{
    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            mat.data[i][j].ax = ;
            mat.data[i][j].ay = ;
            mat.data[i][j].vx = *t;
            mat.data[i][j].vy = *t;
            mat.data[i][j].x = *t;
            mat.data[i][j].y = *t;
        }
    }
}*/




void collision() {
    int temp1, temp2, x, y;
    for (int k = 0 ; k<matlength ; k++) {
        for (int n = 0 ; n<matwidth ; n++ ) {
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        if ((abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y)) < pradius*2) {
                            x = mat.data[k][n].x - mat.data[i][j].x;
                            y = mat.data[k][n].y - mat.data[i][j].y;
                            if (abs(x) >= abs(y)) {
                                mat.data[k][n].xdirection = (int)x/abs(x);
                                mat.data[k][n].ydirection = (int)y/abs(x);
                            } else  {
                                mat.data[k][n].xdirection = (int)y/abs(y);
                                mat.data[k][n].ydirection = (int)y/abs(y);
                            }
                            
                            mat.data[k][n].vx = 0.99*((mat.data[i][j].vx+mat.data[k][n].vx)/2);
                            mat.data[k][n].vy = 0.99*((mat.data[i][j].vy+mat.data[k][n].vy)/2);

                        }
                    }   
                }   
            }
        }
    }
}

double calcule_viscosite_eau(int T)
/*calcule de la viscosité de l'eau en fonction de la temperature*/
{
    double viscositer;
    viscositer = pow(1.79*10,-3)*(1/1+0.03368*T+T*0.000211);
    return viscositer ;
}



void particle_out_of_the_grid()
/*Détecte une sortie de l'écran*/
{
    for (int i = 0; i<matlength; i++) { 
        for (int j = 0; j<matwidth ; j++) {
            O+=4;           
            if (mat.data[i][j].x<(width/40)+pradius && mat.data[i][j].xdirection!= 1) {
                mat.data[i][j].vx *= 0.99;
                mat.data[i][j].xdirection = 1;
            } else if (mat.data[i][j].x>width-(width/40)-pradius && mat.data[i][j].xdirection!= -1) {    
                mat.data[i][j].vx *= 0.99;
                mat.data[i][j].xdirection = -1;
            }
            if (mat.data[i][j].y<(height/40)+pradius && mat.data[i][j].ydirection!= 1) {
                mat.data[i][j].vy *= 0.99;
                mat.data[i][j].ydirection = 1;
            } else if (mat.data[i][j].y>height-(height/40)-pradius && mat.data[i][j].ydirection!= -1) {
                mat.data[i][j].vy *= 0.99;
                mat.data[i][j].ydirection = -1;
            }
        }
    }

}

void update(double dt)   
/*mise à jour des positions de chaque particule*/
{
    draw_grid();
    drawstatgrid();
    clear_grid();
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);


    for (int i = 0 ; i<matlength ; i++ ){
        for (int j = 0 ; j<matwidth ; j++ ) {
            mat.data[i][j].color[0] = 0;
            mat.data[i][j].color[1] = 0;
            mat.data[i][j].color[2] = 255;
        }
    }
    collision(); 
    particle_out_of_the_grid();
    for (int i = 0 ; i < matlength ; i++) {
        for (int j = 0 ; j < matwidth ; j++) {

            mat.data[i][j].x += mat.data[i][j].vx*dt*mat.data[i][j].xdirection;
            mat.data[i][j].y += mat.data[i][j].vy*dt*mat.data[i][j].ydirection;
            mat.data[i][j].vx += mat.data[i][j].ax*dt;
            mat.data[i][j].vy += mat.data[i][j].ay*dt;
            
            O++;
            SDL_SetRenderDrawColor(renderer, mat.data[i][j].color[0], mat.data[i][j].color[1], mat.data[i][j].color[2], SDL_ALPHA_OPAQUE);
            drawCircle(mat.data[i][j].x,mat.data[i][j].y,pradius);
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
            switch (Event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_KEYDOWN:
                    if (Event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        running = 0;
                    }
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    switch (Event.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        Width = Event.window.data1;
                        Height = Event.window.data2;
                        break;
                    
                    default:
                        break;
                    }
                    break;
                default:
                    break;
            
            }

        }
        
        update(1/FPS);
        end_time = SDL_GetTicks();
        elapsed_time = end_time - start_time;
        if (elapsed_time < 1000 / FPS) {
            SDL_Delay((1000 / FPS) - elapsed_time);
        }
        stat_aff(FPS);
        SDL_RenderPresent(renderer);
        SDL_UpdateWindowSurface(window);
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
