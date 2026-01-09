#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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

// Configuration du multi-threading
#define NUM_THREADS 16

typedef struct {
    int start_idx;
    int end_idx;
    int thread_id;
} ThreadRange;

pthread_t worker_threads[NUM_THREADS];
ThreadRange thread_ranges[NUM_THREADS];
 

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

double compute_surface_height_at_x(double target_x, double search_radius)
{
    double weighted_sum = 0.0;
    double weight_total = 0.0;
    double min_y = y_down; 

    for (int i = 0; i < particle_grid.MATlength; i++) {
        for (int j = 0; j < particle_grid.MATwidth; j++) {
            double px = particle_grid.data[i][j].position.x;
            double py = particle_grid.data[i][j].position.y;

            if (py < y_up || py > y_down || px < x_left || px > x_right) continue;

            double dx = px - target_x;

            if (fabs(dx) <= search_radius) {
                double normalized_dist = dx / search_radius;
                double weight = exp(-2.0 * normalized_dist * normalized_dist);

                double height_factor = 1.0 - (py - y_up) / (double)(y_down - y_up);
                weight *= (1.0 + height_factor * 2.0);

                weighted_sum += py * weight;
                weight_total += weight;

                if (py < min_y) min_y = py;
            }
        }
    }

    if (weight_total > 0.01) {
        double avg_height = weighted_sum / weight_total;
        return min_y * 0.7 + avg_height * 0.3;
    }

    return y_down;  
}

void particleonttop(void)
{
    double segment_width = (double)(width+widthstats) / (numofseparation - 1);
    double search_radius = segment_width * 1.5;  

    for (int k = 0; k < numofseparation; k++) {
        double target_x = x_left + k * segment_width;

        gradient_control_x[k] = target_x;

        gradient_control_y[k] = compute_surface_height_at_x(target_x, search_radius);
    }
}

