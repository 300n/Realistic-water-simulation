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
#include <stdint.h>
#include <limits.h>

#include "Get_stat.h"
 

char texte[100];
double dt;


double mu_eau = 0.0001;
double p0 = 0.5; // base density

Uint32 last_time;
Uint32 current_time;
double fq;
double fqq;







double Eularian_distance(int i, int j, int k, int n)
{
    return sqrt(pow((particle_grid.data[k][n].position.x - particle_grid.data[i][j].position.x),2) + pow((particle_grid.data[k][n].position.y - particle_grid.data[i][j].position.y),2));
}


double max(double x, double y)
{
	if (x >= y) {
		return x;
	} else {
		return y;
	}
}

Vect2D Position_to_Cell(Vect2D point)
{   
    Vect2D out;
    out.x = point.x / smoothing_radius;
    out.y = point.y / smoothing_radius;
    return out;
}

u_int64_t HashCell(int cellX, int cellY)
{
    u_int64_t a = (u_int64_t)cellX * 15823;
    u_int64_t b = (u_int64_t)cellY * 9737333;
    return a + b;
}

// u_int64_t Get_Key_from_Hash(u_int64_t hash)
// {
//     return hash % (int64_t)Spatial_lookup.length;
// }


double Smoothing_Kernel(double radius, double dst)
{
    if (dst>=radius) {
        return 0;
    } else {
	    double volume = pi * pow(radius, 4) / 6;
	    return (radius - dst) * (radius - dst) / volume;
    }
}

double Smoothing_Kernel_Derivative(double radius, double dst)
{
	if (dst>=radius) {
		return 0;
	} else {
		double scale = 12 / (pi * pow(radius, 4));
		return (dst - radius) * scale;
	}
}

double Calculate_Density(int k, int n)
{
	double density = 0, q;
	for (int i = 0; i<particle_grid.MATlength; i++) {
		for (int j = 0; j<particle_grid.MATwidth; j++) {
			q = Eularian_distance(k, n, i, j);
			if (q/smoothing_radius<1) {
				double influence = Smoothing_Kernel(smoothing_radius, q);
				density += m*influence;
			}
		}
	}
	return density;
}

void update_densities()
{
	for (int i = 0; i<particle_grid.MATlength; i++) {
		for (int j = 0; j<particle_grid.MATwidth; j++) {
			particle_grid.data[i][j].density = Calculate_Density(i,j);
		}
	}
}

double Convert_Density_To_Pressure(double density)
{
	double density_error = 	density - target_density;
	double pressure = density_error * pressure_multiplier;
	return pressure;
}

double Calculate_Shared_Pressure(double densityA, double densityB)
{
	double pressureA = Convert_Density_To_Pressure(densityA);
	double pressureB = Convert_Density_To_Pressure(densityB);
	return (pressureA + pressureB) / 2;
}

Vect2D Calculate_Pressure_Force(int k, int n)
{	
	double q;
	Vect2D property_pressure_force;
    property_pressure_force.x = 0;
    property_pressure_force.y = 0;
	for (int i = 0; i<particle_grid.MATlength; i++) {
		for (int j = 0; j<particle_grid.MATwidth; j++) {
			if (k != i || n != j) {
				q = Eularian_distance(k, n, i ,j);
				if (q/smoothing_radius<1) {
					Vect2D dir; 
                    if (q!=0){
					    dir.x = (particle_grid.data[i][j].position.x - particle_grid.data[k][n].position.x) / q; 
					    dir.y = (particle_grid.data[i][j].position.y - particle_grid.data[k][n].position.y) / q;
                        // printf("dir.x = %lf && dir.y = %lf \n", dir.x, dir.y);
                    }
					double slope = Smoothing_Kernel_Derivative(smoothing_radius,q);
                    // printf("slope = %lf\n", slope);
					double density = particle_grid.data[i][j].density;
					double shared_pressure = Calculate_Shared_Pressure(density,particle_grid.data[i][j].density);
                    if (density!=0) {
					    property_pressure_force.x += shared_pressure * dir.x * slope * m / density ;
					    property_pressure_force.y += shared_pressure * dir.y * slope * m / density ;
                    }
				}
			}
		}
	}
    // printf("property_pressure_force.x = %lf && property_pressure_force.y = %lf\n", property_pressure_force.x, property_pressure_force.y);
	return property_pressure_force;

}






