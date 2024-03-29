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
bool Processing = false;
bool inputing = false;
bool full_screen = false;
bool help = false;
bool drawing_obstacles = false;

// Trouve toutes les particules qui se situent au dessus et forment la ligne barrière entre "l'air" et "l'eau"
// Fonction peu efficace en O(n) probablement optimisable  
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
int particle_on_top_space;

void particule_on_top()
{
    particle_on_top_space = 0;
    for (int Cell_x = 0; Cell_x<width/smoothing_radius; Cell_x++) {
        int maxheight = height;
        int cellStartIndex = start_indices[Cell_x];
        for (int l = cellStartIndex; l<particle_grid.MATlength*particle_grid.MATwidth; l++) {
            if (Spatial_Lookup[l].value != Cell_x) { break; }
            int particle_Index = Spatial_Lookup[l].index;
            if (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].position.y < maxheight) {
                maxheight = particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].position.y;
                particle_grid.particle_on_top[particle_on_top_space] = particle_Index;
            } 

        }
        // printf("%d\n", particle_on_top_space);                                                                                              
        particle_on_top_space++;
    }
    // for (int k = 0; k < (particle_on_top_space); k++) { 
    //     printf("%d\n",particle_grid.particle_on_top[k]);
    // }
}

                                                                                                                                                                        
// Trace un trait entre les particules qui consitues le dessus du fluide 
// Manque de clareté + nombreux problèmes (ligne qui disparaît entre deux segments) a améliorer
void align()
{
     printf("%d\n", particle_on_top_space);
    int x1, y1, x2, y2;
    SDL_SetRenderDrawColor(renderer,0,0,255,SDL_ALPHA_OPAQUE);
    for (int k = 0; k < (particle_on_top_space); k++) {
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




















// Calcul de distance entre deux particules avec la norme des deux vecteurs (nom?)
double Eularian_distance(int i, int j, int k, int n)
{
    return sqrt(pow((particle_grid.data[k][n].predicted_position.x - particle_grid.data[i][j].predicted_position.x),2) + pow((particle_grid.data[k][n].predicted_position.y - particle_grid.data[i][j].predicted_position.y),2));
}


// Fonction responsable de l'optimisation sur le calcul des particules dans le rayon Kernel
// On se base sur l'optimisation proposée ici https://web.archive.org/web/20140725014123/https://docs.nvidia.com/cuda/samples/5_Simulations/particles/doc/particles.pdf
// Résultat de l'optimisation => augmentation de 2000% des performance, on passe d'une complexité en O(n²) à un complexité théorique en O(n) même si en réalité en pratique 
// il s'agit plutôt d'une complexité en O(n² / 4)

// Trouve la position sur la grille de chaque cellulle en fonction de sa position dans le cadre
Vect2D Position_to_Cell(Vect2D point)
{   
    Vect2D out;
    out.x = (int) (point.x / smoothing_radius);
    out.y = (int) (point.y / smoothing_radius);
    return out;
}

// Calcul le Hash de la case où se situe la particule
// Nombres premiers choisis pour le hash hashK1 = 15823 && hashK2 = 9737333 => intérêt de prendre de grand nombre afin d'éviter l'overlapping des cellules
u_int64_t HashCell(int cellX, int cellY)
{
    u_int64_t a = (int)cellX * hashK1;
    u_int64_t b = (int)cellY * hashK2;
    return a + b;
}

// Obtention de la clé de la cellule où se situe la particule afin d'effectuer un tri par la suite en fonction de cette clé
int Get_Key_from_Hash(u_int64_t hash)
{
    // printf("hash = %d\n\n", hash);
    return hash % (int)(particle_grid.MATlength*particle_grid.MATwidth);
}

// tri fusion avec une complexité moyenne de 0(nlogn)
void merge(Couple arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m-l+1;
    int n2 = r-m;

    Couple L[n1], R[n2];
    for (i = 0; i<n1; i++)
        L[i] = arr[l+i];
    for (j = 0; j<n2; j++)
        R[j] = arr[m+1+j];

    i = 0;
    j = 0;
    k = l;
    while (i<n1 && j<n2) {
        if (L[i].value<=R[j].value) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    while (i<n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    while (j<n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(Couple arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}


// Fonction qui met à jour le "Spatial Lookup" un tableau recensant chaque particule est la cellule ou elle est située
// Ce tableau est ensuite trié par ordre de cellule afin d'obtenir les particules qui sont situées dans la même cellule pour optimiser les calculs de rayon Kernel
// Pas d'amélioration prévue ici
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

    // Tri en fonction des cellules
    mergeSort(Spatial_Lookup,0,particle_grid.MATlength*particle_grid.MATwidth-1);

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
        double volume = (pi * pow(smoothing_radius, 8)) / 4;
        double v = smoothing_radius * smoothing_radius - dst * dst;
        return v * v * v / volume;
    }
}

// Troisième Kernel permettant de calculer la densité pour deux particules extrêmement proches
// Utilisation de la fonction f(x) = (rayon_kernel - x)^3
// Si la distance entre les deux particules est supérieure au rayon kernel alors la valeur de la fonction kernel est égale à 0
// Calcul du volume en double intégrant entre 0 et le rayon kernel et 0 et 2pi 
// Integrate[Integrate[Power[\(40)s-x\(41),3]x,{θ,0,2π}],{x,0,s}]
// https://www.wolframalpha.com/input?i2d=true&i=Integrate%5BIntegrate%5BPower%5B%5C%2840%29s-x%5C%2841%29%2C3%5Dx%2C%7B%CE%B8%2C0%2C2%CF%80%7D%5D%2C%7Bx%2C0%2Cs%7D%5D
// On obtient alors volume = (pi * rayon_kernel^5) / 10
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


// Calcul de la densité en utilisant le parcours 3x3 vu précédement 
// en ajoutant la masse de chaque particule x l'influence de cette particule sur la particule concernée
// Problèmes -> Calcul des densités à l'extérieur des limites -> volume souvent trop élevé
// Conséquences -> Les particules sont aglutinées sur les bords 
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
				        dir.x = (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].predicted_position.x-particle_grid.data[k][n].predicted_position.x)
                         / q; 
				        dir.y = (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].predicted_position.y-particle_grid.data[k][n].predicted_position.y)
                         / q;
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



// Utilisation d'une fonction kernel moins pentue -> pas de besoin d'augmenter la viscosité si deux particules sont trop proches
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
                        double influence  = Viscosity_smoothing_kernel(q);
                        viscosityforce.x += (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity.x - particle_grid.data[k][n].velocity.x) * influence;
                        viscosityforce.y += (particle_grid.data[particle_Index/particle_grid.MATlength][particle_Index%particle_grid.MATlength].velocity.y - particle_grid.data[k][n].velocity.y) * influence;

                    }
                }
            }
        }
    }
    return viscosityforce;
}


