#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include <limits.h>

//GET CPU Data
#include <unistd.h>
#include <dirent.h> 

//Multi-threading
#include <pthread.h>
#include <sched.h>



#include "Get_stat.h"
 

char texte[100];
double dt;

int objet = 0;
double mu_eau = 0.0001;
double p0 = 0.5; // base density

Uint32 last_time;
Uint32 current_time;
double fq;
double fqq;

bool isPaused = false;
bool Processing = false;
bool inputing = false;
bool full_screen = false;
bool help = false;

void particleonttop(void)
{
    int maxheight, temp = 0;
    for (int k = 0; k <= numofseparation; k++) {
        maxheight = height;
        int found_particle = -1;
        double segmentwidht = (width+widthstats) / (numofseparation-1);

        for (int i = 0; i < matlength; i++) {
            for (int j = 0; j < matwidth; j++) {
                if(particle_grid.data[i][j].position.x >= segmentwidht * k - 0 && 
                   particle_grid.data[i][j].position.x <= segmentwidht * (k+1) + 0 && 
                   particle_grid.data[i][j].position.y < maxheight) {
                    maxheight = particle_grid.data[i][j].position.y;
                    found_particle = i*matlength+j;
                }   
            }
        }
        
        if (found_particle == -1 && temp > 0) {
            found_particle = particle_grid.particle_on_top[temp - 1];
        } else if (found_particle == -1) {
            found_particle = 0;
        }
        
        particle_grid.particle_on_top[temp] = found_particle;
        temp++;
    }
}


