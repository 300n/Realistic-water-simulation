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


clock_t starttime;
int O = 0;
char comm[256];
int color[21] = {255,255,255, 255,0,0, 0,255,0, 0,0,255, 128,128,0, 128,0,128, 0,128,128};

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


double get_time()
{
    return (double)(clock() - starttime) / CLOCKS_PER_SEC;
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


unsigned long get_cpu_time(const char* pid) {
    char stat_file_path[256];
    snprintf(stat_file_path, sizeof(stat_file_path), "/proc/%s/stat", pid);
    FILE* stat_file = fopen(stat_file_path, "r");
    if (stat_file != NULL) {
        char state;
        int ppid;
        int pgrp;
        int session;
        int tty_nr;
        int tpgid;
        unsigned int flags;
        unsigned long minflt;
        unsigned long cminflt;
        unsigned long majflt;
        unsigned long cmajflt;
        unsigned long utime;
        unsigned long stime;
        long cutime;
        long cstime;
        long priority;
        long nice;
        long num_threads;
        long long starttime;
        O++;
        fscanf(stat_file, "%*d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %lld",
               comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt, &cminflt,
               &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &priority, &nice, &num_threads,
               &starttime);

        fclose(stat_file);
        return (unsigned long) utime + stime;
    }
    return 0;
}

void drawCPUstat(char* comm, double cpu_usage, int R, int G, int B)
{
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = width;
    square.y = height/2;
    square.h = height/2;
    square.w = width+600;
    SDL_RenderFillRect(renderer, &square);
    int i;
    SDL_SetRenderDrawColor(renderer,R,G,B,SDL_ALPHA_OPAQUE);
    drawCircle(width+300,height-height/4,100);
    


    
}

void getCPUstat()
{
    int i = 0; 
    unsigned long prev_total_time = 0;
    DIR* proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Erreur lors de l'ouverture du répertoire /proc");
    }
    struct dirent* entry;
    unsigned long total_time_all_procs = 0;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char* endptr;
            long pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0') {
                unsigned long total_time = get_cpu_time(entry->d_name);
                total_time_all_procs += total_time;
            }
        }
    }
    rewinddir(proc_dir);
    while ((entry = readdir(proc_dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                char* endptr;
                long pid = strtol(entry->d_name, &endptr, 10);
                if (*endptr == '\0') {
                    unsigned long total_time = get_cpu_time(entry->d_name);
                    unsigned long delta_time = total_time - prev_total_time;
                    prev_total_time = total_time;

                    // Calculer l'utilisation du CPU en pourcentage
                    float cpu_usage = 0.0;
                    if (total_time_all_procs > 0) {
                        cpu_usage = (delta_time * 100.0) / total_time_all_procs;
                    }
                    if (cpu_usage>0.1 && cpu_usage<=100) {
                        drawCPUstat(comm,cpu_usage,color[i],color[i+1],color[i+2]);
                        i +=3;
                    }
                }
            }
        }


}



void stat_aff(int fps)
/*affichage des statistiques*/ 
{
    static int i = 0;
    sprintf(texte, "fps : %d", fps);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = width+30;
    rect_texte.y = (width/40)+10;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "nombre d'operations par frame : %d", O);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "temps ecoule : %-2f s", get_time());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    sprintf(texte, "nombre de particule dans le cadre : %d", get_number_of_particle());
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
    if (i%30 == 0) {
        getCPUstat();
        i++;
    } else {
        i++;
    }
} 