void Examine_Density(int x, int y)
{
    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
    for (int i = 0; i < 5; i++) {
        dessinerCercle(x,y,smoothing_radius-i);
    }

    double density = 0, q;
	for (int i = 0; i<particle_grid.MATlength; i++) {
		for (int j = 0; j<particle_grid.MATwidth; j++) {
            particle_grid.data[i][j].color[0] = 0;
            q = sqrt(pow((particle_grid.data[i][j].position.x-x),2) + pow((particle_grid.data[i][j].position.y - y),2)) ;
			if (q/smoothing_radius<=1) {
                particle_grid.data[i][j].color[0] = 255;
				double influence = Smoothing_Kernel(smoothing_radius, q);
				density += m*influence;
			}
		}
	}
    sprintf(texte, "Density : %lf", density);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    rect_texte.x = x - rect_texte.w/2;
    if (y + smoothing_radius > height*0.9 ) {
        rect_texte.y = y - smoothing_radius*1.2;
    } else {
        rect_texte.y = y + smoothing_radius;
    }
    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            SDL_SetRenderDrawColor(renderer, particle_grid.data[i][j].color[0], particle_grid.data[i][j].color[1], particle_grid.data[i][j].color[2], SDL_ALPHA_OPAQUE);
            if (particle_visible == 1) {
                drawCircle(particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y,pradius);
            }
            particle_grid.data[i][j].color[0] = 0;
        }
    }

    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_RenderPresent(renderer);
}

void Visualize_Density()
{
    double density,q,delta;
    for (int w = 0; w<width; w+=10) {
        for (int h = 0; h<height; h+=10) {
            density = 0;
            for (int i = 0; i<particle_grid.MATlength; i++) {
		        for (int j = 0; j<particle_grid.MATwidth; j++) {
                    q = sqrt(pow((particle_grid.data[i][j].position.x-w),2) + pow((particle_grid.data[i][j].position.y - h),2)) ;
			            if (q/smoothing_radius<=1) {
				            double influence = Smoothing_Kernel(smoothing_radius, q);
				            density += m*influence;
			            }
		            }
            }
            delta = ((density-target_density));
            if (delta>0) {
                delta = fmod(delta*400000,255);
                SDL_SetRenderDrawColor(renderer,delta,delta/4,0,0.1);
            } else if (fabs(delta) < 0.00005) {
                delta = fmod(delta*1000000,255);
                SDL_SetRenderDrawColor(renderer,255,255,255,0.1);
            } else {
                delta = fmod(delta*2000000,255);
                SDL_SetRenderDrawColor(renderer,0,0,delta,0.1);
            }
            drawCircle(w,h,5);


        }
    }
    SDL_RenderPresent(renderer);
}

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

float threshold;

