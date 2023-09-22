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
 

const double pi = 3.1415926535;
const double g = 9.80665;
char texte[100];
double dt;


double mu_eau = 0.0001;
double p0 = 0.5; // base density

Uint32 last_time;
Uint32 current_time;
double fq;
double fqq;



double Wpike(double q)
{
    return (15/(pi*pow(h,6)))*pow((h-q),3);
}



double Wvisco(double q)
{
    return (15/(2*pi*pow(h,3))) * (-(q*q*q/(2*pow(h,3))) + (q*q)/(pow(h,2)) + (h/(2*q)) - 1);
}


double calcul_pression(int i, int j)
{
    return k * (mat.data[i][j].density - coeff_visco);
}


double Derive_partielle1ere_Q(double q, double h)
{
    fq = Wpike(q + h) - Wpike(q - h);
    return fq/(2*h);
}

double Derive_partielle2nd_Q(double q, double h) {
    fqq = Wvisco(q + h) - 2.0 * Wvisco(q) + Wvisco(q - h);
    return fqq / (h * h);
}   

void Calcul_density()
{
    double q, sigma;
    for (int k = 0; k<matlength+numof_particle_added; k++) {
        for (int n = 0; n<matlength+numof_particle_added; n++) {
            mat.data[k][n].density = 0;
            sigma = 0;
            for (int i = 0; i<matlength+numof_particle_added; i++) {
                for (int j = 0; j<matwidth+numof_particle_added; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = (abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y))/h;
                        if (q<1) {
                            sigma += pow((h*h-q*q),3);
                        }
                    }
                }
            }
            mat.data[k][n].density = ((315*m) / (64*pi*pow(h,9))) * sigma;
            // printf("mat.data[%d][%d].density = %lf\n", k, n, mat.data[k][n].density);
        }
    }
}


void viscosity()
{
    double sigmaX, sigmaY, q;
    for (int k = 0; k<matlength+numof_particle_added; k++) {
        for (int n = 0; n<matwidth+numof_particle_added; n++) {
            sigmaX = 0;
            sigmaY= 0;
            for (int i = 0; i<matlength+numof_particle_added; i++) {
                for (int j = 0; j<matwidth+numof_particle_added; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = (abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y))/h;
                        if (q>0&&q<1) {
                            sigmaX += m*((abs(mat.data[i][j].vx-mat.data[k][n].vx))/mat.data[i][j].density)*(45/(pi*pow(h,6)))*(h-q);
                            sigmaY += m*((abs(mat.data[i][j].vy-mat.data[k][n].vy))/mat.data[i][j].density)*(45/(pi*pow(h,6)))*(h-q);   


                            // printf("Derive_partielle2nd_Q(%-2lf,%-2lf) = %lf\n", q, h, Derive_partielle2nd_Q(q,h));
                            // printf("mat.data[%d][%d].density = %lf\n", i, j, mat.data[i][j].density);
                        }
                    }
                }
            }   
            // printf("sigmaX = %lf, sigmaY = %lf\n", sigmaX, sigmaY);
            mat.data[k][n].vx += mu_eau*sigmaX;
            mat.data[k][n].vy += mu_eau*sigmaY;
        }
    }
}

void pressure()
{
    double sigma, q;
     for (int k = 0; k<matlength+numof_particle_added; k++) {
        for (int n = 0; n<matwidth+numof_particle_added; n++) {
            sigma = 0;
            for (int i = 0; i<matlength+numof_particle_added; i++) {
                for (int j = 0; j<matwidth+numof_particle_added; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = (abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y))/h;
                        if (q>0&&q<1) {
                            sigma += m*((calcul_pression(k,n)+calcul_pression(i,j))/(2*mat.data[i][j].density))*Derive_partielle1ere_Q(q,h); 
                            // printf("calcul_pression(k,n) = %lf, calcul_pression(i,j) = %lf\n", calcul_pression(k,n),calcul_pression(i,j));
                        }
                    }
                }
            }
            // printf("sigmaP = %lf\n", sigma);
            
            mat.data[k][n].vx -= sigma*dt;
            mat.data[k][n].vy -= sigma*dt;
            // mat.data[k][n].color[0] = (sigma*10)/255;
            // mat.data[k][n].color[1] = (sigma*20)/255;
            // mat.data[k][n].color[2] = (sigma*30)/255;
        }
     }
}

double Eularian_distance(int i, int j, int k, int n)
{
    return abs(mat.data[k][n].x-mat.data[i][j].x)+abs(mat.data[k][n].y-mat.data[i][j].y);
}

