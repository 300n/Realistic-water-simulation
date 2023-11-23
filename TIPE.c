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
#include <float.h>

#include "Get_stat.h"
 

char texte[100];
double dt;


double mu_eau = 0.0001;
double p0 = 0.5; // base density

Uint32 last_time;
Uint32 current_time;
double fq;
double fqq;

bool isPaused = false;



void particleonttop()
{
    int maxheight, temp = 0;
    for (int k = 0; k < numofseparation; k++) {
        maxheight = height;
        for (int i = 0; i<matlength; i++) {
            for (int j = 0; j<matwidth; j++) {
                if(particle_grid.data[i][j].position.x >= numofseparation*k && particle_grid.data[i][j].position.x <= numofseparation * (k+1) && particle_grid.data[i][j].position.y < maxheight) {
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
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    SDL_SetRenderDrawColor(renderer,0,0,255,SDL_ALPHA_OPAQUE);
    for (int k = 0; k < (numofseparation)-1; k++) {
        x1 = particle_grid.particle_on_top[k] / matwidth;
        x2 = particle_grid.particle_on_top[k+1] / matwidth;
        y1 = particle_grid.particle_on_top[k] % matlength;
        y2 = particle_grid.particle_on_top[k+1] % matlength;
        if (particle_grid.particle_on_top[k] > 0 && x1 > 0 && x1 < particle_grid.MATlength && x2 > 0 && x2 < particle_grid.MATlength && y1 > 0 && y1 < particle_grid.MATwidth 
        && y2 > 0 && y2 < particle_grid.MATwidth && particle_grid.data[x1][y1].position.x < particle_grid.data[x2][y2].position.x) {
            SDL_RenderDrawLine(renderer,particle_grid.data[x1][y1].position.x,particle_grid.data[x1][y1].position.y,particle_grid.data[x2][y2].position.x,particle_grid.data[x2][y2].position.y);
        }
    }
}

double Eularian_distance(int i, int j, int k, int n)
{
    return sqrt(pow((particle_grid.data[k][n].predicted_position.x - particle_grid.data[i][j].predicted_position.x),2) + pow((particle_grid.data[k][n].predicted_position.y - particle_grid.data[i][j].predicted_position.y),2));
}

Vect2D Position_to_Cell(Vect2D point)
{   
    Vect2D out;
    out.x = (int) (point.x / smoothing_radius);
    out.y = (int) (point.y / smoothing_radius);
    return out;
}

u_int64_t HashCell(int cellX, int cellY)
{
    u_int64_t a = (int)cellX * hashK1;
    u_int64_t b = (int)cellY * hashK2;
    return a + b;
}

int Get_Key_from_Hash(u_int64_t hash)
{
    // printf("hash = %d\n\n", hash);
    return hash % (int)(particle_grid.MATlength*particle_grid.MATwidth);
}

void swap(Couple* a, Couple* b) {
    Couple temp = *a;
    *a = *b;
    *b = temp;
}

int partition(Couple arr[], int low, int high) {
    int pivot = arr[high].value;
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j].value < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quicksort(Couple arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

void Update_Spatial_Lookup()
{
    for (int i = 0; i < particle_grid.MATlength*particle_grid.MATwidth ; i++) {
        Vect2D cell = Position_to_Cell(particle_grid.data[(int)(i/particle_grid.MATlength)][(int)(i%particle_grid.MATlength)].predicted_position);
        // printf("Part n %d &&   cell.x = %d && cell.y = %d\n", i, (int)cell.x, (int)cell.y);
        int cellKey = Get_Key_from_Hash(HashCell((int)cell.x,(int)cell.y));
        Spatial_Lookup[i].index = i;
        Spatial_Lookup[i].value = cellKey;
        start_indices[i] = INT_MAX;

    
    }

    // Sort cell by key
    quicksort(Spatial_Lookup,0,particle_grid.MATlength*particle_grid.MATwidth-1);

    for (int i = 0; i < particle_grid.MATlength*particle_grid.MATwidth; i++) {
        int key = Spatial_Lookup[i].value;
        int previouskey = INT_MAX;
        if (i!=0) {
            previouskey = Spatial_Lookup[i-1].value;
        }
        if (key != previouskey) {
            start_indices[key] = i;
        }
    
    }
}

void color_particle_concerned(Vect2D sample_point)
{
    isPaused = !isPaused;
    Vect2D SampleCell = Position_to_Cell(sample_point);
    
    // Parcours le 3x3 autour de la case d'origine
    for (int i = SampleCell.x - 1; i <= SampleCell.x + 1; i++) {
        for (int j = SampleCell.y - 1; j <= SampleCell.y + 1; j++) {

            SDL_SetRenderDrawColor(renderer,255,0,255,SDL_ALPHA_OPAQUE);
            dessinerCercle(i*smoothing_radius+smoothing_radius/2,j*smoothing_radius+smoothing_radius/2,smoothing_radius/2);

            int key = Get_Key_from_Hash(HashCell(i,j));


            int cellStartIndex = start_indices[key];
            for (int k = cellStartIndex; k<particle_grid.MATlength*particle_grid.MATwidth; k++) {
                if (Spatial_Lookup[k].value != key) { break; }
                int particle_Index = Spatial_Lookup[k].index;

                // double q = Eularian_distance(,,particle_Index/particle_grid.MATlength,particle_Index%particle_grid.MATlength);
                particle_grid.data[(int)particle_Index/particle_grid.MATlength][(int)particle_Index%particle_grid.MATlength].color[0] = 255;
                particle_grid.data[(int)particle_Index/particle_grid.MATlength][(int)particle_Index%particle_grid.MATlength].color[1] = 0;
                particle_grid.data[(int)particle_Index/particle_grid.MATlength][(int)particle_Index%particle_grid.MATlength].color[2] = 0;
            }
        }
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(100000);
}






double Smoothing_Kernel(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
	    double volume = (pi * pow(smoothing_radius, 4)) / 6;
	    return ((smoothing_radius - dst) * (smoothing_radius - dst)) / volume;
    }
}

double Smoothing_Kernel_Derivative(double dst)
{
	if (dst>=smoothing_radius) { return 0;}
    else {
		double volume = (pow(smoothing_radius, 4) * pi) / 12;
		return (dst - smoothing_radius) / volume;
	}
}

double Viscosity_smoothing_kernel(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
        double volume = (pi * pow(smoothing_radius, 8)) / 4;
        double v = smoothing_radius * smoothing_radius - dst * dst;
        return v * v * v / volume;
    }
}

double Near_density_Kernel( double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
        double volume = (pi * pow(smoothing_radius,5)) / 10;
        double v = smoothing_radius - dst;
        return v * v * v / volume;
    }
}

double Near_density_Kernel_Derivative(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
        double volume = (pi * pow(smoothing_radius,5)) / 30;
        double v = smoothing_radius - dst;
        return -v * v / volume;
    }
}


double2 Calculate_Density(int k, int n)
{
    double density = 0, near_density = 0, q;
    Vect2D point = Vect2D_cpy(particle_grid.data[k][n].predicted_position);
    Vect2D SampleCell = Position_to_Cell(point);

    // Parcours le 3x3 autour de la case d'origine
    for (int i = SampleCell.x - 1; i <= SampleCell.x + 1; i++) {
        for (int j = SampleCell.y - 1; j <= SampleCell.y + 1; j++) {
            int key = Get_Key_from_Hash(HashCell(i,j));

            if (key>=0 && key<particle_grid.MATlength*particle_grid.MATwidth) {
                int cellStartIndex = start_indices[key];
                
                for (int l = cellStartIndex; l<particle_grid.MATlength*particle_grid.MATwidth; l++) {
                    if (Spatial_Lookup[l].value != key) { break; }
                    O++;
                    int particle_Index = Spatial_Lookup[l].index;


                    q = Eularian_distance(k,n,particle_Index/particle_grid.MATlength,particle_Index%particle_grid.MATlength);

			        density += m * Smoothing_Kernel(q);
                    near_density += m * Near_density_Kernel(q);
                    

                }
            }
        }
    }
    double2 out = double2_cpy(density,near_density);
    return out;
}



double2 Convert_Density_To_Pressure(double density, double near_density)
{
	double density_error = 	density - target_density;
	double pressure = density_error * pressure_multiplier;
    double near_pressure = near_density * near_pressure_multiplier;
    double2 pressures = double2_cpy(pressure,near_pressure);
	return pressures;
}

double Calculate_Shared_Pressure(double densityA, double densityB)
{
	double pressureA = Convert_Density_To_Pressure(densityA,0
    ).first_value;
	double pressureB = Convert_Density_To_Pressure(densityB,0).first_value;
	return (pressureA + pressureB) / 2;
}


Vect2D Calculate_Pressure_Force(int k, int n)
{
    double q;
	Vect2D property_pressure_force = Vect2D_zero();

    Vect2D point = Vect2D_cpy(particle_grid.data[k][n].predicted_position);
    Vect2D SampleCell = Position_to_Cell(point);


    // Parcours le 3x3 autour de la case d'origine
    for (int i = SampleCell.x - 1; i <= SampleCell.x + 1; i++) {
        for (int j = SampleCell.y - 1; j <= SampleCell.y + 1; j++) {
            int key = Get_Key_from_Hash(HashCell(i,j));
            int cellStartIndex = start_indices[key];
            
            for (int l = cellStartIndex; l<particle_grid.MATlength*particle_grid.MATwidth; l++) {
                if (Spatial_Lookup[l].value != key) { break; }
                O++;
                int particle_Index = Spatial_Lookup[l].index;

                if (k!=particle_Index/particle_grid.MATlength || n !=particle_Index%particle_grid.MATlength) {

                
                    q = Eularian_distance(k,n,particle_Index/particle_grid.MATlength,particle_Index%particle_grid.MATlength);
                    Vect2D dir; 
                    if (q!=(double)0) {
				        dir.x = (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].predicted_position.x - particle_grid.data[k][n].predicted_position.x) / q; 
				        dir.y = (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].predicted_position.y - particle_grid.data[k][n].predicted_position.y) / q;
                    }
				    double slope = Smoothing_Kernel_Derivative(q);
				    double density = particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].density;
				    double shared_pressure = Calculate_Shared_Pressure(density,particle_grid.data[k][n].density);
                    if (density!=(double)0) {
				        property_pressure_force.x += (shared_pressure * dir.x * slope * m) / density ;
				        property_pressure_force.y += (shared_pressure * dir.y * slope * m) / density ;
                    }
                    
                }
            }
        }
    }
    return property_pressure_force;
}




