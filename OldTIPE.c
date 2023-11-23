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

#define SMOOTHING_LENGTH 10
#define NORMALIZATION_DENSITY (315 * m / (64 * pi * pow(SMOOTHING_LENGTH, 9)))
#define NORMALIZATION_PRESSURE_FORCE (-45 * m / (pi * pow(SMOOTHING_LENGTH, 6)))
#define NORMALIZATION_VISCOUS_FORCE (45 * coeff_visco * m / (pi * pow(SMOOTHING_LENGTH, 6)))


#define CONSTANT_FORCE_X 0.0
#define CONSTANT_FORCE_Y 10  

#define DAMPING_COEFFICIENT -0.9
#define COLLISION_COEFFICIENT 0.8

#define GAS_CONSTANT 1000
#define REST_DENSITY 1000


void addparticles(double x, double y)
{
    int i = 0;
    if (particle_grid.data[particle_grid.MATlength-1][particle_grid.MATwidth-1].exist) {
        particle_grid.MATlength += 1;
        particle_grid.data[particle_grid.MATlength-1] = (Particule*)malloc(sizeof(Particule)*particle_grid.MATwidth);
        for (int j = 0; j<particle_grid.MATwidth ; j++) {
            particle_grid.data[particle_grid.MATlength-1][j].exist = false;
            particle_grid.data[particle_grid.MATlength-1][j].color = (int*)calloc(0,sizeof(int)*3);
            particle_grid.data[particle_grid.MATlength-1][j].color[2] = 255;
            particle_grid.data[particle_grid.MATlength-1][j].position.x = -100;
        }
    } else {

        while (particle_grid.data[particle_grid.MATlength-1][i].exist) {
            i++;
        }
        particle_grid.data[particle_grid.MATlength-1][i].exist = true;

    }
    particle_grid.data[particle_grid.MATlength-1][i].force.y = 3;
    particle_grid.data[particle_grid.MATlength-1][i].position.x = x;
    particle_grid.data[particle_grid.MATlength-1][i].position.y = y;
        
    
}


void particleonttop()
{
    int maxheight, temp = 0;
    for (int k = 0; k< (numofseparation); k++) {
        maxheight = height;
        for (int i = 0; i<matlength; i++) {
            for (int j = 0; j<matwidth; j++) {
                if(particle_grid.data[i][j].position.x>=(numofseparation)*k && particle_grid.data[i][j].position.x<=(numofseparation)*(k+1) && particle_grid.data[i][j].position.y<maxheight) {
                    maxheight = particle_grid.data[i][j].position.y;
                    particle_grid.particle_on_top[temp] = i*matlength+j;
                }   
            }
        }
        temp++;
    }
}

void align()
{
    int x1, y1, x2, y2;
    SDL_SetRenderDrawColor(renderer,0,0,RGB_txt,SDL_ALPHA_OPAQUE);
    for (int k = 0; k< (numofseparation); k++) {
        O++;
        x1 = particle_grid.particle_on_top[k]/matwidth;
        x2 = particle_grid.particle_on_top[k+1]/matwidth;
        y1 = particle_grid.particle_on_top[k]%matlength;
        y2 = particle_grid.particle_on_top[k+1]%matlength;
        if (particle_grid.particle_on_top[k]>0 && particle_grid.data[x1][y1].position.x<particle_grid.data[x2][y2].position.x) {
            SDL_RenderDrawLine(renderer,particle_grid.data[x1][y1].position.x,particle_grid.data[x1][y1].position.y,particle_grid.data[x2][y2].position.x,particle_grid.data[x2][y2].position.y);
        }
    }
}


void particle_near()
{
    int q, l = 0;
    for (int k = 0; k<matlength; k++) {
        for (int n = 0; n<matwidth; n++) {
            l = 0;
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {                    
                        q = (fabs(particle_grid.data[k][n].position.x-particle_grid.data[i][j].position.x)+fabs(particle_grid.data[k][n].position.y-particle_grid.data[i][j].position.y))/h;
                        if (q>0&&q<1) {
                            particle_grid.data[k][n].part_near[l] = i*matlength+j;
                        }
                    }
                }
            }
        }
    }
}

