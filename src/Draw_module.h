#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include "init.h"


int RGB_background = 22 , RGB_lines = 50, RGB_txt = 255;



void draw_rect(int x, int y, int xwidth, int yheight)
{
    SDL_SetRenderDrawColor(renderer, RGB_txt, RGB_txt, RGB_txt, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,x,y,x+xwidth,y);
    SDL_RenderDrawLine(renderer,x,y,x,y+yheight);
    SDL_RenderDrawLine(renderer,x+xwidth,y,x+xwidth,y+yheight);
    SDL_RenderDrawLine(renderer,x,y+yheight,x+xwidth,y+yheight);
}




void draw_grid(void)
{
    if (stat_visual_status) {
        SDL_SetRenderDrawColor(renderer,RGB_lines,RGB_lines,RGB_lines,SDL_ALPHA_TRANSPARENT);
        for (int i = 1; i<=(int)((x_right-x_left)/smoothing_radius); i++) {
            SDL_RenderDrawLine(renderer,smoothing_radius*i+x_left,y_up,smoothing_radius*i+x_left,y_down);
        }
        for (int i = 1; i<=(int)((y_down-y_up)/smoothing_radius); i++) {
            SDL_RenderDrawLine(renderer,x_left,smoothing_radius*i+y_up,x_right,smoothing_radius*i+y_up);
        }
    }
    // for (int i = 0; i<(int)width/smoothing_radius; i++) {
    //     for (int j = 0; j<(int)height/smoothing_radius; j++) {
    //         sprintf(texte, "(%d;%d)",j,i);
    //         surface_texte = TTF_RenderText_Blended(smallfont, texte, white);
    //         texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    //         rect_texte.x = j*smoothing_radius;
    //         rect_texte.y = i*smoothing_radius;


    //         rect_texte.w = surface_texte->w;
    //         rect_texte.h = surface_texte->h;    
    //         SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    //     }
    // }


    SDL_SetRenderDrawColor(renderer,RGB_lines,RGB_lines,RGB_lines,SDL_ALPHA_TRANSPARENT);
    draw_rect(x_left,y_up,x_right-1-x_left,y_down-1-y_up);

}