Vect2D Calculate_Viscosity_Force(int k, int n)
{isPaused = !isPaused;
    Vect2D viscosityforce = Vect2D_zero();
    Vect2D position = Vect2D_cpy(particle_grid.data[k][n].position);
    Vect2D SampleCell = Position_to_Cell(position);
    // Parcours le 3x3 autour de la case d'origine
    for (int i = SampleCell.x - 1; i <= SampleCell.x + 1; i++) {
        for (int j = SampleCell.y - 1; j <= SampleCell.y + 1; j++) {
            int key = Get_Key_from_Hash(HashCell(i,j));
            int cellStartIndex = start_indices[key];

            for (int l = cellStartIndex; l<particle_grid.MATlength*particle_grid.MATwidth; l++) {
                if (Spatial_Lookup[l].value != key) { break; }
                O++;
                int particle_Index = Spatial_Lookup[l].index;

                if (k!=particle_Index/particle_grid.MATlength || n !=particle_Index%particle_grid.MATlength) {
                     double q = Eularian_distance(k,n,particle_Index/particle_grid.MATlength,particle_Index%particle_grid.MATlength);
                    if (q<=smoothing_radius) {
                        double influence = Viscosity_smoothing_kernel(q);
                        viscosityforce.x += (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity.x - particle_grid.data[k][n].velocity.x) * influence;
                        viscosityforce.y += (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity.y - particle_grid.data[k][n].velocity.y) * influence;

                    }
                }
            }
        }
    }
    return viscosityforce;
}