void fill_water_area_gradient(void)
{
    for (int x = x_left; x < x_right; x++) {
        int surface_y = y_down;
        
        // Trouver la hauteur de surface 
        for (int k = 0; k < numofseparation - 1; k++) {
            int x1 = particle_grid.particle_on_top[k] / matwidth;
            int y1 = particle_grid.particle_on_top[k] % matlength;
            int x2 = particle_grid.particle_on_top[k+1] / matwidth;
            int y2 = particle_grid.particle_on_top[k+1] % matlength;
            
            if (x1 >= 0 && x1 < particle_grid.MATlength && 
                x2 >= 0 && x2 < particle_grid.MATlength && 
                y1 >= 0 && y1 < particle_grid.MATwidth && 
                y2 >= 0 && y2 < particle_grid.MATwidth) {
                
                int pos_x1 = particle_grid.data[x1][y1].position.x;
                int pos_y1 = particle_grid.data[x1][y1].position.y;
                int pos_x2 = particle_grid.data[x2][y2].position.x;
                int pos_y2 = particle_grid.data[x2][y2].position.y;
                
                if (x >= pos_x1 && x <= pos_x2 && pos_x2 > pos_x1) {
                    double t = (double)(x - pos_x1) / (pos_x2 - pos_x1);
                    surface_y = (int)(pos_y1 + t * (pos_y2 - pos_y1));
                    break;
                }
            }
        }
        
        // Tracer avec dégradé de couleur selon la profondeur
        for (int y = surface_y; y < y_down; y++) {
            double depth_ratio = (double)(y - surface_y) / (y_down - surface_y);
            int blue = 200 - (int)(depth_ratio * 80); // Plus foncé en profondeur
            int green = 100 - (int)(depth_ratio * 60);
            SDL_SetRenderDrawColor(renderer, 0, green, blue, 180);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
}

void align(void)
{
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, SDL_ALPHA_OPAQUE);
    
    if (numofseparation < 4) {
        align(); // Pas assez de points pour une spline
        return;
    }
    
    for (int k = 1; k < (numofseparation)-2; k++) {
        int x0 = particle_grid.particle_on_top[k-1] / matwidth;
        int y0 = particle_grid.particle_on_top[k-1] % matlength;
        int x1 = particle_grid.particle_on_top[k] / matwidth;
        int y1 = particle_grid.particle_on_top[k] % matlength;
        int x2 = particle_grid.particle_on_top[k+1] / matwidth;
        int y2 = particle_grid.particle_on_top[k+1] % matlength;
        int x3 = particle_grid.particle_on_top[k+2] / matwidth;
        int y3 = particle_grid.particle_on_top[k+2] % matlength;
        
        if (x0 >= 0 && x0 < particle_grid.MATlength && 
            x1 >= 0 && x1 < particle_grid.MATlength && 
            x2 >= 0 && x2 < particle_grid.MATlength &&
            x3 >= 0 && x3 < particle_grid.MATlength &&
            y0 >= 0 && y0 < particle_grid.MATwidth && 
            y1 >= 0 && y1 < particle_grid.MATwidth && 
            y2 >= 0 && y2 < particle_grid.MATwidth &&
            y3 >= 0 && y3 < particle_grid.MATwidth) {
            
            double p0_x = particle_grid.data[x0][y0].position.x;
            double p0_y = particle_grid.data[x0][y0].position.y;
            double p1_x = particle_grid.data[x1][y1].position.x;
            double p1_y = particle_grid.data[x1][y1].position.y;
            double p2_x = particle_grid.data[x2][y2].position.x;
            double p2_y = particle_grid.data[x2][y2].position.y;
            double p3_x = particle_grid.data[x3][y3].position.x;
            double p3_y = particle_grid.data[x3][y3].position.y;
            
            int segments = 15;
            double prev_x = p1_x;
            double prev_y = p1_y;
            double tension = 0.5; // Ajuster entre 0 et 1
            
            for (int i = 1; i <= segments; i++) {
                double t = (double)i / segments;
                double t2 = t * t;
                double t3 = t2 * t;
                
                // Matrice de Catmull-Rom
                double cx = (-tension * p0_x + (2 - tension) * p1_x + (tension - 2) * p2_x + tension * p3_x) * t3 +
                           (2 * tension * p0_x + (tension - 3) * p1_x + (3 - 2 * tension) * p2_x - tension * p3_x) * t2 +
                           (-tension * p0_x + tension * p2_x) * t + p1_x;
                           
                double cy = (-tension * p0_y + (2 - tension) * p1_y + (tension - 2) * p2_y + tension * p3_y) * t3 +
                           (2 * tension * p0_y + (tension - 3) * p1_y + (3 - 2 * tension) * p2_y - tension * p3_y) * t2 +
                           (-tension * p0_y + tension * p2_y) * t + p1_y;
                
                SDL_RenderDrawLine(renderer, (int)prev_x, (int)prev_y, (int)cx, (int)cy);
                prev_x = cx;
                prev_y = cy;
            }
        }
    }
}

double Eularian_distance(int i, int j, int k, int n)
{
    double dx = particle_grid.data[k][n].predicted_position.x - particle_grid.data[i][j].predicted_position.x;
    double dy = particle_grid.data[k][n].predicted_position.y - particle_grid.data[i][j].predicted_position.y;
    return sqrt(dx * dx + dy * dy); 
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

// Fonction qui met à jour le "Spatial Lookup" un tableau recensant chaque particule est la cellule ou elle est située
// Ce tableau est ensuite trié par ordre de cellule afin d'obtenir les particules qui sont situées dans la même cellule pour optimiser les calculs de rayon Kernel
// Pas d'amélioration prévue ici
void Update_Spatial_Lookup(void)
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

// Colorie les particules se situant de le rayon Kernel du point examiné
// Parcours 3x3 autour de la cellule concernée afin d'obtenir les clés de chaque cellule examinée
// Parcours du tableau Spatial_Lookup à partir des indices de chaque cellule afin de determiner les particules concernées et pouvoir les colorier 
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
                int pi = (int)particle_Index / particle_grid.MATlength;
                int pj = (int)particle_Index % particle_grid.MATlength;

                particle_grid.data[pi][pj].color[0] = 255;
                particle_grid.data[pi][pj].color[1] = 0;
                particle_grid.data[pi][pj].color[2] = 0;
            }
        }
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(100000);
}


