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

#include "Draw_module.h"



int get_number_of_particle()
{
    int out = 0;
    for (int i = 0; i<particle_grid.MATlength; i++) {
        for (int j = 0; j<particle_grid.MATwidth; j++ ) {
            if (particle_grid.data[i][j].position.x>=0 && particle_grid.data[i][j].position.x<=width && particle_grid.data[i][j].position.y>=0 && particle_grid.data[i][j].position.y<=height) {
                out++;
            }
        }
    }
    return out;
}


double get_time()
{
    return (double)(clock() - starttime) / CLOCKS_PER_SEC;
}


char* complexity()
{
    double n = particle_grid.MATlength*particle_grid.MATwidth;
    if (O>=n && O<pow(n,2)) {
        return "O(n)";
    } else if (O>=pow(n,2) && O<pow(n,3)) {
        return "O(n^2)";
    } else if (O>=pow(n,3)) {
        return "O(n^3)";
    } else {
        return "O(0)";
    }
    
}


unsigned long get_cpu_time(const char* pid) 
{
    snprintf(stat_file_path, sizeof(stat_file_path), "/proc/%s/stat", pid);
    FILE* stat_file = fopen(stat_file_path, "r");
    if (stat_file != NULL) {
        fscanf(stat_file, "%*d %255s %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu", comm, &utime, &stime);
        fclose(stat_file);
        total_time = utime + stime;
        return total_time;
    } else {
        fclose(stat_file);
    }
    return 0;
}


bool present(long int PID, long int tab[40], int n)
{
    int i = 0, a = 0;
    while (a==0 && i<n) {
        if (tab[i]==PID) {
            a++;
        }
        i++;
    }
    return a!=0;
}

int alreadyp()
{
    int i = 0, a = 0;
    while (a == 0 && i<30) {
        if (strcmp(comm,prog[i].id)==0) {
            a++;
        }
        i++;
    }
    return i%30;
}


void getCPUstat()
{
    unsigned long prev_total_time = 0;
    int index = 0, ans;
    DIR* proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Erreur lors de l'ouverture du répertoire /proc");
    }



    struct dirent* entry;
    if (a%1 == 0) {
        total_time_all_procs = 0;
        while ((entry = readdir(proc_dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                pid = strtol(entry->d_name, &endptr, 10);                                                                  
                if (*endptr == '\0') {
                    total_time_all_procs += get_cpu_time(entry->d_name);
                }
            }
        }
        rewinddir(proc_dir);
     
    a = 0;
    }


    index = 0;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            pid = strtol(entry->d_name, &endptr, 10);
            if ( present(pid,tab,20)==1 || a == 0 ) { 
                if (*endptr == '\0') {
                    total_time = get_cpu_time(entry->d_name);
                    delta_time = total_time - prev_total_time;
                    prev_total_time = total_time;
                    cpu_usage = 0.0;
                    
                    if (total_time_all_procs > 0) {
                        cpu_usage = (delta_time * 100.0) / total_time_all_procs;
                    }
                    if (cpu_usage>0.1 && cpu_usage<=100) {
                        ans = alreadyp();
                        if (ans==0) {
                            prog[index%20].PID = pid;
                            strcpy(prog[index%20].id, comm); 
                            prog[index%20].cpu_usage = cpu_usage;


                            // strcpy(prog[index].color.name,Palette[index%7].name);
                            // prog[index].color.R = Palette[index%7].R;
                            // prog[index].color.G = Palette[index%7].G; 
                            // prog[index].color.B = Palette[index%7].B;
                            
                        } else {
                            prog[ans-1].cpu_usage += cpu_usage;
                        }
                        tab[index] = pid;
                        index++;
                    }
                }
                
            }
        }
    }
    a++;
    if (a%10>0 || a<2) {
        drawcpudata();
    }
    for (int i = 0 ; i<20; i++) {
        prog[i].cpu_usage = 0;

    }

    closedir(proc_dir);        
}



void stat_aff(int fps)    
/*affichage des statistiques*/ 
{
    drawstatgrid();
    static int i = 0;
    sprintf(texte, "FPS : %d", fps);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = width+30;
    rect_texte.y = (width/40)+10;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "Nombre d'operations par frame : %d", O);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "Temps ecoule : %-2f s", get_time());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "Nombre de particule dans le cadre : %d", get_number_of_particle());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "Complexite : T(n) = %s", complexity());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    O = 0;
    if (i%1 == 0) {
        // getCPUstat();
        i++;
    } else {
        i++;
    }
    
} 