Vect2D Mouse_force(Vect2D inputPos, int k ,int n, double strength)
{
    Vect2D interaction_Force = Vect2D_zero();
    Vect2D offset;
    offset.x = inputPos.x - particle_grid.data[k][n].position.x;
    offset.y = inputPos.y - particle_grid.data[k][n].position.y;
    double q = sqrt(pow(offset.x,2)+pow(offset.y,2));

    if (q <= smoothing_radius*2) {
        Vect2D dir_to_input_force = Vect2D_zero();
        if (q >= FLT_EPSILON) {
            dir_to_input_force.x = offset.x / q;
            dir_to_input_force.y = offset.y / q;
        }
        double centreT = 1 - q / smoothing_radius*2;
        interaction_Force.x = (dir_to_input_force.x * strength ) * centreT;
        interaction_Force.y = (dir_to_input_force.y * strength) * centreT;
    }
    return interaction_Force;
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
            if (q<=smoothing_radius) {
                particle_grid.data[i][j].color[0] = 255;
                 particle_grid.data[i][j].color[2] = 0;
            }
            double influence = Smoothing_Kernel(q);
			density += m*influence;
			
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
				        double influence = Smoothing_Kernel(q);
				        density += m*influence;
			        
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

void affichage()
{
    clear_grid();
    draw_grid();    
    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            SDL_SetRenderDrawColor(renderer, particle_grid.data[i][j].color[0], particle_grid.data[i][j].color[1], particle_grid.data[i][j].color[2], SDL_ALPHA_OPAQUE);
            if (particle_visible == 1) {
                drawCircle(particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y,pradius);
                // sprintf(texte, "%d", i*particle_grid.MATlength+j);
                // surface_texte = TTF_RenderText_Blended(smallfont, texte, white);
                // texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
                // rect_texte.x = particle_grid.data[i][j].position.x;
                // rect_texte.y = particle_grid.data[i][j].position.y;
                // rect_texte.w = surface_texte->w;
                // rect_texte.h = surface_texte->h;    
                // SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
            }            
        }
    }
    if (particle_visible == -1) {
            particleonttop();
            align();
    }
}