double catmull_rom_interpolate(double p0, double p1, double p2, double p3, double t)
{
    double t2 = t * t;
    double t3 = t2 * t;
    
    return 0.5 * ((2.0 * p1) +
                  (-p0 + p2) * t +
                  (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
                  (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

typedef struct {
    int y_start;
    int y_end;
} WaterSegment;

#define MAX_SEGMENTS_PER_COLUMN 8
#define PARTICLE_RADIUS 8          // Rayon d'influence d'une particule (pixels) - petit pour contours précis
#define GAP_MERGE_THRESHOLD 6      // Fusionner les gaps plus petits que cette valeur

static uint8_t* water_mask = NULL;
static int mask_width = 0;
static int mask_height = 0;

void fill_water_area_gradient(void)
{
    int sim_width = x_right - x_left;
    int sim_height = y_down - y_up;

    if (water_mask == NULL || mask_width != sim_width || mask_height != sim_height) {
        if (water_mask) free(water_mask);
        mask_width = sim_width;
        mask_height = sim_height;
        water_mask = (uint8_t*)malloc(mask_width * mask_height);
    }

    memset(water_mask, 0, mask_width * mask_height);

    int radius = PARTICLE_RADIUS;
    int radius_sq = radius * radius;

    for (int i = 0; i < particle_grid.MATlength; i++) {
        for (int j = 0; j < particle_grid.MATwidth; j++) {
            int px = (int)particle_grid.data[i][j].position.x;
            int py = (int)particle_grid.data[i][j].position.y;

            if (py < y_up || py > y_down || px < x_left || px > x_right) continue;

            int local_x = px - x_left;
            int local_y = py - y_up;

            for (int dy = -radius; dy <= radius; dy++) {
                int y = local_y + dy;
                if (y < 0 || y >= mask_height) continue;

                int dy_sq = dy * dy;
                int dx_max = (int)sqrt(radius_sq - dy_sq);

                for (int dx = -dx_max; dx <= dx_max; dx++) {
                    int x = local_x + dx;
                    if (x < 0 || x >= mask_width) continue;

                    water_mask[y * mask_width + x] = 255;
                }
            }
        }
    }

    // // === PASS 2: Léger lissage horizontal (dilatation + moyenne) ===
    // // Dilater de 1 pixel pour combler les micro-trous
    // static uint8_t* temp_mask = NULL;
    // if (temp_mask == NULL || mask_width * mask_height > 0) {
    //     temp_mask = (uint8_t*)realloc(temp_mask, mask_width * mask_height);
    // }
    // memcpy(temp_mask, water_mask, mask_width * mask_height);

    // for (int y = 1; y < mask_height - 1; y++) {
    //     for (int x = 1; x < mask_width - 1; x++) {
    //         if (temp_mask[y * mask_width + x] == 0) {
    //             // Vérifier les 4 voisins
    //             int neighbors = 0;
    //             if (temp_mask[y * mask_width + x - 1]) neighbors++;
    //             if (temp_mask[y * mask_width + x + 1]) neighbors++;
    //             if (temp_mask[(y - 1) * mask_width + x]) neighbors++;
    //             if (temp_mask[(y + 1) * mask_width + x]) neighbors++;

    //             // Remplir si au moins 2 voisins
    //             if (neighbors >= 2) {
    //                 water_mask[y * mask_width + x] = 200;
    //             }
    //         }
    //     }
    // }

    for (int x = 0; x < mask_width; x++) {
        WaterSegment segments[MAX_SEGMENTS_PER_COLUMN];
        int num_segments = 0;
        int in_water = 0;
        int segment_start = 0;

        for (int y = 0; y < mask_height; y++) {
            uint8_t val = water_mask[y * mask_width + x];

            if (val > 0) {
                if (!in_water) {
                    in_water = 1;
                    segment_start = y;
                }
            } else {
                if (in_water) {
                    in_water = 0;
                    if (num_segments < MAX_SEGMENTS_PER_COLUMN) {
                        segments[num_segments].y_start = segment_start;
                        segments[num_segments].y_end = y;
                        num_segments++;
                    }
                }
            }
        }

        if (in_water && num_segments < MAX_SEGMENTS_PER_COLUMN) {
            segments[num_segments].y_start = segment_start;
            segments[num_segments].y_end = mask_height;
            num_segments++;
        }
        for (int i = 0; i < num_segments - 1; i++) {
            int gap = segments[i + 1].y_start - segments[i].y_end;
            if (gap > 0 && gap < GAP_MERGE_THRESHOLD) {
                segments[i].y_end = segments[i + 1].y_end;
                for (int j = i + 1; j < num_segments - 1; j++) {
                    segments[j] = segments[j + 1];
                }
                num_segments--;
                i--;
            }
        }

        if (num_segments == 0) continue;

        double x_var = sin((x + x_left) * 0.03) * 0.04;

        int screen_x = x + x_left;

        for (int seg = 0; seg < num_segments; seg++) {
            int y_start = segments[seg].y_start;
            int y_end = segments[seg].y_end;
            int seg_height = y_end - y_start;
            if (seg_height <= 0) continue;

            int is_top = (seg == 0);

            for (int y = y_start; y < y_end; y++) {
                int screen_y = y + y_up;
                int dy = y - y_start;
                double depth_ratio = (double)dy / seg_height;

                int r, g, b, a;

                if (dy == 0 && is_top) {
                    r = 170; g = 210; b = 255; a = 245;
                } else if (dy == 0) {
                    r = 90; g = 150; b = 210; a = 220;
                } else if (dy < 3) {
                    double t = dy / 3.0;
                    r = (int)(150 * (1 - t) + 40 * t);
                    g = (int)(200 * (1 - t) + 130 * t);
                    b = (int)(250 * (1 - t) + 210 * t);
                    a = 235;
                } else {
                    double d = 1.0 - exp(-depth_ratio * 2.2);
                    double h = x_var * (1.0 - d * 0.6);

                    r = (int)(30 * (1 - d * 0.8) + h * 10);
                    g = (int)(120 - d * 60 + h * 15);
                    b = (int)(200 - d * 70 + h * 8);

                    if (r < 8) r = 8; if (r > 50) r = 50;
                    if (g < 40) g = 40; if (g > 140) g = 140;
                    if (b < 110) b = 110; if (b > 210) b = 210;
                    a = 250;
                }

                SDL_SetRenderDrawColor(renderer, r, g, b, a);
                SDL_RenderDrawPoint(renderer, screen_x, screen_y);
            }

            if (y_end < mask_height - 2 && seg < num_segments - 1) {
                int next_start = segments[seg + 1].y_start;
                if (next_start - y_end > GAP_MERGE_THRESHOLD) {
                    for (int e = 0; e < 2; e++) {
                        int screen_y = y_end + e + y_up;
                        if (screen_y >= y_down) break;
                        double fade = 1.0 - e / 2.0;
                        SDL_SetRenderDrawColor(renderer, (int)(70*fade), (int)(130*fade), (int)(190*fade), (int)(180*fade));
                        SDL_RenderDrawPoint(renderer, screen_x, screen_y);
                    }
                }
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

// Counting sort O(n) - plus efficace que quicksort O(n log n) pour des clés entières bornées
void counting_sort_spatial(int total)
{
    memset(counting_sort_count, 0, total * sizeof(int));

    for (int i = 0; i < total; i++) {
        counting_sort_count[Spatial_Lookup[i].value]++;
    }

    for (int i = 1; i < total; i++) {
        counting_sort_count[i] += counting_sort_count[i - 1];
    }

    for (int i = total - 1; i >= 0; i--) {
        int key = Spatial_Lookup[i].value;
        counting_sort_count[key]--;
        Spatial_Lookup_temp[counting_sort_count[key]] = Spatial_Lookup[i];
    }

    memcpy(Spatial_Lookup, Spatial_Lookup_temp, total * sizeof(Couple));
}

// Fonction qui met à jour le "Spatial Lookup" un tableau recensant chaque particule et la cellule où elle est située
// Ce tableau est ensuite trié par ordre de cellule afin d'obtenir les particules qui sont situées dans la même cellule pour optimiser les calculs de rayon Kernel
void Update_Spatial_Lookup(void)
{
    int total = particle_grid.MATlength * particle_grid.MATwidth;

    for (int i = 0; i < total; i++) {
        int pi = i / particle_grid.MATlength;
        int pj = i % particle_grid.MATlength;
        Vect2D cell = Position_to_Cell(particle_grid.data[pi][pj].predicted_position);
        int cellKey = Get_Key_from_Hash(HashCell((int)cell.x, (int)cell.y));
        Spatial_Lookup[i].index = i;
        Spatial_Lookup[i].value = cellKey;
        start_indices[i] = INT_MAX;
    }

    // Tri par counting sort O(n) au lieu de quicksort O(n log n)
    counting_sort_spatial(total);

    // Calculer les indices de début pour chaque cellule
    int previouskey = -1;
    for (int i = 0; i < total; i++) {
        int key = Spatial_Lookup[i].value;
        if (key != previouskey) {
            start_indices[key] = i;
            previouskey = key;
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
double smoothing_radius_sq;
double smoothing_kernel_volume;
double viscosity_kernel_volume;
double near_density_kernel_volume;

void init_kernel(void) {
    smoothing_radius_sq = smoothing_radius * smoothing_radius;
    smoothing_radius_pow4 = smoothing_radius_sq * smoothing_radius_sq;
    smoothing_kernel_volume = smoothing_radius_pow4 * pi / 6;
    viscosity_kernel_volume = (smoothing_radius_pow4 * smoothing_radius_pow4 * pi) / 4;
    near_density_kernel_volume = (smoothing_radius_pow4 * smoothing_radius * pi) / 10;
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

    // Cache des données de la particule courante
    double cached_masse = particle_grid.data[k][n].masse * 3;
    double base_mass = m + cached_masse;

    int min_i = SampleCell.x - 1;
    int max_i = SampleCell.x + 1;
    int min_j = SampleCell.y - 1;
    int max_j = SampleCell.y + 1;

    for (int i = min_i; i <= max_i; i++) {
        for (int j = min_j; j <= max_j; j++) {
            int key = Get_Key_from_Hash(HashCell(i, j));

            if (key < 0 || key >= total_particles) continue;

            int cellStartIndex = start_indices[key];
            if (cellStartIndex == INT_MAX) continue;

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
                    density += base_mass * Smoothing_Kernel(q);
                    near_density += base_mass * Near_density_Kernel(q);
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


// Viscosité simple - force qui égalise les vitesses entre particules voisines
Vect2D Calculate_Viscosity_Force(int k, int n)
{
    Vect2D viscosityforce = {0, 0};
    Vect2D position = particle_grid.data[k][n].predicted_position;
    Vect2D SampleCell = Position_to_Cell(position);

    double my_vel_x = particle_grid.data[k][n].velocity.x;
    double my_vel_y = particle_grid.data[k][n].velocity.y;
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

                if (k != pi || n != pj) {
                    double dx = particle_grid.data[pi][pj].predicted_position.x - position.x;
                    double dy = particle_grid.data[pi][pj].predicted_position.y - position.y;
                    double dist_sq = dx * dx + dy * dy;

                    if (dist_sq < smoothing_radius_sq) {
                        double q = sqrt(dist_sq);
                        double influence = Viscosity_smoothing_kernel(q);
                        viscosityforce.x += (particle_grid.data[pi][pj].velocity.x - my_vel_x) * influence;
                        viscosityforce.y += (particle_grid.data[pi][pj].velocity.y - my_vel_y) * influence;
                    }
                }
            }
        }
    }
    return viscosityforce;
}

// Cohésion/tension de surface - tire les particules vers leurs voisins
// S'active UNIQUEMENT quand la densité est basse (particules dispersées)
// => Sécurisé: pas d'effet au démarrage quand les particules sont compactes
Vect2D Calculate_Cohesion_Force(int k, int n)
{
    Vect2D cohesion = {0, 0};

    // Seuil: cohésion s'active seulement si densité < 60% de la cible
    double density = particle_grid.data[k][n].density;
    double density_threshold = target_density * 0.6;

    if (density >= density_threshold) {
        // Densité suffisante, pas besoin de cohésion
        return cohesion;
    }

    Vect2D position = particle_grid.data[k][n].predicted_position;
    Vect2D SampleCell = Position_to_Cell(position);

    int neighbor_count = 0;
    double avg_x = 0, avg_y = 0;
    int total_particles = particle_grid.MATlength * particle_grid.MATwidth;

    // Trouver la position moyenne des voisins
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

                if (k != pi || n != pj) {
                    double dx = particle_grid.data[pi][pj].predicted_position.x - position.x;
                    double dy = particle_grid.data[pi][pj].predicted_position.y - position.y;
                    double dist_sq = dx * dx + dy * dy;

                    if (dist_sq < smoothing_radius_sq && dist_sq > 0) {
                        avg_x += particle_grid.data[pi][pj].predicted_position.x;
                        avg_y += particle_grid.data[pi][pj].predicted_position.y;
                        neighbor_count++;
                    }
                }
            }
        }
    }

    if (neighbor_count > 0) {
        avg_x /= neighbor_count;
        avg_y /= neighbor_count;

        // Facteur basé sur combien la densité est basse
        // Plus la densité est basse, plus la cohésion est forte
        double density_factor = 1.0 - (density / density_threshold);
        if (density_factor < 0) density_factor = 0;
        if (density_factor > 1) density_factor = 1;

        // Tirer vers la position moyenne des voisins
        cohesion.x = (avg_x - position.x) * density_factor;
        cohesion.y = (avg_y - position.y) * density_factor;
    }

    return cohesion;
}

// Applique une force répulsive ou attractive au praticule selon le click utilisé
Vect2D Mouse_force(Vect2D inputPos, int k, int n, double strength)
{
    Vect2D interaction_Force = Vect2D_zero();
    Vect2D offset;
    offset.x = inputPos.x - particle_grid.data[k][n].position.x;
    offset.y = inputPos.y - particle_grid.data[k][n].position.y;
    double q = sqrt(offset.x * offset.x + offset.y * offset.y);

    if (q <= smoothing_radius * 2) {
        Vect2D dir_to_input_force = Vect2D_zero();
        if (q >= FLT_EPSILON) {
            dir_to_input_force.x = offset.x / q;
            dir_to_input_force.y = offset.y / q;
        }
        
        double influence = 1.0 - (q / (smoothing_radius * 2));
        
        double spring_force = q * strength * influence / 8;
        
        double damping = 0.25; 
        Vect2D velocity = particle_grid.data[k][n].velocity;
        
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
        // align(); 
        fill_water_area_gradient(); 
    }
}

// Fonction parallèle pour calculer la densité (appelée par chaque thread)
void* parallel_calculate_density(void* arg)
{
    ThreadRange* range = (ThreadRange*)arg;
    int total = particle_grid.MATlength * particle_grid.MATwidth;

    for (int idx = range->start_idx; idx < range->end_idx && idx < total; idx++) {
        int i = idx / particle_grid.MATwidth;
        int j = idx % particle_grid.MATwidth;
        double2 densities = Calculate_Density(i, j);
        particle_grid.data[i][j].density = densities.first_value;
        particle_grid.data[i][j].near_density = densities.second_value;
    }
    return NULL;
}

// Fonction parallèle pour calculer les forces de pression
void* parallel_calculate_pressure(void* arg)
{
    ThreadRange* range = (ThreadRange*)arg;
    int total = particle_grid.MATlength * particle_grid.MATwidth;

    for (int idx = range->start_idx; idx < range->end_idx && idx < total; idx++) {
        int i = idx / particle_grid.MATwidth;
        int j = idx % particle_grid.MATwidth;

        Vect2D pressure_force = Calculate_Pressure_Force(i, j);
        double density = particle_grid.data[i][j].density;

        if (density > 0) {
            particle_grid.data[i][j].velocity.x += (pressure_force.x / density) * dt;
            particle_grid.data[i][j].velocity.y += (pressure_force.y / density) * dt;
        }
    }
    return NULL;
}

// Fonction parallèle pour calculer la viscosité et la cohésion
void* parallel_calculate_viscosity(void* arg)
{
    ThreadRange* range = (ThreadRange*)arg;
    int total = particle_grid.MATlength * particle_grid.MATwidth;

    for (int idx = range->start_idx; idx < range->end_idx && idx < total; idx++) {
        int i = idx / particle_grid.MATwidth;
        int j = idx % particle_grid.MATwidth;

        Vect2D viscosity_force = Calculate_Viscosity_Force(i, j);
        particle_grid.data[i][j].velocity.x += viscosity_force.x * viscosity_strength * dt;
        particle_grid.data[i][j].velocity.y += viscosity_force.y * viscosity_strength * dt;

        Vect2D cohesion_force = Calculate_Cohesion_Force(i, j);
        particle_grid.data[i][j].velocity.x += cohesion_force.x * cohesion_strength * dt;
        particle_grid.data[i][j].velocity.y += cohesion_force.y * cohesion_strength * dt;
    }
    return NULL;
}

// Initialise les ranges de travail pour chaque thread
void setup_thread_ranges(void)
{
    int total = particle_grid.MATlength * particle_grid.MATwidth;
    int chunk_size = total / NUM_THREADS;
    int remainder = total % NUM_THREADS;

    int current_start = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        thread_ranges[t].thread_id = t;
        thread_ranges[t].start_idx = current_start;
        int extra = (t < remainder) ? 1 : 0;
        thread_ranges[t].end_idx = current_start + chunk_size + extra;
        current_start = thread_ranges[t].end_idx;
    }
}

void run_parallel(void* (*work_func)(void*))
{
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_create(&worker_threads[t], NULL, work_func, &thread_ranges[t]);
    }
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(worker_threads[t], NULL);
    }
}

/*mise à jour des positions de chaque particule*/
void update(void)
{
    if (!isPaused) {
        Processing = true;
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);

        double dt_over_fps = 1.0 / FPS;
        double gravity_step = g * 20.0 / FPS;

        for (int i = 0; i < particle_grid.MATlength; i++) {
            for (int j = 0; j < particle_grid.MATwidth; j++) {
                particle_grid.data[i][j].velocity.y += gravity_step;
                particle_grid.data[i][j].predicted_position.x = particle_grid.data[i][j].position.x + particle_grid.data[i][j].velocity.x * dt_over_fps;
                particle_grid.data[i][j].predicted_position.y = particle_grid.data[i][j].position.y + particle_grid.data[i][j].velocity.y * dt_over_fps;
            }
        }

        Update_Spatial_Lookup();

        setup_thread_ranges();

        run_parallel(parallel_calculate_density);

        run_parallel(parallel_calculate_pressure);

        run_parallel(parallel_calculate_viscosity);

        // Mise à jour des positions 
        for (int i = 0; i < particle_grid.MATlength; i++) {
            for (int j = 0; j < particle_grid.MATwidth; j++) {
                particle_grid.data[i][j].position.x += particle_grid.data[i][j].velocity.x * dt;
                particle_grid.data[i][j].position.y += particle_grid.data[i][j].velocity.y * dt;
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
    double target_frame_time = 1000.0 / 480.0;  
    
    restart:
    initmat();
    init_kernel();  // Recalculer smoothing_radius_sq
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
                         // Vérifier si on clique dans la zone des sliders
                         if (is_in_slider_panel((int)Sample_point.x, (int)Sample_point.y)) {
                             // Mode slider: interagir avec les sliders sans appliquer de force
                             while (Event.type != SDL_MOUSEBUTTONUP) {
                                 if (Event.type == SDL_MOUSEMOTION) {SDL_GetMouseState(&mousex,&mousey);}
                                 Sample_point.x = mousex;
                                 Sample_point.y = mousey;
                                 int slider_idx = get_slider_at_position((int)Sample_point.x, (int)Sample_point.y);
                                 if (slider_idx >= 0) {
                                     update_slider_from_mouse(slider_idx, (int)Sample_point.x);
                                     if (slider_idx == 1) smoothing_radius_sq = smoothing_radius * smoothing_radius;
                                 }

                                 
                                 update();
                                 end_time = SDL_GetTicks();
                                 double elapsed_time = (double)(end_time - start_time);
                                 draw_scale();
                                 stat_aff(1000.0 / (elapsed_time + 0.0001));
                                 SDL_RenderPresent(renderer);
                                 SDL_PollEvent(&Event);
                             }
                         } else if (Sample_point.x > 0 && Sample_point.x < width && Sample_point.y > 0 && Sample_point.y < height) {
                             // Mode simulation: appliquer la force souris
                             while (Event.type != SDL_MOUSEBUTTONUP) {
                                start_time = SDL_GetTicks();
                                 if (Event.button.button == SDL_BUTTON_LEFT) {
                                     if (mouse_force > 0) { mouse_force *= -1; }
                                 } else if (Event.button.button == SDL_BUTTON_RIGHT) {
                                     if (mouse_force < 0) { mouse_force *= -1; }
                                 }
                                 if (Event.type == SDL_MOUSEMOTION) {SDL_GetMouseState(&mousex,&mousey);}
                                 Sample_point.x = mousex;
                                 Sample_point.y = mousey;
                                 // Appliquer la force seulement dans la zone de simulation
                                 if (Sample_point.x > 0 && Sample_point.x < width && Sample_point.y > 0 && Sample_point.y < height) {
                                     Vect2D SampleCell = Vect2D_cpy(Position_to_Cell(Sample_point));
                                     for (int i = SampleCell.x - 2; i <= SampleCell.x + 2; i++) {
                                         for (int j = SampleCell.y - 2; j <= SampleCell.y + 2; j++) {
                                             int key = Get_Key_from_Hash(HashCell(i,j));
                                             int cellStartIndex = start_indices[key];
                                             for (int l = cellStartIndex; l<particle_grid.MATlength*particle_grid.MATwidth; l++) {
                                                 if (Spatial_Lookup[l].value != key) { break; }
                                                 O++;
                                                 int particle_Index = Spatial_Lookup[l].index;
                                                 particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity = Vect2D_add(particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity,  Mouse_force(Sample_point,particle_Index/particle_grid.MATlength,particle_Index%particle_grid.MATlength,mouse_force));
                                             }
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
                        } else {
                            // Nouveau système de sliders unifié
                            int slider_idx = get_slider_at_position((int)Sample_point.x, (int)Sample_point.y);
                            if (slider_idx >= 0) {
                                update_slider_from_mouse(slider_idx, (int)Sample_point.x);
                                if (slider_idx == 1) smoothing_radius_sq = smoothing_radius * smoothing_radius;
                            }
                        }
                     } else if (choice == -1) {
                         if (Event.button.button == SDL_BUTTON_LEFT) {
                             if (Sample_point.x > 0 && Sample_point.x < width && Sample_point.y > 0 && Sample_point.y < height) {
                                 Examine_Density(Sample_point.x,Sample_point.y);    
                                 while (Event.type != SDL_MOUSEBUTTONUP) {
                                     SDL_PollEvent(&Event);
                                 }
                             }
                            // Vérifier les sliders
                            int slider_idx = get_slider_at_position((int)Sample_point.x, (int)Sample_point.y);
                            if (slider_idx >= 0) {
                                update_slider_from_mouse(slider_idx, (int)Sample_point.x);
                                if (slider_idx == 1) smoothing_radius_sq = smoothing_radius * smoothing_radius;
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
    init_sliders();
    init_kernel();
    aff(NULL);



    return 0;
}