double Eularian_distance(int i, int j, int k, int n)
{
    return fabs(particle_grid.data[k][n].position.x-particle_grid.data[i][j].position.x)+fabs(particle_grid.data[k][n].position.y-particle_grid.data[i][j].position.y);
}

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
    return k * (particle_grid.data[i][j].density - coeff_visco);
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
    for (int k = 0; k<matlength; k++) {
        for (int n = 0; n<matlength; n++) {
            particle_grid.data[k][n].density = 0;
            sigma = 0;
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = Eularian_distance(k,n,i,j)/h;
                        if (q>0&&q<1) {
                            sigma += pow((h*h-q*q),3);
                        }
                    }
                }
            }
            particle_grid.data[k][n].density = ((315*m) / (64*pi*pow(h,9))) * sigma;
            printf("particle_grid.data[%d][%d].density = %lf\n",k,n, particle_grid.data[k][n].density);
        }
    }
}

void viscosity()
{
    double pressure, sigmaX, sigmaY, q, dx, dy;
    for (int k = 0; k<matlength; k++) {
        for (int n = 0; n<matwidth; n++) {
            sigmaX = 0;
            sigmaY= 0;
            for (int i = 0; i<matlength; i++) {
                for (int j = 0; j<matwidth; j++) {
                    O++;
                    if (i!=k || j!=n) {
                        q = Eularian_distance(k,n,i,j)/h;
                        if (q>0&&q<1) {
                            dx = particle_grid.data[i][j].position.x - particle_grid.data[k][n].position.x;
                            dy = particle_grid.data[i][j].position.y - particle_grid.data[k][n].position.y;
                            sigmaX += m*((fabs(particle_grid.data[i][j].force.x-particle_grid.data[k][n].force.x))/particle_grid.data[i][j].density)*(45/(pi*pow(h,6)))*(h-q);
                            sigmaY += m*((fabs(particle_grid.data[i][j].force.y-particle_grid.data[k][n].force.y))/particle_grid.data[i][j].density)*(45/(pi*pow(h,6)))*(h-q);   

                            pressure += m*((calcul_pression(k,n)+calcul_pression(i,j))/(2*particle_grid.data[i][j].density))*Derive_partielle1ere_Q(q,h); 
                        }
                    }
                }
            }   


            particle_grid.data[k][n].force.x = (pressure+mu_eau*sigmaX)*dx;
            particle_grid.data[k][n].force.y = (pressure+mu_eau*sigmaY)*dy;
            particle_grid.data[k][n].force.x += CONSTANT_FORCE_X;
            particle_grid.data[k][n].force.y += CONSTANT_FORCE_Y;
            // printf("force.x = %lf\n", particle_grid.data[k][n].force.x);
        }
    }
}

// void pressure()
// {
//     double sigma, q;
//      for (int k = 0; k<matlength; k++) {
//         for (int n = 0; n<matwidth; n++) {
//             sigma = 0;
//             for (int i = 0; i<matlength; i++) {
//                 for (int j = 0; j<matwidth; j++) {
//                     O++;
//                     if (i!=k || j!=n) {
//                         q = Eularian_distance(k,n,i,j)/h;
//                         if (q>0&&q<1) {
//                             pressure += m*((calcul_pression(k,n)+calcul_pression(i,j))/(2*particle_grid.data[i][j].density))*Derive_partielle1ere_Q(q,h); 
//                         }
//                     }
//                 }
//             }
//             particle_grid.data[k][n].force.x -= pressure*dt;
//             particle_grid.data[k][n].force.y -= pressure*dt;
//         }
//     }
// }