void drawCircle(int X, int Y, int radius)
{
    int radius_sq = radius * radius;
    for (int x = X - radius; x <= X + radius; x++) {
        int dx = x - X;
        int dx_sq = dx * dx;
        for (int y = Y - radius; y <= Y + radius; y++) {
            int dy = y - Y;
            if (dx_sq + dy * dy <= radius_sq) {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

// Algorithme de Bresenham 
void dessinerCercle(int xc, int yc, int r) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (y >= x) {
        SDL_RenderDrawPoint(renderer, xc + x, yc + y);
        SDL_RenderDrawPoint(renderer, xc - x, yc + y);
        SDL_RenderDrawPoint(renderer, xc + x, yc - y);
        SDL_RenderDrawPoint(renderer, xc - x, yc - y);
        SDL_RenderDrawPoint(renderer, xc + y, yc + x);
        SDL_RenderDrawPoint(renderer, xc - y, yc + x);
        SDL_RenderDrawPoint(renderer, xc + y, yc - x);
        SDL_RenderDrawPoint(renderer, xc - y, yc - x);

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void drawstatgrid(void)
{
    draw_rect(x_right-widthstats,height/40,widthstats-widthstats/15,height/4-height/40);    
}

void clear_grid(void)
{
    SDL_SetRenderDrawColor(renderer,RGB_background,RGB_background,RGB_background,SDL_ALPHA_OPAQUE);
    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            SDL_RenderDrawPoint(renderer, particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y);
        }
    } 
    square.x = 0;
    square.y = 0;
    square.h = (y_down - y_up);
    square.w = (x_right - x_left);
    SDL_RenderFillRect(renderer,&square);
}



void drawcpudata(void)
{
    SDL_Rect bar, legend;
    draw_rect(width+widthstats/30,height/40+height/2,widthstats-widthstats/15,height/2-height/20);
    sprintf(texte, "Utilisation du CPU en %% (%f%%)", cpu_tot);
    cpu_tot = 0;
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    rect_texte.x = width+widthstats/2-rect_texte.w/2;
    rect_texte.y = height/2+height/20;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);



    square.x = width+100+5;
    square.y = height-height/3-height/14+5;
    square.h = 50;
    square.w = widthstats-200;
    for (int i = 0 ;i<5; i++) {
        square.x--;
        square.y--;
        SDL_RenderDrawRect(renderer,&square);
    }
    SDL_SetRenderDrawColor(renderer, RGB_lines, RGB_lines, RGB_lines, SDL_ALPHA_OPAQUE);
    for (int i = 1; i<40; i++) {
        SDL_RenderDrawLine(renderer, square.x+(square.w/40)*i+5,square.y+5,square.x+(square.w/40)*i+7,square.y+square.h-2);
    }

    bar.x = square.x+5;
    bar.y = square.y+5;
    bar.h = square.h-6;

    legend.x = width+100;
    legend.y = height/2+height/6;
    legend.w = width/40;
    legend.h = height/40; 
    int temp = 0;
    for (int i = 0; i<20; i++) {
        if (prog[i].PID!=0 && prog[i].cpu_usage>0.8 && prog[i].PID<20000) {
            // SDL_SetRenderDrawColor(renderer,prog[i].color.R,prog[i].color.G,prog[i].color.B,SDL_ALPHA_OPAQUE);
            SDL_SetRenderDrawColor(renderer,Palette[temp%7].R,Palette[temp%7].G,Palette[temp%7].B,SDL_ALPHA_OPAQUE);
            bar.w = square.w*(prog[i].cpu_usage/100);
            cpu_tot += prog[i].cpu_usage;
            SDL_RenderFillRect(renderer,&bar);
            bar.x += bar.w;
            temp++;




            //printf("legend.x = %d, legend.y = %d, legend.w = %d, legend.h = %d\n", legend.x, legend.y, legend.w, legend.h);
            SDL_RenderFillRect(renderer,&legend);
            SDL_SetRenderDrawColor(renderer,RGB_txt,RGB_txt,RGB_txt,SDL_ALPHA_OPAQUE);
            SDL_RenderDrawRect(renderer,&legend);
            strcpy(texte,prog[i].id);
            char* texte1 = strtok(texte, "(");
            char* texte2 = strtok(texte1, ")");


            surface_texte = TTF_RenderText_Blended(font, texte2, white);
            texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
            rect_texte.x = legend.x + 30    ;
            rect_texte.y = legend.y;
            rect_texte.w = surface_texte->w;
            rect_texte.h = surface_texte->h;
            SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
            sprintf(texte, "(%-2f %%)", prog[i].cpu_usage);
            surface_texte = TTF_RenderText_Blended(font, texte, white);
            texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
            rect_texte.x += rect_texte.w+5;
            rect_texte.w = surface_texte->w;
            rect_texte.h = surface_texte->h;
            SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);

            
            legend.y += height/40+2;



        }   
    }    
}


void Colorflipped(void)
{
    if (RGB_background == 22) { RGB_background = 230; } else { RGB_background = 22; }
    if (RGB_lines == 50) { RGB_lines = 175; } else { RGB_lines = 50; }
    if (RGB_txt == 255) { RGB_txt = 0,white.r = 0, white.g = 0, white.b = 0; } else { RGB_txt = 255, white.r = 255, white.g = 255, white.b = 255; }    
}



void init_sliders(void)
{
    sliders[0] = (Slider){"dt", "%.0f", &FPS, 10, 120, FPSconst, 100, 200, 255};
    sliders[1] = (Slider){"Radius", "%.0f", &smoothing_radius, 5, 100, h, 255, 150, 100};
    sliders[2] = (Slider){"Mass", "%.1f", &m, 0.1, 10, mconst, 150, 255, 150};
    sliders[3] = (Slider){"Density", "%.4f", &target_density, 0.001, 0.05, t_dens, 255, 200, 100};
    sliders[4] = (Slider){"Pressure", "%.0f", &pressure_multiplier, 10000, 500000, k, 255, 100, 150};
    sliders[5] = (Slider){"Viscosity", "%.2f", &viscosity_strength, 0.1, 30, viscosity_strength_const, 100, 180, 255};
}

// Dessine un slider individuel
void draw_single_slider(Slider* s, int y_pos)
{
    int bar_x = SLIDER_BAR_X;
    int bar_w = SLIDER_BAR_W;
    int bar_h = SLIDER_BAR_H;
    int knob_r = SLIDER_KNOB_R;

    // Calculer la position du curseur (0.0 à 1.0)
    double ratio = (*(s->value) - s->min_val) / (s->max_val - s->min_val);
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    int knob_x = bar_x + (int)(ratio * bar_w);

    // === Fond de la barre (gris foncé) ===
    SDL_Rect bg_bar = {bar_x - 2, y_pos - bar_h/2 - 2, bar_w + 4, bar_h + 4};
    SDL_SetRenderDrawColor(renderer, 40, 40, 45, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &bg_bar);

    // === Barre de fond (gris) ===
    SDL_Rect track = {bar_x, y_pos - bar_h/2, bar_w, bar_h};
    SDL_SetRenderDrawColor(renderer, 70, 70, 80, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &track);

    // === Barre de progression (colorée) ===
    SDL_Rect progress = {bar_x, y_pos - bar_h/2, knob_x - bar_x, bar_h};
    SDL_SetRenderDrawColor(renderer, s->color_r, s->color_g, s->color_b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &progress);

    // === Curseur (knob) ===
    // Ombre du curseur
    SDL_SetRenderDrawColor(renderer, 20, 20, 25, SDL_ALPHA_OPAQUE);
    drawCircle(knob_x + 1, y_pos + 1, knob_r);
    // Curseur principal
    SDL_SetRenderDrawColor(renderer, 220, 220, 230, SDL_ALPHA_OPAQUE);
    drawCircle(knob_x, y_pos, knob_r);
    // Centre coloré
    SDL_SetRenderDrawColor(renderer, s->color_r, s->color_g, s->color_b, SDL_ALPHA_OPAQUE);
    drawCircle(knob_x, y_pos, knob_r - 3);

    // === Label (à gauche) ===
    SDL_SetRenderDrawColor(renderer, RGB_txt, RGB_txt, RGB_txt, SDL_ALPHA_OPAQUE);
    sprintf(texte, "%s", s->label);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = bar_x - surface_texte->w - 15;
    rect_texte.y = y_pos - surface_texte->h / 2;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_FreeSurface(surface_texte);
    SDL_DestroyTexture(texture_texte);

    // === Valeur (à droite) ===
    sprintf(texte, s->format, *(s->value));
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = bar_x + bar_w + 15;
    rect_texte.y = y_pos - surface_texte->h / 2;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_FreeSurface(surface_texte);
    SDL_DestroyTexture(texture_texte);
}

// Vérifie si la position est dans le panneau des sliders
int is_in_slider_panel(int mouse_x, int mouse_y)
{
    int panel_x = width + widthstats/20;
    int panel_y = SLIDER_START_Y - 30;
    int panel_w = widthstats - widthstats/10;
    int panel_h = NUM_SLIDERS * SLIDER_SPACING + 40;

    return (mouse_x >= panel_x && mouse_x <= panel_x + panel_w &&
            mouse_y >= panel_y && mouse_y <= panel_y + panel_h);
}

// Retourne l'index du slider survolé, ou -1 si aucun
int get_slider_at_position(int mouse_x, int mouse_y)
{
    int bar_x = SLIDER_BAR_X;
    int bar_w = SLIDER_BAR_W;
    int hit_margin = 15;  // Marge de clic verticale

    for (int i = 0; i < NUM_SLIDERS; i++) {
        int y_pos = SLIDER_START_Y + i * SLIDER_SPACING;

        if (mouse_x >= bar_x - 10 && mouse_x <= bar_x + bar_w + 10 &&
            mouse_y >= y_pos - hit_margin && mouse_y <= y_pos + hit_margin) {
            return i;
        }
    }
    return -1;
}


void draw_scale(void)
{
    int panel_x = x_right - widthstats;
    int panel_y = SLIDER_START_Y - 50;
    int panel_w = widthstats - widthstats/15;
    int panel_h = NUM_SLIDERS * SLIDER_SPACING + 40;

    SDL_Rect panel = {panel_x, panel_y, panel_w, panel_h};
    

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(renderer, &panel);

    sprintf(texte, "Parametres");
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = panel_x + panel_w/2 - surface_texte->w/2;
    rect_texte.y = panel_y + 5;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    SDL_FreeSurface(surface_texte);
    SDL_DestroyTexture(texture_texte);

    for (int i = 0; i < NUM_SLIDERS; i++) {
        int y_pos = SLIDER_START_Y + i * SLIDER_SPACING;
        draw_single_slider(&sliders[i], y_pos);
    }
}

void update_slider_from_mouse(int slider_idx, int mouse_x)
{
    if (slider_idx < 0 || slider_idx >= NUM_SLIDERS) return;
    draw_scale();
    Slider* s = &sliders[slider_idx];
    int bar_x = SLIDER_BAR_X;
    int bar_w = SLIDER_BAR_W;

    double ratio = (double)(mouse_x - bar_x) / bar_w;
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;

    *(s->value) = s->min_val + ratio * (s->max_val - s->min_val);
}



void draw_help(void)
{
    SDL_Rect help;
    help.h = height/2.4;
    help.w = width/3;
    help.x = 0;
    help.y = 0;
    SDL_SetRenderDrawColor(renderer, RGB_txt, RGB_txt, RGB_txt, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(renderer,&help);


    rect_texte.x = width/50;
    rect_texte.y = height/50;
    
    char command[11][100] = {
        {"[SPACE] : Stop sim\0"},
        {"[F5] : Restart sim\0"},
        {"[F11] : Full screen\0"},
        {"[->] : Moove forward\0"},
        {"[<-] : Moove backward\0"},
        {"[I] : White/Dark mode\0"},
        {"[R] : Reset const\0"},
        {"[S] : Un/show part\0"},
        {"[G] : Right shift\0"},
        {"[U] : Up shift\0"},
        {"[H] : Un/show help\0"},

    };
    for (int i = 0; i<11; i++) {
        SDL_snprintf(texte, sizeof(texte), "%s", command[i]);
        surface_texte = TTF_RenderText_Blended(font, texte, white);
        texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
        rect_texte.w = surface_texte->w;
        rect_texte.h = surface_texte->h;
        SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
        rect_texte.y += surface_texte->h*1.2;
    }
}