double smoothing_radius_pow4;
double smoothing_kernel_volume;
double viscosity_kernel_volume;
double near_density_kernel_volume;

void init_kernel(void) {
    smoothing_radius_pow4 = pow(smoothing_radius, 4);
    smoothing_kernel_volume = smoothing_radius_pow4 * pi / 6;
    viscosity_kernel_volume = (smoothing_radius_pow4 * smoothing_radius_pow4 * pi) / 4;
    near_density_kernel_volume = (smoothing_radius_pow4*smoothing_radius * pi) / 10;
}

// Rayon Kernel => permet de négliger les effets entre deux particules trop éloignées 
// https://fr.wikipedia.org/wiki/Astuce_du_noyau && https://en.wikipedia.org/wiki/Kernel_method

// Premier Kernel permettant le calcul de densité 
// Utilisation de la fonction f(x) = (rayon_kernel - x)² 
// Si la distance entre les deux particules est supérieure au rayon kernel alors la valeur de la fonction kernel est égale à 0
// Calcul du volume en double intégrant entre 0 et le rayon kernel et 0 et 2pi 
// (Integrate[Integrate[\(40)s-x\(41)²x,{θ,0,2π}],{x,0,s}]) avec s notre rayon kernel
// https://www.wolframalpha.com/input?i2d=true&i=Integrate%5BIntegrate%5B%5C%2840%29s-x%5C%2841%29%C2%B2x%2C%7B%CE%B8%2C0%2C2%CF%80%7D%5D%2C%7Bx%2C0%2Cs%7D%5D
// On obtient alors volume = (pi*rayon_kernel^4) / 6 
// Ensemble des Kernels sont basés sur les calculs ici https://matthias-research.github.io/pages/publications/sca03.pdf
double Smoothing_Kernel(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
	    return ((smoothing_radius - dst) * (smoothing_radius - dst)) / smoothing_kernel_volume;
    }
}

double Smoothing_Kernel_Derivative(double dst)
{
	if (dst>=smoothing_radius) { return 0;}
    else {
		return (dst - smoothing_radius) / (smoothing_kernel_volume/2);
	}
}

// Second Kernel permettant de calculer la viscosité
// Utilisation de la fonction f(x) = (rayon_kernel² - x²)^3 
// Si la distance entre les deux particu //printf("Touche pressée : %s\n", SDL_GetKeyName(Event.key.keysym.sym));les est supérieure au rayon kernel alors la valeur de la fonction kernel est égale à 0
// Calcul du volume en double intégrant entre 0 et le rayon kernel et 0 et 2pi 
// Integrate[Integrate[Power[\(40)s²-x²\(41),3]x,{θ,0,2π}],{x,0,s}]
// https://www.wolframalpha.com/input?i2d=true&i=Integrate%5BIntegrate%5BPower%5B%5C%2840%29s%C2%B2-x%C2%B2%5C%2841%29%2C3%5Dx%2C%7B%CE%B8%2C0%2C2%CF%80%7D%5D%2C%7Bx%2C0%2Cs%7D%5D
// On obtient alors volume = (pi * rayon_kernel^8) / 4
double Viscosity_smoothing_kernel(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
        double v = smoothing_radius * smoothing_radius - dst * dst;
        return v * v * v / viscosity_kernel_volume;
    }
}

// Troisième Kernel permettant de calculer la densité pour deux particules extrêmement proches
// Utilisation de la fonction f(x) = (rayon_kernel - x)^3
// Si la distance entre les deux particules est supérieure au rayon kernel alors la valeur de la fonction kernel est égale à 0
// Calcul du volume en double intégrant entre 0 et le rayon kernel et 0 et 2pi 
// Integrate[Integrate[Power[\(40)s-x\(41),3]x,{θ,0,2π}],{x,0,s}]
// https://www.wolframalpha.com/input?i2d=true&i=Integrate%5BIntegrate%5BPower%5B%5C%2840%29s-x%5C%2841%29%2C3%5Dx%2C%7B%CE%B8%2C0%2C2%CF%80%7D%5D%2C%7Bx%2C0%2Cs%7D%5D
// On obtient alors volume = (pi * rayon_kernel^5) / 10
double Near_density_Kernel(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
        double v = smoothing_radius - dst;
        return v * v * v / near_density_kernel_volume;
    }
}

