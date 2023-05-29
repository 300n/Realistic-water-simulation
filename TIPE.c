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

#include "Get_stat.h"



#define Mparticle 20


double FPS = 60;
const double pi = 3.1415926535;
const double g = 9.80665;
char texte[100];
double dt;



double d0 = 1000;
double h = 6;

/*Coefficient de normalisation Cd pour le noyau de Wendland*/
double Cd; /*Valeur de Cd pour un espace 2D (Pour un espace 3D Cd = 21/(6*pi*h*h*h)*/

double m = 0.1;

double mu_eau = 0.0001;

// ?
double p0 = 0;
double density0;
double k = 0.01;


Uint32 last_time;
Uint32 current_time;





double W(double q) {
    return 7/(4*pi*h*h)* pow(1.0 - q, 4) * (4.0 * q + 1.0);
}

double calcul_pression(int i, int j)
{
    return k*(mat.data[i][j].density-density0);
}


double Derive_partielle1ere_Q(double q, double h)
{
    double fq = W(q + h) - W(q - h);
    return fq/(2*h);
}

double Derive_partielle2nd_Q(double q, double h) {
    double fqq = W(q + h) - 2.0 * W(q) + W(q - h);
    return fqq / (h * h);
}   




double Eularian_distance(int i, int j, int k, int n)
{
    return abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y);
}


void Calcul_density()
{
    double q, weight;
    for (int k = 0; k<matlength; k++) {
        for (int n = 0; n<matlength; n++) {
            mat.data[k][n].density = 0;
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = (abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y))/h;
                        if (q<1) {
                            weight = W(q);
                            mat.data[k][n].density += weight*m;
                        }
                    }
                }
            }
        }
    }
}


void viscosity()
{
    double sigmaX, sigmaY, q;
    for (int k = 0; k<matlength ; k++) {
        for (int n = 0; n<matwidth; n++) {
            sigmaX = 0;
            sigmaY= 0;
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = (abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y))/h;
                        if (q<1) {
                            sigmaX += m*(abs(mat.data[i][j].vx-mat.data[k][n].vx))*Derive_partielle2nd_Q(q,h)/mat.data[i][j].density;
                            sigmaY += m*(abs(mat.data[i][j].vy-mat.data[k][n].vy))*Derive_partielle2nd_Q(q,h)/mat.data[i][j].density;

                        }
                    }
                }
            }
            mat.data[k][n].vx += mu_eau*sigmaX;
            mat.data[k][n].vy += mu_eau*sigmaY;
        }
    }
}

void pressure()
{
    double sigma, q;
     for (int k = 0; k<matlength ; k++) {
        for (int n = 0; n<matwidth; n++) {
            sigma = 0;
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = (abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y))/h;
                        if (q<1) {
                            sigma += m*((calcul_pression(k,n)+calcul_pression(i,j))/(2*mat.data[i][j].density))*Derive_partielle1ere_Q(q,h); 
                        }
                    }
                }
            }
            mat.data[k][n].vx -= sigma;
            mat.data[k][n].vy -= sigma;
        }
     }
}



