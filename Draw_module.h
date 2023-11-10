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


void draw_Cartesian_axes()
{
    SDL_SetRenderDrawColor(renderer,RGB_txt,RGB_txt,RGB_txt,SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,height/40+(((height-(height/20))/20)),width-(width*3/40)-(width/150),height/40+(((height-(height/20))/20)*2),width-(width*3/40)-(width/150));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*18),(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*19));

    /*tip of arrows*/
    SDL_RenderDrawLine(renderer,height/40+(((height-(height/20))/20)*2),width-(width*3/40)-(width/150),(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)-(width/140));
    SDL_RenderDrawLine(renderer,height/40+(((height-(height/20))/20)*2),width-(width*3/40)-(width/150),(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)-(width/140),(height/40+(((height-(height/20))/20)*2))-(height/140),width-(width*3/40)-(width/150)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*18),(height/40*3)-(height/275)-(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275),(width/40)+(((width-(width/20))/20)*18),(height/40*3)-(height/275)+(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140));
    SDL_RenderDrawLine(renderer,(height/40*3)-(height/275)-(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140),(height/40*3)-(height/275)+(height/140),(width/40)+(((width-(width/20))/20)*18)+(width/140));
    /*txt*/
    sprintf(texte, "1m");
    surface_texte = TTF_RenderText_Blended(smallfont, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.x = (height/40+(((height-(height/20))/20))+height/40+(((height-(height/20))/20)*2))/2-height/80;
    rect_texte.y = width-(width*3/40)+width/300;
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;    
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    rect_texte.x = (height/40*3)-1-height/40;
    rect_texte.y = ((width/40)+(((width-(width/20))/20)*(18+19)))/2+width/140;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
}

void draw_grid()
{

    SDL_SetRenderDrawColor(renderer,RGB_lines,RGB_lines,RGB_lines,SDL_ALPHA_TRANSPARENT);
    for (int i = 1; i<=(int)(width/smoothing_radius); i++) {
        SDL_RenderDrawLine(renderer,smoothing_radius*i,0,smoothing_radius*i,height);
        SDL_RenderDrawLine(renderer,0,smoothing_radius*i,width,smoothing_radius*i);
    }
    SDL_SetRenderDrawColor(renderer,RGB_lines,RGB_lines,RGB_lines,SDL_ALPHA_TRANSPARENT);
    draw_rect(x_left2,y_up,x_right2-1,y_down-1);
    draw_Cartesian_axes();

}

void drawCircle(int X, int Y, int radius) 
{
    for (int x = X - radius; x <= X + radius; x++) {
        for (int y = Y - radius; y <= Y + radius; y++) {
            if (pow(x - X, 2) + pow(y - Y, 2) <= pow(radius, 2)) {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }

}

void dessinerCercle(int x, int y, int rayon) {
    for (int i = 0; i < 360; i++) {
        double angle = i * pi / 180;
        int xPoint = x + (int)(rayon * cos(angle));
        int yPoint = y + (int)(rayon * sin(angle));
        SDL_RenderDrawPoint(renderer, xPoint, yPoint);
    }
}

void drawstatgrid()
{
    draw_rect(width+widthstats/30,height/40,widthstats-widthstats/15,height/4-height/40);    
}

void clear_grid()
{
    SDL_SetRenderDrawColor(renderer,RGB_background,RGB_background,RGB_background,SDL_ALPHA_OPAQUE);
    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            SDL_RenderDrawPoint(renderer, particle_grid.data[i][j].position.x,particle_grid.data[i][j].position.y);
        }
    } 
    square.x = 0;
    square.y = 0;
    square.h = height;
    square.w = width+widthstats+widthscale;
    SDL_RenderFillRect(renderer,&square);
}



void drawcpudata()
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


void Colorflipped()
{
    if (RGB_background == 22) { RGB_background = 230; } else { RGB_background = 22; }
    if (RGB_lines == 50) { RGB_lines = 175; } else { RGB_lines = 50; }
    if (RGB_txt == 255) { RGB_txt = 0,white.r = 0, white.g = 0, white.b = 0; } else { RGB_txt = 255, white.r = 255, white.g = 255, white.b = 255; }    
}



void draw_scale()
{
    SDL_Rect bar;
    
    SDL_SetRenderDrawColor(renderer, RGB_background, RGB_background, RGB_background, SDL_ALPHA_OPAQUE);
    draw_rect(width+widthstats/30, height/4+height/40, widthstats-widthstats/15, height/4-height/40);
    bar.x = width+widthstats/30*2;
    bar.y = height/4+height/20;
    bar.w = widthstats/2;
    bar.h = height/400;
    SDL_RenderFillRect(renderer,&bar);
    

    rect_texte.x = bar.x+bar.w+widthstats/16+widthstats/100;
    rect_texte.y = bar.y-bar.h*6;
    sprintf(texte, "FPS = %.0lf", FPS);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(xFPS,yFPS,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);

    rect_texte.y += (height/4-height/10)/5;
    sprintf(texte, "h = %.0lf", smoothing_radius);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(xh,yh,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);

    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    sprintf(texte, "m = %.0lf", m);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(xm,ym,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    sprintf(texte, "t_dens = %.6lf", target_density);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_coeff_visco,y_coeff_visco,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    sprintf(texte, "p = %.0lf", pressure_multiplier);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(xk,yk,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);




}