// Applique une force répulsive ou attractive au praticule selon le click utilisé
Vect2D Mouse_force(Vect2D inputPos, int k ,int n, double strength)
{
    Vect2D interaction_Force = Vect2D_zero();
    Vect2D offset;
    offset.x = inputPos.x - particle_grid.data[k][n].position.x;
    offset.y = inputPos.y - particle_grid.data[k][n].position.y;
    double q = sqrt(pow(offset.x,2)+pow(offset.y,2));

    if (q <= smoothing_radius*4) {
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

// Visualise les endroits de la simulation où la densité n'est pas assez/trop grande par rapport à la densité visée
// Si cette densité est trop petite alors la zone apparait en couleur froide (bleu) et si c'est l'inverse il s'agit d'une couleur chaude (orange)
// Pour les zones proches de la densité visée elles apparaissent en blanc
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


// Détecte une sortie de l'écran pour une partciule
void particle_out_of_the_grid()
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
    if (help) {
        draw_help();
    }
    if (particle_visible == -1) {
        particule_on_top();
        align();
        for (int i = 0 ; i < particle_grid.MATlength ; i++) {
            for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
                SDL_SetRenderDrawColor(renderer, particle_grid.data[i][j].color[0], particle_grid.data[i][j].color[1], particle_grid.data[i][j].color[2], SDL_ALPHA_OPAQUE);
                drawCircle(particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y,10);
     
            }
        }   
    }
}

/*mise à jour des positions de chaque particule*/
void update()   
{
    if (!isPaused) {
	    for (int i = 0 ; i < particle_grid.MATlength ; i++) {
            for (int j = 0 ; j < particle_grid.MATwidth ; j++) {
	    		particle_grid.data[i][j].velocity.y += g * 20 / FPS;
                particle_grid.data[i][j].predicted_position.x = particle_grid.data[i][j].position.x + particle_grid.data[i][j].velocity.x* 1 / FPS;
                particle_grid.data[i][j].predicted_position.y = particle_grid.data[i][j].position.y + particle_grid.data[i][j].velocity.y* 1 / FPS;
	    	}
	    }


        Update_Spatial_Lookup(); /// PB d'optimisation ici 100%

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
                }
            }
        }
        Processing = false;

        affichage();

    }
}

// Décale l'espace de simulation vers la droite pendant un instant avant de revenir à l'état initial
void rightshift() 
{
    int timeout = SDL_GetTicks64() + 500; 
    int temp = x_right;
    while (SDL_GetTicks64() < timeout){
        update();
        SDL_RenderPresent(renderer);
        x_left += 10;
        x_right += 10;
    }
    while (x_right>temp){
        update();
        SDL_RenderPresent(renderer);
        x_left -= 10;
        x_right -= 10;
    }
}

// Décale l'espace de simiulation vers le haut pendant un instant avant de revenir à l'état initial 
void upshift()
{
    int timeout = SDL_GetTicks64() + 1000;
    int temp = y_down;
    while (SDL_GetTicks64() < timeout){
        update();
        SDL_RenderPresent(renderer);
        y_up -= 10;
        y_down -= 10;
    }
    while (y_down<temp){
        update();
        SDL_RenderPresent(renderer);
        y_up += 10;
        y_down += 10;
    }
}