// void collision() 
// {
//     int x, y;
//     for (int k = 0 ; k<matlength ; k++) {
//         for (int n = 0 ; n<matwidth ; n++ ) {
//             for (int i = 0; i<matlength; i++) {
//                 for (int j = 0; j<matwidth; j++) {
//                     if (i!=k || j!=n ) {
//                         if (Eularian_distance(k,n,i,j) < pradius*2+pradius/10) { // Regarde si la distance entre les deux particules est inférieur à 2x le rayon d'une particule "Détection de collision"
//                             x = particle_grid.data[k][n].position.x - particle_grid.data[i][j].position.x; // Phase pour déterminer la direction de la particule après la collision 
//                             y = particle_grid.data[k][n].position.y - particle_grid.data[i][j].position.y;
//                             if (x < 0) {
//                                 if (particle_grid.data[k][n].velocity.x >= 0) {
//                                     particle_grid.data[k][n].velocity.x *=-1;
//                                 }
//                             } else if (x > 0) {
//                                 if (particle_grid.data[k][n].velocity.x < 0) {
//                                     particle_grid.data[k][n].velocity.x *=-1;
//                                 }
//                             }
//                             if (y < 0) {
//                                 if (particle_grid.data[k][n].velocity.y >= 0) {
//                                     particle_grid.data[k][n].velocity.y *=-1;
//                                 }
//                             } else if (y > 0) { 
//                                 if (particle_grid.data[k][n].velocity.y < 0) {
//                                     particle_grid.data[k][n].velocity.y *=-1;
//                                 }
//                             }
//                             particle_grid.data[i][j].force.y = 0.9*((particle_grid.data[i][j].force.y+particle_grid.data[k][n].force.y)/2);
//                             particle_grid.data[i][j].force.x = 0.9*((particle_grid.data[i][j].force.x+particle_grid.data[k][n].force.x)/2);
//                             particle_grid.data[i][j].velocity.y = COLLISION_COEFFICIENT*((particle_grid.data[i][j].velocity.y+particle_grid.data[k][n].velocity.y)/2);
//                             particle_grid.data[i][j].velocity.x = COLLISION_COEFFICIENT*((particle_grid.data[i][j].velocity.x+particle_grid.data[k][n].velocity.x)/2);  // Mise à jour de la vistesse de la particule                     
//                             particle_grid.data[i][j].color[0] = ((i*20)/2)%255;
//                             particle_grid.data[i][j].color[1] = ((k*1)/2)%255;
//                             particle_grid.data[i][j].color[2] = ((j*20)/2)%255;
//                         }
//                     }   
//                 }   
//             }
//         }
//     }
// }







// void sumforce()
// {
//     double q, w_density, w_pressure, w_viscosity, pressure_term;
//     for (int i = 0; i<matlength; i++) {
//         for (int j = 0; j<matwidth; j++) {
//             particle_grid.data[i][j].density = 0;
//             particle_grid.data[i][j].force.x = 0;
//             particle_grid.data[i][j].force.y = 0;
//             for (int k = 0; k<matlength; k++) {
//                 for (int n = 0; n<matwidth; n++) {
//                     O++;
//                     if (i!=k || j!=n) {
//                         q = Eularian_distance(k,n,i,j)/h;
//                         if (q<1) {
//                             // Calcul du gradient de la fonction de noyau SPH par rapport à x (gradient_of_w_x)
//                             double gradient_of_w_x = -NORMALIZATION_DENSITY * 45.0 / (pi * h * h * h) * (particle_grid.data[i][j].position.x - particle_grid.data[k][n].position.x) * pow(1 - q, 2);
//                             // Calcul du gradient de la fonction de noyau SPH par rapport à y (gradient_of_w_y)
//                             double gradient_of_w_y = -NORMALIZATION_DENSITY * 45.0 / (pi * h * h * h) * (particle_grid.data[i][j].position.y - particle_grid.data[k][n].position.y) * pow(1 - q, 2);
//                             // Calcul du Laplacien de la fonction de noyau SPH par rapport à x (Laplacian_of_w_x)
//                             double Laplacian_of_w_x = NORMALIZATION_VISCOUS_FORCE * 45.0 / (pi * h * h * h) * (particle_grid.data[i][j].position.x - particle_grid.data[k][n].position.x) * (1 - q);
//                             // Calcul du Laplacien de la fonction de noyau SPH par rapport à y (Laplacian_of_w_y)
//                             double Laplacian_of_w_y = NORMALIZATION_VISCOUS_FORCE * 45.0 / (pi * h * h * h) * (particle_grid.data[i][j].position.y - particle_grid.data[k][n].position.y) * (1 - q);
//                             particle_grid.data[i][j].density += w_density*m;
//                             particle_grid.data[i][j].pressure = GAS_CONSTANT * (particle_grid.data[i][j].density - REST_DENSITY);
//                             particle_grid.data[i][j].force.x += -m * (particle_grid.data[i][j].pressure / (2 * particle_grid.data[i][j].density)) * gradient_of_w_x;
//                             particle_grid.data[i][j].force.y += -m * (particle_grid.data[i][j].pressure / (2 * particle_grid.data[i][j].density)) * gradient_of_w_y;
//                             particle_grid.data[i][j].force.x += coeff_visco * m * (particle_grid.data[k][n].velocity.x - particle_grid.data[i][j].velocity.x) * Laplacian_of_w_x;
//                             particle_grid.data[i][j].force.y += coeff_visco * m * (particle_grid.data[k][n].velocity.y - particle_grid.data[i][j].velocity.y) * Laplacian_of_w_y;
//                             printf("particle_grid.data[%d][%d].pressure = %lf\n",i,j, particle_grid.data[i][j].pressure);
//                             printf("particle_grid.data[%d][%d].density = %lf\n",i,j, particle_grid.data[i][j].density);                        
//                         }
//                     }
//                 }
//             }
//             particle_grid.data[i][j].force.x += CONSTANT_FORCE_X;
//             particle_grid.data[i][j].force.y += CONSTANT_FORCE_Y;
//         }
//     }
// }

