double Near_density_Kernel_Derivative(double dst)
{
    if (dst>=smoothing_radius) { return 0; }
    else {
        double v = smoothing_radius - dst;
        return -v * v / near_density_kernel_volume / 3;
    }
}

// Calcul de la densité en utilisant le parcours 3x3 vu précédement 
// en ajoutant la masse de chaque particule x l'influence de cette particule sur la particule concernée
// Problèmes -> Calcul des densités à l'extérieur des limites -> volume souvent trop élevé
// Conséquences -> Les particules sont aglutinées sur les bords 
double2 Calculate_Density(int k, int n)
{
    double density = 0, near_density = 0;
    Vect2D point = particle_grid.data[k][n].predicted_position;
    Vect2D SampleCell = Position_to_Cell(point);
    
    int total_particles = particle_grid.MATlength * particle_grid.MATwidth;
    double smoothing_radius_sq = smoothing_radius * smoothing_radius; 
    
    int min_i = SampleCell.x - 1;
    int max_i = SampleCell.x + 1;
    int min_j = SampleCell.y - 1;
    int max_j = SampleCell.y + 1;
    
    for (int i = min_i; i <= max_i; i++) {
        for (int j = min_j; j <= max_j; j++) {
            int key = Get_Key_from_Hash(HashCell(i, j));
            
            if (key < 0 || key >= total_particles) continue;
            
            int cellStartIndex = start_indices[key];
            if (cellStartIndex == INT_MAX) continue; // Cellule vide
            
            for (int l = cellStartIndex; l < total_particles; l++) {
                if (Spatial_Lookup[l].value != key) break;
                
                int particle_Index = Spatial_Lookup[l].index;
                int pi = particle_Index / particle_grid.MATlength;
                int pj = particle_Index % particle_grid.MATlength;
                
                double dx = particle_grid.data[pi][pj].predicted_position.x - point.x;
                double dy = particle_grid.data[pi][pj].predicted_position.y - point.y;
                double dist_sq = dx * dx + dy * dy;
                
                if (dist_sq < smoothing_radius_sq) {
                    double q = sqrt(dist_sq);
                    double nv_m = particle_grid.data[k][n].masse * 3;
                    double mass = m + nv_m;
                    
                    density += mass * Smoothing_Kernel(q);
                    near_density += mass * Near_density_Kernel(q);
                }
            }
        }
    }
    
    return double2_cpy(density, near_density);
}



// Convertit une densité en une force de pression en utilisant un objectif de densité qui est "fixé" au début du programme
// Tant que cet objectif n'est pas atteint dans une zone les particules se déplacent de façon à l'atteindre 
// La multiplication par un pressure_multiplier permet d'atteindre cet objectif de pression plus ou moins rapidement et ainsi de fluidifier la simulation 
double2 Convert_Density_To_Pressure(double density, double near_density)
{
	double density_error = 	density - target_density;
	double pressure = density_error * pressure_multiplier;
    double near_pressure = near_density * near_pressure_multiplier;
    double2 pressures = double2_cpy(pressure,near_pressure);
	return pressures;
}

// Calcul la force de pression exercée par la particule examiné sur l'autre 
// => d'après Newton chaque force admet une force réciproque (https://fr.wikipedia.org/wiki/Lois_du_mouvement_de_Newton)
double Calculate_Shared_Pressure(double densityA, double densityB)
{
	double pressureA = Convert_Density_To_Pressure(densityA,0
    ).first_value;
	double pressureB = Convert_Density_To_Pressure(densityB,0).first_value;
	return (pressureA + pressureB) / 2;
}