void update()   
/*mise à jour des positions de chaque particule*/
{
    if (!isPaused) {

        // Set render color to blue
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);





	    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
            for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
	    		particle_grid.data[i][j].velocity.y += g * 20 / FPS;
                particle_grid.data[i][j].predicted_position.x = particle_grid.data[i][j].position.x + particle_grid.data[i][j].velocity.x* 1 / FPS;
                particle_grid.data[i][j].predicted_position.y = particle_grid.data[i][j].position.y + particle_grid.data[i][j].velocity.y* 1 / FPS;
	    	}
	    }


        Update_Spatial_Lookup();

        for (int i = 0 ; i < particle_grid.MATlength ; i++) {
            for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
                double2 densities = Calculate_Density(i,j);
                particle_grid.data[i][j].density = densities.first_value;
                particle_grid.data[i][j].near_density = densities.second_value;
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
        		particle_grid.data[i][j].velocity.x += pressure_acceleration.x * dt;;
        		particle_grid.data[i][j].velocity.y += pressure_acceleration.y * dt;; 
        	}
        }   

        for (int i = 0; i < particle_grid.MATlength ; i++) {
            for (int j = 0; j < particle_grid.MATwidth ; j++) {
                Vect2D viscosity_force = Calculate_Viscosity_Force(i,j);
                particle_grid.data[i][j].velocity.x += viscosity_force.x * viscosity_strength * dt;
        		particle_grid.data[i][j].velocity.y += viscosity_force.y * viscosity_strength *  dt; 
            }
        }


        // Update particle positions
	    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
            for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
	    		particle_grid.data[i][j].position.x += particle_grid.data[i][j].velocity.x*dt;
                particle_grid.data[i][j].position.y += particle_grid.data[i][j].velocity.y*dt;
                // printf("particle_grid.data[i][j].velocity.x = %lf && particle_grid.data[i][j].velocity.y = %lf\n", fabs(particle_grid.data[i][j].velocity.x), fabs(particle_grid.data[i][j].velocity.y));
	    	}
	    }

        particle_out_of_the_grid();

        double vitesse_max = INT_MIN, vitesse_min = INT_MAX;
        for (int i = 0 ; i < particle_grid.MATlength ; i++ ) {
            for (int j = 0 ; j < particle_grid.MATlength ; j++) {
                if (particle_grid.data[i][j].position.x < x_right - pradius && particle_grid.data[i][j].position.x > x_left + pradius && 
                particle_grid.data[i][j].position.y > y_up + pradius && particle_grid.data[i][j].position.y < y_down - pradius) {
                    if (fabs(particle_grid.data[i][j].velocity.x) + fabs(particle_grid.data[i][j].velocity.y) > vitesse_max) {
                        vitesse_max = fabs(particle_grid.data[i][j].velocity.x) + fabs(particle_grid.data[i][j].velocity.y);
                    } else if (fabs(particle_grid.data[i][j].velocity.x) + fabs(particle_grid.data[i][j].velocity.y) < vitesse_min) {
                        vitesse_min = fabs(particle_grid.data[i][j].velocity.x) + fabs(particle_grid.data[i][j].velocity.y);
                    }
                }
            }
        }


        
        for (int i = 0 ; i < particle_grid.MATlength ; i++ ) {
            for (int j = 0 ; j < particle_grid.MATlength ; j++) {
                double vitesse_normalisee = ((fabs(particle_grid.data[i][j].velocity.x) + fabs(particle_grid.data[i][j].velocity.y))) / vitesse_max;
                // printf("vitesse_normalisee = %lf && vitesse_max = %lf && vitess_part = %lf\n", vitesse_normalisee, vitesse_max,(fabs(particle_grid.data[i][j].velocity.x) + fabs(particle_grid.data[i][j].velocity.y)));
                
                if (particle_grid.data[i][j].position.x < x_right - pradius && particle_grid.data[i][j].position.x > x_left + pradius && 
                particle_grid.data[i][j].position.y > y_up + pradius && particle_grid.data[i][j].position.y < y_down - pradius) {
                    particle_grid.data[i][j].color[0] = vitesse_normalisee * 255;
                    particle_grid.data[i][j].color[1] = 0;
                    particle_grid.data[i][j].color[2] = (1 - vitesse_normalisee) * 255;
                } else {
                    particle_grid.data[i][j].color[0] = 0;
                    particle_grid.data[i][j].color[1] = 0;
                    particle_grid.data[i][j].color[2] = 255;
                }
            }
        }

        affichage();
    }
}