void collision() {
    int x, y;
    for (int k = 0 ; k<matlength+numof_particle_added ; k++) {
        for (int n = 0 ; n<matwidth+numof_particle_added ; n++ ) {
            for (int i = 0; i<matlength+numof_particle_added; i++) {
                for (int j = 0; j<matwidth+numof_particle_added; j++) {
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
                                mat.data[k][n].color[0] = (i*1)%255;
                                mat.data[k][n].color[1] = (k*20)%255;
                                mat.data[k][n].color[2] = (j*30)%255;
                    
                                
                        }
                    }   
                }   
            }
        }
    }
}




void particle_out_of_the_grid()
/*Détecte une sortie de l'écran*/
{
    for (int i = 0; i<matlength+numof_particle_added; i++) { 
        for (int j = 0; j<matwidth+numof_particle_added; j++) {
            O+=4;           
            if (mat.data[i][j].x<(width/40)+pradius && mat.data[i][j].xdirection!= 1) {
                mat.data[i][j].xdirection = 1;
                mat.data[i][j].vx *= 0.5;
            } else if (mat.data[i][j].x>width-(width/40)-pradius && mat.data[i][j].xdirection!= -1) {    
                mat.data[i][j].xdirection = -1;
                mat.data[i][j].vx *= 0.5;
            }
            if (mat.data[i][j].y<(height/40)+pradius && mat.data[i][j].ydirection!= 1) {
                mat.data[i][j].ydirection = 1;
                mat.data[i][j].vy *= 0.5;
            } else if (mat.data[i][j].y>height-(height/40)-pradius && mat.data[i][j].ydirection!= -1) {
                mat.data[i][j].ydirection = -1;
                mat.data[i][j].vy *= 0.5;
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

    draw_scale();

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);

    for (int i = 0 ; i<matlength+numof_particle_added ; i++ ){
        for (int j = 0 ; j<matwidth+numof_particle_added ; j++ ) {
            if (mat.data[i][j].ydirection == -1) {
                mat.data[i][j].ydirection = 1;
            }
            mat.data[i][j].color[0] = 0;
            mat.data[i][j].color[1] = 0;
            mat.data[i][j].color[2] = 255;
        }
    }

    Calcul_density();
    pressure();
    viscosity();
    collision(); 
    particle_out_of_the_grid();


    for (int i = 0 ; i < matlength ; i++) {
        for (int j = 0 ; j < matwidth ; j++) {

            
            mat.data[i][j].vy += m*g*dt;
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
    int running = 1, paused, mousex, mousey;
    SDL_Event Event, Pause;
    initSDL();
    initTTF();
    Uint32 start_time, end_time, elapsed_time = 1, pausedtime;
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
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_I ){
                                        Colorflipped();
                                    }
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_R) {
                                        reset_const();
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
                        dt = 1/(FPS*10)*5;
                        for (int i = 0; i<10; i++) {
                            update();
                        }
                        dt = 1/(FPS/10);
                    }
                    if (Event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                        dt = -1/(FPS/10)*5;
                        for (int i = 0; i<10; i++) {
                            update();
                        }
                        dt = 1/(FPS/10);
                    }

                    if (Event.key.keysym.scancode == SDL_SCANCODE_I ){
                        Colorflipped();
                    }
                    if (Event.key.keysym.scancode == SDL_SCANCODE_R) {
                        reset_const();
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
                case SDL_MOUSEBUTTONDOWN:

                    SDL_GetMouseState(&mousex,&mousey);
                    if (mousex > 0 && mousex < height && mousey > 0 && mousey < width) {
                        // addparticle(mousex,mousey);
                        printf("Done\n");
                    }
                    if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40 && mousey<(height/4+height/20+height/400)+height/100) {
                        FPS = 120*(mousex-width-width/30)/(width/6);
                        xFPS = mousex;
                    } else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+(height/4-height/10)/5 && mousey<(height/4+height/20+height/400)+height/100+(height/4-height/10)/5) {
                        h = 20*(mousex-width-width/30)/(width/6);
                        xh = mousex;
                    } else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*2 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)) {
                        m = 10*(mousex-width-width/30)/(width/6)+1;
                        xm = mousex;
                    } else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*3 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*1.5) {
                        coeff_visco = 0.01*(mousex-width-width/30)/(width/6);
                        x_coeff_visco = mousex;
                    }else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*4 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*2.5) {
                        k = 50000*(mousex-width-width/30)/(width/6);
                        xk = mousex;
                    }       
                    
                    break;

            }

        }
        dt = 10/FPS;
        update();
        end_time = SDL_GetTicks();


        elapsed_time = end_time - start_time;
        
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