// Pour chaque particule on regarde ses voisines se situant dans le rayon Kernel
// On utilise une fonction Kernel plus pentue afin d'augmenter l'effet des particules proches 
// (et donc d'éviter les contacts qui ne peuvent pas exister dans un modèle parfait)
// 
Vect2D Calculate_Pressure_Force(int k, int n)
{
    Vect2D property_pressure_force = {0, 0}; 
    Vect2D point = particle_grid.data[k][n].predicted_position;
    Vect2D SampleCell = Position_to_Cell(point);
    
    double my_density = particle_grid.data[k][n].density;
    int total_particles = particle_grid.MATlength * particle_grid.MATwidth;
    
    for (int i = SampleCell.x - 1; i <= SampleCell.x + 1; i++) {
        for (int j = SampleCell.y - 1; j <= SampleCell.y + 1; j++) {
            int key = Get_Key_from_Hash(HashCell(i, j));
            if (key < 0 || key >= total_particles) continue;
            
            int cellStartIndex = start_indices[key];
            if (cellStartIndex == INT_MAX) continue;
            
            for (int l = cellStartIndex; l < total_particles; l++) {
                if (Spatial_Lookup[l].value != key) break;
                
                int particle_Index = Spatial_Lookup[l].index;
                int pi = particle_Index / particle_grid.MATlength;
                int pj = particle_Index % particle_grid.MATlength;
                if (k == pi && n == pj) continue;
                
                double dx = particle_grid.data[pi][pj].predicted_position.x - point.x;
                double dy = particle_grid.data[pi][pj].predicted_position.y - point.y;
                double dist_sq = dx * dx + dy * dy;
                
                if (dist_sq > 0 && dist_sq < smoothing_radius * smoothing_radius) {
                    double q = sqrt(dist_sq);
                    double dir_x = dx / q;
                    double dir_y = dy / q;
                    
                    double slope = Smoothing_Kernel_Derivative(q);
                    double density = particle_grid.data[pi][pj].density;
                    
                    if (density > 0) {
                        double shared_pressure = Calculate_Shared_Pressure(density, my_density);
                        double force_factor = (shared_pressure * slope * m) / density;
                        
                        property_pressure_force.x += force_factor * dir_x;
                        property_pressure_force.y += force_factor * dir_y;
                    }
                }
            }
        }
    }
    return property_pressure_force;
}


// Utilisation d'une fonction kernel moins pentue -> pas de besoin d'augmenter la viscosité si deux particules sont trop proches
Vect2D Calculate_Viscosity_Force(int k, int n)
{
    Vect2D viscosityforce = {0, 0};
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
                int pi = (int)particle_Index / particle_grid.MATlength;
                int pj = (int)particle_Index % particle_grid.MATlength;
                if (k!=pi || n !=pj) {
                     double q = Eularian_distance(k,n,pi,pj);
                    if (q<=smoothing_radius) {
                        double influence = Viscosity_smoothing_kernel(q);
                        viscosityforce.x += (particle_grid.data[pi][pj].velocity.x - particle_grid.data[k][n].velocity.x) * influence;
                        viscosityforce.y += (particle_grid.data[pi][pj].velocity.y - particle_grid.data[k][n].velocity.y) * influence;

                    }
                }
            }
        }
    }
    return viscosityforce;
}