void rightshift() 
{
    int timeout = SDL_GetTicks64() + 1000; 
    while (SDL_GetTicks64() < timeout){
        update();
        SDL_RenderPresent(renderer);
        x_left += 10;
        x_right += 10;
    }
    while (x_right>width){
        update();
        SDL_RenderPresent(renderer);
        x_left -= 10;
        x_right -= 10;
    }
}

void upshift()
{
    int timeout = SDL_GetTicks64() + 1000;
    while (SDL_GetTicks64() < timeout){
        update();
        SDL_RenderPresent(renderer);
        y_up -= 10;
        y_down -= 10;
    }
    while (y_down<height){
        update();
        SDL_RenderPresent(renderer);
        y_up += 10;
        y_down += 10;
    }
}






void aff()
/*affichage des particules*/
{
    int running = 1, choice = 1, mousex, mousey;
    SDL_Event Event;
    initSDL();
    initTTF();
    Uint32 start_time, end_time, elapsed_time = FPS;
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
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                        isPaused = !isPaused;
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_F5) {
                        goto restart;
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                        dt = 1/(FPS*10)*5;
                        for (int i = 0; i<10; i++) {
                            update();
                        }
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_LEFT) {                                                //
                        dt = -1/(FPS/10)*5;
                        for (int i = 0; i<10; i++) {
                            update();
                        }
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_I ){
                        Colorflipped();
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_R) {
                        reset_const();
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_S) {
                        particle_visible*=-1;
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_C) {
                        choice *= -1;
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_G) {
                        rightshift();
                    } else if (Event.key.keysym.scancode == SDL_SCANCODE_H) {
                        upshift();
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
                    Vect2D Sample_point = Vect2D_zero();
                    SDL_GetMouseState(&mousex,&mousey);
                    Sample_point.x = mousex;
                    Sample_point.y = mousey;
                    if (choice == 1) {
                        if (Sample_point.x > 0 && Sample_point.x < width && Sample_point.y > 0 && Sample_point.y < height) {
                            while (Event.type != SDL_MOUSEBUTTONUP) {
                                if (Event.button.button == SDL_BUTTON_LEFT) {
                                    // printf("Left\n");
                                    if (mouse_force > 0) { mouse_force *= -1; }
                                } else if (Event.button.button == SDL_BUTTON_RIGHT) {
                                    // printf("Right\n");
                                    if (mouse_force < 0) { mouse_force *= -1; }
                                } 
                                if (Event.type == SDL_MOUSEMOTION) {SDL_GetMouseState(&mousex,&mousey);}


                                Sample_point.x = mousex;
                                Sample_point.y = mousey;
                                // printf("Sample_point.x = %lf && Sample_point.y = %lf\n", Sample_point.x, Sample_point.y);
                                Vect2D SampleCell = Vect2D_cpy(Position_to_Cell(Sample_point));
                                // Parcours le 3x3 autour de la case d'origine
                                for (int i = SampleCell.x - 2; i <= SampleCell.x + 2; i++) {
                                    for (int j = SampleCell.y - 2; j <= SampleCell.y + 2; j++) {
                                        int key = Get_Key_from_Hash(HashCell(i,j));
                                        int cellStartIndex = start_indices[key];
                                        for (int l = cellStartIndex; l<particle_grid.MATlength*particle_grid.MATwidth; l++) {
                                            if (Spatial_Lookup[l].value != key) { break; }
                                            O++;
                                            int particle_Index = Spatial_Lookup[l].index;
                                            // particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].color[0] = 255;
                                            particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity = Vect2D_add(particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity,  Mouse_force(Sample_point,particle_Index/particle_grid.MATlength,particle_Index%particle_grid.MATlength,mouse_force));
                                        }
                                    }
                                }
                            

                                update();
                                

                                SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
                                dessinerCercle(Sample_point.x,Sample_point.y,smoothing_radius*2);
                                // end_time = SDL_GetTicks();
                                // elapsed_time = end_time - start_time;
                                // stat_aff(1000/(elapsed_time+0.0001));
                                SDL_RenderPresent(renderer);
                                SDL_UpdateWindowSurface(window);
                                // printf("O = %d\n", O);
                                SDL_PollEvent(&Event);
                            }
                        } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40 && Sample_point.y<(height/4+height/20+height/400)+height/100) {
                            FPS = 120*(Sample_point.x-width-width/30)/(width/6);
                            xFPS = Sample_point.x;
                        } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+(height/4-height/10)/5 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/4-height/10)/5) {
                            smoothing_radius = 100  *(Sample_point.x-width-width/30)/(width/6);
                            xh = Sample_point.x;
                        } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*2 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)) {
                            m = 10*(Sample_point.x-width-width/30)/(width/6)+1;
                            xm = Sample_point.x;
                        } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*3 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*1.5) {
                            target_density = 0.01*(Sample_point.x-width-width/30)/(width/6);
                            x_tdens = Sample_point.x;
                        } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*4 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*2.5) {
                            pressure_multiplier = 100000*(Sample_point.x-width-width/30)/(width/6);
                            xk = Sample_point.x;
                        } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*5 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*3.5) {
                                viscosity_strength = 10*(Sample_point.x-width-width/30)/(width/6);
                                x_vs = Sample_point.x;
                        }


                    } else if (choice == -1) {
                        if (Event.button.button == SDL_BUTTON_LEFT) {
                            if (Sample_point.x > 0 && Sample_point.x < width && Sample_point.y > 0 && Sample_point.y < height) {
                                Examine_Density(Sample_point.x,Sample_point.y);    
                                while (Event.type != SDL_MOUSEBUTTONUP) {
                                    SDL_PollEvent(&Event);
                                }
                            }
                            if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40 && Sample_point.y<(height/4+height/20+height/400)+height/100) {
                                FPS = 120*(Sample_point.x-width-width/30)/(width/6);
                                xFPS = Sample_point.x;
                            } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+(height/4-height/10)/5 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/4-height/10)/5) {
                                smoothing_radius = 500  *(Sample_point.x-width-width/30)/(width/6);
                                xh = Sample_point.x;
                            } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*2 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)) {
                                m = 10*(Sample_point.x-width-width/30)/(width/6)+1;
                                xm = Sample_point.x;
                            } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*3 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*1.5) {
                                target_density = 0.01*(Sample_point.x-width-width/30)/(width/6);
                                x_tdens = Sample_point.x;
                            } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*4 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*2.5) {
                                pressure_multiplier = 200000*(Sample_point.x-width-width/30)/(width/6);
                                xk = Sample_point.x;
                            } else if (Sample_point.x>(width+widthstats/30*2) && Sample_point.x<(width+widthstats/30*2+widthstats/2) && Sample_point.y>(height/4+height/20)-height/40+((height/4-height/10)/5)*5 && Sample_point.y<(height/4+height/20+height/400)+height/100+(height/40+(height/4-height/10)/5)*3.5) {
                                viscosity_strength = 10*(Sample_point.x-width-width/30)/(width/6);
                                x_vs = Sample_point.x;
                            }
                        } else if (Event.button.button == SDL_BUTTON_RIGHT) {

                            if (Sample_point.x > 0 && Sample_point.x < width+500 && Sample_point.y > 0 && Sample_point.y < height) {
                                    SDL_GetMouseState(&mousex, &mousey);
                                    Sample_point.x = mousex;
                                    Sample_point.y = mousey;
                                    Visualize_Density();
                                while (Event.type != SDL_MOUSEBUTTONUP) {
                                    SDL_PollEvent(&Event);
                                }
                            }

                            // color_particle_concerned(Sample_point);
                            update();


                            SDL_RenderPresent(renderer);
                            while (Event.type != SDL_MOUSEBUTTONUP) {
                                    SDL_PollEvent(&Event);
                            }

                        }
                    }


                    break;
            }

        }

        dt = 0.049000;
        if (elapsed_time!=0) {
            dt = 0.049000;
        } 

        update();
        end_time = SDL_GetTicks();
        fflush(stdout);
        stat_aff(1000/((double)elapsed_time+0.0001));
        draw_scale();
        elapsed_time = end_time - start_time;


        if (!isPaused) {
            SDL_RenderPresent(renderer);
            SDL_UpdateWindowSurface(window);
        }
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