int running = 1, choice = 1;


void aff()
/*affichage des particules*/
{
    SDL_Event Event;
    int mousex, mousey;
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
                     } else if (Event.key.keysym.scancode == SDL_SCANCODE_U) {
                         upshift();
                     } else if (Event.key.keysym.scancode == SDL_SCANCODE_D) {
                         drawing_obstacles = !drawing_obstacles;
                     }  else if (Event.key.keysym.scancode== SDL_SCANCODE_H) {
                        help = !help;
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
                     } else if (Event.key.keysym.scancode  == SDL_SCANCODE_H) {
                        stat_visual_status = !stat_visual_status;
                     }

                     break;
                 case SDL_MOUSEBUTTONDOWN:
                     Vect2D Sample_point = Vect2D_zero();
                     SDL_GetMouseState(&mousex,&mousey);
                     Sample_point.x = mousex;
                     Sample_point.y = mousey;
                     if (choice == 1 && !drawing_obstacles) {
                         if (Sample_point.x > 0 && Sample_point.x < x_right && Sample_point.y > 0 && Sample_point.y < y_down && (Sample_point.x < x_right - widthstats || Sample_point.y>y_down/2)) {
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
                                 for (int i = SampleCell.x - 4; i <= SampleCell.x + 4; i++) {
                                     for (int j = SampleCell.y - 4; j <= SampleCell.y + 4; j++) {
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
                                 dessinerCercle(Sample_point.x,Sample_point.y,smoothing_radius*4);
                                 // end_time = SDL_GetTicks();
                                 // elapsed_time = end_time - start_time;
                                 // stat_aff(1000/(elapsed_time+0.0001));
                                 SDL_RenderPresent(renderer);
                                 SDL_UpdateWindowSurface(window);
                                 // printf("O = %d\n", O);
                                 SDL_PollEvent(&Event);
                            }
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>yFPS-10 && Sample_point.y<yFPS+10) {
                            FPS = FPS_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15)&& Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>yh-10 && Sample_point.y<yh+10) {
                            smoothing_radius = smoothing_radius_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>ym-10 && Sample_point.y<ym+10) {
                            m = mass_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>y_tdens-10 && Sample_point.y<y_tdens+10) {
                            target_density = target_density_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>yk-10 && Sample_point.y<yk+10) {
                            pressure_multiplier = pressure_multiplier_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>y_vs-10 && Sample_point.y<y_vs+10) {
                            viscosity_strength = viscosity_strength_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>y_np-10 && Sample_point.y<y_np+10) {
                            near_pressure_multiplier = near_pressure_multiplier_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                         }
                     } else if (choice == -1 && !drawing_obstacles) {
                         if (Event.button.button == SDL_BUTTON_LEFT) {
                             if (Sample_point.x > 0 && Sample_point.x < width && Sample_point.y > 0 && Sample_point.y < height) {
                                 Examine_Density(Sample_point.x,Sample_point.y);    
                                 while (Event.type != SDL_MOUSEBUTTONUP) {
                                     SDL_PollEvent(&Event);
                                 }
                             }
                            if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>yFPS-10 && Sample_point.y<yFPS+10) {
                               FPS = FPS_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                            } else if (Sample_point.x>(x_right-widthstats+widthstats/15)&& Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>yh-10 && Sample_point.y<yh+10) {
                               smoothing_radius = smoothing_radius_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                            } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>ym-10 && Sample_point.y<ym+10) {
                               m = mass_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                            } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>y_tdens-10 && Sample_point.y<y_tdens+10) {
                               target_density = target_density_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                            } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>yk-10 && Sample_point.y<yk+10) {
                               pressure_multiplier = pressure_multiplier_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                            } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>y_vs-10 && Sample_point.y<y_vs+10) {
                               viscosity_strength = viscosity_strength_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
                            } else if (Sample_point.x>(x_right-widthstats+widthstats/15) && Sample_point.x<(x_right-widthstats+widthstats/15)+widthstats/2 && Sample_point.y>y_np-10 && Sample_point.y<y_np+10) {
                               near_pressure_multiplier = near_pressure_multiplier_MAX * (Sample_point.x - (x_right-widthstats+widthstats/15)) / (widthstats/2);
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
                     } else if (drawing_obstacles) {
                        int drawing = 1;
                        while (drawing) {
                            while (Event.type != SDL_MOUSEBUTTONUP) {
                                    SDL_GetMouseState(&mousex,&mousey);
                                    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
                                    drawCircle(mousex,mousey,5);
                                    
                                    drawing++;
                                    SDL_RenderPresent(renderer);
                                    SDL_Delay(1);
                                    SDL_PollEvent(&Event);
                            }
                            drawing = 0;
                            isPaused = true;
                        }
                            
                         }
                     break;
             }
         }
        dt = 0.049000;

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
    SDL_FreeSurface(surface_texte);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main()
{
    initSDL();
    initTTF();
    aff();
    return 0;
}