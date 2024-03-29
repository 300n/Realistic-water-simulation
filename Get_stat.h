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



void stat_aff(int fps)    
/*affichage des statistiques*/ 
{
    drawstatgrid();
    SDL_snprintf(texte, sizeof(texte), "FPS : %d\0", fps);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = x_right-widthstats+widthstats/15;
    rect_texte.y = (width/40)+10;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_snprintf(texte, sizeof(texte), "Nombre d'operations par frame : %d\0", O);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_snprintf(texte, sizeof(texte), "Temps ecoule : %-2f s\0", get_time());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_snprintf(texte, sizeof(texte), "Nombre de particule dans le cadre : %d\0", get_number_of_particle());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_snprintf(texte, sizeof(texte), "Complexite : T(n) = %s\0", complexity());
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.y += 30;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    O = 0;
} 