// Applique une force répulsive ou attractive au praticule selon le click utilisé
Vect2D Mouse_force(Vect2D inputPos, int k, int n, double strength)
{
    Vect2D interaction_Force = Vect2D_zero();
    Vect2D offset;
    offset.x = inputPos.x - particle_grid.data[k][n].position.x;
    offset.y = inputPos.y - particle_grid.data[k][n].position.y;
    double q = sqrt(pow(offset.x, 2) + pow(offset.y, 2));

    if (q <= smoothing_radius * 2) {
        Vect2D dir_to_input_force = Vect2D_zero();
        if (q >= FLT_EPSILON) {
            dir_to_input_force.x = offset.x / q;
            dir_to_input_force.y = offset.y / q;
        }
        
        // Influence basée sur la distance
        double influence = 1.0 - (q / (smoothing_radius * 2));
        
        // Force de ressort : proportionnelle à la distance
        double spring_force = q * strength * influence / 8;
        
        // Force d'amortissement : oppose la vitesse actuelle
        double damping = 0.25; // Ajustez entre 0.5 et 0.95
        Vect2D velocity = particle_grid.data[k][n].velocity;
        
        // Force totale = ressort - amortissement
        interaction_Force.x = (dir_to_input_force.x * spring_force) - (velocity.x * damping * influence);
        interaction_Force.y = (dir_to_input_force.y * spring_force) - (velocity.y * damping * influence);
    }
    return interaction_Force;
}




// Examine la densité en un point de la simulation 
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
			density += (m-particle_grid.data[i][j].masse)*influence;
			
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

// Visualise les endroits de la simulation où la densité n'est pas assez/trop grande par rapport à la densité visée
// Si cette densité est trop petite alors la zone apparait en couleur froide (bleu) et si c'est l'inverse il s'agit d'une couleur chaude (orange)
// Pour les zones proches de la densité visée elles apparaissent en blanc
void Visualize_Density(void)
{
    double density,q,delta;
    for (int w = 0; w<width; w+=10) {
        for (int h = 0; h<height; h+=10) {
            density = 0;
            for (int i = 0; i<particle_grid.MATlength; i++) {
		        for (int j = 0; j<particle_grid.MATwidth; j++) {
                        q = sqrt(pow((particle_grid.data[i][j].position.x-w),2) + pow((particle_grid.data[i][j].position.y - h),2)) ;
				        double influence = Smoothing_Kernel(q);
				        density += (m+particle_grid.data[i][j].masse)*influence;
			        
		            }
            }
            delta = ((density-target_density));
            if (delta>0) {
                delta = fmod(delta*400000,255);
                SDL_SetRenderDrawColor(renderer,delta,delta/4,0,25);
            } else if (fabs(delta) < 0.00005) {
                delta = fmod(delta*1000000,255);
                SDL_SetRenderDrawColor(renderer,255,255,255,25);
            } else {
                delta = fmod(delta*2000000,255);
                SDL_SetRenderDrawColor(renderer,0,0,delta,25);
            }
            drawCircle(w,h,5);

        }
    }
    SDL_RenderPresent(renderer);
}

// Détecte une sortie de l'écran pour une partciule
void particle_out_of_the_grid(void)
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

// Affiche l'espace de simulation 
void affichage(void)
{
    clear_grid();
    draw_grid();  
    if (help) {
        draw_help();
    }  
    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
        for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
            SDL_SetRenderDrawColor(renderer, particle_grid.data[i][j].color[0], particle_grid.data[i][j].color[1], particle_grid.data[i][j].color[2], SDL_ALPHA_OPAQUE);
            if (particle_visible == 1) {
                drawCircle(particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y,pradius+particle_grid.data[i][j].rayon);
            }            
        }
    }
    if (particle_visible == -1) {
        particleonttop();
        align(); 
        fill_water_area_gradient(); 
    }
}

/*mise à jour des positions de chaque particule*/
void update(void)   
{
    if (!isPaused
    ) {


        Processing = true;
        // Set render color to blue
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);


        double dt_over_fps = 1/FPS;
        double gravity_step = g * 20.0 / FPS;

	    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
            for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
	    		particle_grid.data[i][j].velocity.y += gravity_step;
                particle_grid.data[i][j].predicted_position.x = particle_grid.data[i][j].position.x + particle_grid.data[i][j].velocity.x* dt_over_fps;
                particle_grid.data[i][j].predicted_position.y = particle_grid.data[i][j].position.y + particle_grid.data[i][j].velocity.y* dt_over_fps;
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
        
        double vitesse_max = 200, vitesse_min = INT_MAX;
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


        // Colore chaque particule selon sa vitesse relative par rapport à la particule la plus rapide de la simulation
        // Plus une particule est rapide plus elle apparait dans une couleur chaude 
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
        Processing = false;

        affichage();

    }
}