void particle_out_of_the_grid()
/*Détecte une sortie de l'écran*/
{
    for (int i = 0; i<particle_grid.MATlength; i++) { 
        for (int j = 0; j<particle_grid.MATwidth; j++) {
            O+=4;           
            if (particle_grid.data[i][j].position.x<x_left+pradius) {
                particle_grid.data[i][j].position.x = x_left+pradius;
                particle_grid.data[i][j].velocity.x *= DAMPING_COEFFICIENT;
            } else if (particle_grid.data[i][j].position.x>x_right-pradius) {    
                particle_grid.data[i][j].position.x = x_right-pradius;
                particle_grid.data[i][j].velocity.x *= DAMPING_COEFFICIENT;
            }
            if (particle_grid.data[i][j].position.y<y_up+pradius) {
                particle_grid.data[i][j].position.y = y_up+pradius;
                particle_grid.data[i][j].velocity.y *= DAMPING_COEFFICIENT;
            } else if (particle_grid.data[i][j].position.y>y_down-pradius) {
                particle_grid.data[i][j].position.y = y_down-pradius;
                particle_grid.data[i][j].velocity.y *= DAMPING_COEFFICIENT;
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

    if (particle_visible == -1) {
        particleonttop();
        align();
    }
    
    

    // Calcul_density();
    // pressure();
    // viscosity();

    
    // sumforce();

    // collision(); 
    particle_out_of_the_grid();


    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            particle_grid.data[i][j].velocity.x += (dt*particle_grid.data[i][j].force.x);
            particle_grid.data[i][j].velocity.y += (dt*particle_grid.data[i][j].force.y);

            particle_grid.data[i][j].position.x += particle_grid.data[i][j].velocity.x*dt;
            particle_grid.data[i][j].position.y += particle_grid.data[i][j].velocity.y*dt;
            O++;
            SDL_SetRenderDrawColor(renderer, particle_grid.data[i][j].color[0], particle_grid.data[i][j].color[1], particle_grid.data[i][j].color[2], SDL_ALPHA_OPAQUE);
            if (particle_visible == 1) {
                drawCircle(particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y,pradius);
            }
        }
    }
}











void aff()
/*affichage des particules*/
{
    int running = 1, paused, mousex, mousey, partadded;
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
        partadded = 0;
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
                                    if (Pause.key.keysym.scancode == SDL_SCANCODE_S) {
                                        particle_visible*=-1;
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
                    if (Event.key.keysym.scancode == SDL_SCANCODE_S) {
                        particle_visible*=-1;
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
                        while (Event.type != SDL_MOUSEBUTTONUP && partadded<5) {
                            SDL_GetMouseState(&mousex, &mousey);
                            addparticles
                            (mousex,mousey);
                            partadded++;
                            SDL_PollEvent(&Event);
                        }
                        
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
                        coeff_visco = 1000*(mousex-width-width/30)/(width/6);
                        x_coeff_visco = mousex;
                    }else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*4 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*2.5) {
                        k = 50000*(mousex-width-width/30)/(width/6);
                        xk = mousex;
                    }/* else if (mousex>x_left && mousex<x_right && mousey> y_up && mousey<y_down) {
                         SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                         drawCircle(mousex,mousey,20);
                         SDL_RenderPresent(renderer);
                    }*/
                    break;

            }

        }
        dt = 10/FPS;
        update();
        end_time = SDL_GetTicks();
        fflush(stdout);

        elapsed_time = end_time - start_time;
        
        stat_aff(1000/elapsed_time);
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