void update()   
/*mise à jour des positions de chaque particule*/
{
    // Clear grid and draw initial grid
    clear_grid();
    drawstatgrid();
    draw_grid();
    draw_scale();

    // Set render color to blue
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);


	for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
			// particle_grid.data[i][j].velocity.y += g*dt;
            // particle_grid.data[i][j].predicted_position.x = particle_grid.data[i][j].velocity.x*dt;
            // particle_grid.data[i][j].predicted_position.y = particle_grid.data[i][j].velocity.x*dt;
		}
	}

    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            particle_grid.data[i][j].density = Calculate_Density(i,j);
        }
    }

    // Update particle velocities
    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            // Calculate pressure force
    		Vect2D pressure_force = Calculate_Pressure_Force(i,j);

            // Calculate pressure acceleration
    		Vect2D pressure_acceleration;
    		pressure_acceleration.x = pressure_force.x / particle_grid.data[i][j].density;
    		pressure_acceleration.y = pressure_force.y / particle_grid.data[i][j].density;

            // Update particle velocity
    		particle_grid.data[i][j].velocity.x += pressure_acceleration.x * dt;
    		particle_grid.data[i][j].velocity.y += pressure_acceleration.y * dt; 
    	}
    }   

    // Update particle positions
	for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            // Update particle position
			particle_grid.data[i][j].position.x += particle_grid.data[i][j].velocity.x*dt;
            particle_grid.data[i][j].position.y += particle_grid.data[i][j].velocity.y*dt;
		}
	}

    particle_out_of_the_grid();
    // int max = INT_MIN;

    // for (int i = 0; i < particle_grid.MATlength; i++) {
    //     for (int j = 0; j < particle_grid.MATwidth; j++) {
    //         if (particle_grid.data[i][j].velocity.x + particle_grid.data[i][j].velocity.y > max) {
    //             max = particle_grid.data[i][j].velocity.x + particle_grid.data[i][j].velocity.y;
    //         }

    //     }
    // }

    // for (int i = 0; i < particle_grid.MATlength; i++) {
    //     for (int j = 0; j < particle_grid.MATwidth; j++) {
    //         threshold = (particle_grid.data[i][j].velocity.x + particle_grid.data[i][j].velocity.y)/max;
    //         particle_grid.data[i][j].color[0] = (int)255 * threshold;
    //         particle_grid.data[i][j].color[1] = 0;
    //         particle_grid.data[i][j].color[2] = (int)255 * (1.0f - threshold);
    //     }
    // }
    

    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
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
    int running = 1, paused, mousex, mousey;
    SDL_Event Event, Pause;
    initSDL();
    initTTF();
    Uint32 start_time, end_time, elapsed_time = FPS, pausedtime;
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
                    if (Event.button.button == SDL_BUTTON_LEFT) {
                        if (mousex > 0 && mousex < width && mousey > 0 && mousey < height) {
                                SDL_GetMouseState(&mousex, &mousey);
                                Examine_Density(mousex,mousey);
                            while (Event.type != SDL_MOUSEBUTTONUP) {
                                SDL_PollEvent(&Event);
                            }
                        }
                        if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40 && mousey<(height/4+height/20+height/400)+height/100) {
                            FPS = 120*(mousex-width-width/30)/(width/6);
                            xFPS = mousex;
                        } else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+(height/4-height/10)/5 && mousey<(height/4+height/20+height/400)+height/100+(height/4-height/10)/5) {
                            smoothing_radius = 120*(mousex-width-width/30)/(width/6);
                            xh = mousex;
                        } else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*2 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)) {
                            m = 10*(mousex-width-width/30)/(width/6)+1;
                            xm = mousex;
                        } else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*3 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*1.5) {
                            target_density = 0.001*(mousex-width-width/30)/(width/6);
                            x_coeff_visco = mousex;
                        }else if (mousex>(width+widthstats/30*2) && mousex<(width+widthstats/30*2+widthstats/2) && mousey>(height/4+height/20)-height/40+((height/4-height/10)/5)*4 && mousey<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*2.5) {
                            pressure_multiplier = 5000*(mousex-width-width/30)/(width/6);
                            xk = mousex;
                        }/* else if (mousex>x_left && mousex<x_right && mousey> y_up && mousey<y_down) {
                             SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                             drawCircle(mousex,mousey,20);
                             SDL_RenderPresent(renderer);
                        }*/
                    } else if (Event.button.button == SDL_BUTTON_RIGHT) {
                        if (mousex > 0 && mousex < width+500 && mousey > 0 && mousey < height) {
                                SDL_GetMouseState(&mousex, &mousey);
                                Visualize_Density();
                            while (Event.type != SDL_MOUSEBUTTONUP) {
                                SDL_PollEvent(&Event);
                            }
                        }
                    }
                    break;

            }

        }
        dt = 10/(10000/(double)elapsed_time);
        update();
        end_time = SDL_GetTicks();
        fflush(stdout);

        elapsed_time = end_time - start_time;
        
        // stat_aff(1000/elapsed_time);
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