// Décale l'espace de simulation vers la droite pendant un instant avant de revenir à l'état initial
void rightshift(void) 
{
    int timeout = SDL_GetTicks64() + 1000; 
    while (SDL_GetTicks64() < timeout){
        update();
        SDL_RenderPresent(renderer);
        x_left += 10;
        x_right += 10;
    }
    while (x_right>width+widthstats){
        update();
        SDL_RenderPresent(renderer);
        x_left -= 10;
        x_right -= 10;
    }
}

// Décale l'espace de simiulation vers le haut pendant un instant avant de revenir à l'état initial 
void upshift(void)
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



int running = 1, choice = 1;


void* aff(void* arg)
{
    SDL_Event Event;
    int mousex, mousey;
    Uint32 start_time, end_time;
    double target_frame_time = 1000.0 / 60.0;  // 60 FPS
    
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
                     } else if (Event.key.keysym.scancode == SDL_SCANCODE_LEFT) {                                                
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
                     } else if (Event.key.keysym.scancode  == SDL_SCANCODE_H) {
                         help = !help;
                     } else if (Event.key.keysym.scancode == SDL_SCANCODE_U) {
                         upshift();                  
                     } else if (Event.key.keysym.scancode == SDL_SCANCODE_S) {
                         stat_visual_status = !stat_visual_status;
                     } else if (Event.key.keysym.scancode == SDL_SCANCODE_F11) {
                        if (!full_screen) {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                            x_right = 1920;
                            y_down = 1080;
                        } else {
                            SDL_SetWindowFullscreen(window, !SDL_WINDOW_FULLSCREEN);
                            clear_grid();
                            x_right = width+widthstats;
                            y_down = height;
                        }
                        full_screen = !full_screen;
                    }

                     break;
                case SDL_MOUSEBUTTONDOWN: {
                     Vect2D Sample_point = Vect2D_zero();
                     SDL_GetMouseState(&mousex,&mousey);
                     Sample_point.x = mousex;
                     Sample_point.y = mousey;
                     if (choice == 1) {
                         if (Sample_point.x > 0 && Sample_point.x < width + widthstats && Sample_point.y > 0 && Sample_point.y < height) {
                             while (Event.type != SDL_MOUSEBUTTONUP) {
                                start_time = SDL_GetTicks();
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
                                 end_time = SDL_GetTicks();
                                 double elapsed_time = (double)(end_time - start_time);
                                 stat_aff(1000.0 / (elapsed_time + 0.0001));
                                 draw_scale();
                             
                                 SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
                                 dessinerCercle(Sample_point.x,Sample_point.y,smoothing_radius*2);
                                 // end_time = SDL_GetTicks();
                                 // elapsed_time = end_time - start_time;
                                 // stat_aff(1000/(elapsed_time+0.0001));
                                 SDL_RenderPresent(renderer);
                                 SDL_UpdateWindowSurface(window);

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
         }

        dt = 0.016;


        update();
        end_time = SDL_GetTicks();
        double elapsed_time = (double)(end_time - start_time);
        
        if (stat_visual_status) {
            stat_aff(1000.0 / (elapsed_time + 0.0001));
            draw_scale();
        }
        
        if (!isPaused) {
            SDL_RenderPresent(renderer);
        }
        
        // Délai adaptatif pour maintenir le framerate
        if (elapsed_time < target_frame_time) {
            SDL_Delay((Uint32)(target_frame_time - elapsed_time));
        }
    }

    SDL_DestroyTexture(texture_texte);
    SDL_FreeSurface(surface_texte);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return NULL;
}





int main(void)
{
    initSDL();
    initTTF();
    init_kernel();
    aff(NULL);



    return 0;
}