void collision() {
    int temp1, temp2, x, y;
    for (int k = 0 ; k<matlength ; k++) {
        for (int n = 0 ; n<matwidth ; n++ ) {
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        
                        if (Eularian_distance(k,n,i,j) < pradius*2) {
                                x = mat.data[k][n].x - mat.data[i][j].x;
                                y = mat.data[k][n].y - mat.data[i][j].y;
                                if (x<0) {
                                    mat.data[k][n].xdirection = -1;
                                } else if (x>0) {
                                    mat.data[k][n].xdirection = 1;
                                }
                                if (y<0) {
                                    mat.data[k][n].ydirection = -1;
                                } else if (y>0) {
                                    mat.data[k][n].ydirection = 1;
                                }
                                mat.data[k][n].vx = 0.9*((mat.data[i][j].vx+mat.data[k][n].vx)/2);
                                mat.data[k][n].vy = 0.9*((mat.data[i][j].vy+mat.data[k][n].vy)/2);

                                
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

}



void particle_out_of_the_grid()
/*Détecte une sortie de l'écran*/
{
    for (int i = 0; i<matlength; i++) { 
        for (int j = 0; j<matwidth ; j++) {
            O+=4;           
            if (mat.data[i][j].x<(width/40)+pradius && mat.data[i][j].xdirection!= 1) {
                mat.data[i][j].xdirection = 1;
                mat.data[i][j].vx *= 0.9;
            } else if (mat.data[i][j].x>width-(width/40)-pradius && mat.data[i][j].xdirection!= -1) {    
                mat.data[i][j].xdirection = -1;
                mat.data[i][j].vx *= 0.9;
            }
            if (mat.data[i][j].y<(height/40)+pradius && mat.data[i][j].ydirection!= 1) {
                mat.data[i][j].ydirection = 1;
                mat.data[i][j].vy *= 0.9;
            } else if (mat.data[i][j].y>height-(height/40)-pradius && mat.data[i][j].ydirection!= -1) {
                mat.data[i][j].ydirection = -1;
                mat.data[i][j].vy *= 0.9;
            }
        }
    }

}

void update()   
/*mise à jour des positions de chaque particule*/
{
    clear_grid();
    
    drawstatgrid();
    draw_grid();
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);

    for (int i = 0 ; i<matlength ; i++ ){
        for (int j = 0 ; j<matwidth ; j++ ) {
            if (mat.data[i][j].ydirection == -1) {
                mat.data[i][j].ydirection = 1;
            }
            mat.data[i][j].color[0] = 0;
            mat.data[i][j].color[1] = 0;
            mat.data[i][j].color[2] = 255;
        }
    }
    Calcul_density();
    collision(); 
    particle_out_of_the_grid();
    viscosity();
    pressure();


    for (int i = 0 ; i < matlength ; i++) {
        for (int j = 0 ; j < matwidth ; j++) {

            mat.data[i][j].vx += mat.data[i][j].ax*dt;
            mat.data[i][j].vy += g*dt;
            mat.data[i][j].x += mat.data[i][j].vx*dt*mat.data[i][j].xdirection;
            mat.data[i][j].y += mat.data[i][j].vy*dt*mat.data[i][j].ydirection;
            O++;
            SDL_SetRenderDrawColor(renderer, mat.data[i][j].color[0], mat.data[i][j].color[1], mat.data[i][j].color[2], SDL_ALPHA_OPAQUE);
            drawCircle(mat.data[i][j].x,mat.data[i][j].y,pradius);
        }
    }
}

void aff()
/*affichage des particules*/
{
    int a, x, y, running = 1, paused;
    SDL_Event Event, Pause;
    initSDL();
    initTTF();
    Uint32 start_time, end_time, elapsed_time, pausedtime;
    restart:
    initmat();
    init_prog();
    init_Palette();
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
                    //printf("Touche pressée : %s\n", SDL_GetKeyName(Event.key.keysym.sym));
            
                    if (Event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        running = 0;
                    }
                    if (Event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                        pausedtime = (double)(clock() - starttime);
                        paused = 1;
                        while (paused) {
                            while (SDL_PollEvent(&Pause)) {
                                if (Pause.type == SDL_KEYDOWN ) {
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                                        paused = 0;
                                    }
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                                        paused = 0;
                                        running = 0;
                                    }
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_F5) {
                                        goto restart;
                                    }
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                                        dt = 1/FPS*5;
                                        for (int i = 0; i<10; i++) {
                                            update();
                                        }
                                        stat_aff(FPS);
                                        SDL_RenderPresent(renderer);
                                        dt = 1/FPS;
                                    }
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                                        dt = -1/FPS*5;
                                        for (int i = 0; i<10; i++) {
                                            update();
                                        }
                                        stat_aff(FPS);
                                        SDL_RenderPresent(renderer);
                                        dt = 1/FPS;
                                    }
                                } else if (Pause.type == SDL_QUIT) {
                                    paused = 0;
                                    running = 0;
                                }
                            }
                        }
                        starttime += (double)((clock() - starttime)-pausedtime);
                    }
                    if (Event.key.keysym.scancode == SDL_SCANCODE_F5) {
                        goto restart;
                    }
                    if (Event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                        dt = 1/FPS*5;
                        for (int i = 0; i<10; i++) {
                            update();
                        }
                        dt = 1/FPS;
                    }
                    if (Event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                        dt = -1/FPS*5;
                        for (int i = 0; i<10; i++) {
                            update();
                        }
                        dt = 1/FPS;
                    }
                
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    switch (Event.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        break;
                    
                    default:
                        break;
                    }   
                    break;
                default:
                    break;
            
            }

        }
        dt = 1/FPS;
        update();
        end_time = SDL_GetTicks();
        elapsed_time = end_time - start_time;
        if (elapsed_time < 1000 / FPS) {
            SDL_Delay((1000 / FPS) - elapsed_time);
        }
        
        stat_aff(elapsed_time);
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