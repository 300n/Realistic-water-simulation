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





void draw_Cartesian_axes()
{
    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
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
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = 0;
    square.y = 0;
    square.h = height;
    square.w = width;
    SDL_RenderFillRect(renderer, &square);
    int nbofd = 20;
    SDL_SetRenderDrawColor(renderer,50,50,50,SDL_ALPHA_TRANSPARENT);
    for (int i = 1; i<nbofd ;i++) {
        SDL_RenderDrawLine(renderer,(height/40)+(((height-(height/20))/nbofd)*i),(width/40),((height/40)+(((height-(height/20))/nbofd)*i)),width-(width/40));
        SDL_RenderDrawLine(renderer,(height/40),(width/40)+(((width-(width/20))/nbofd)*i),height-(height/40),(width/40)+(((width-(width/20))/nbofd)*i));
    }
    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,(height/40),(width/40),(height/40),width-(width/40));
    SDL_RenderDrawLine(renderer,(height/40),(width/40),height-(height/40),(width/40));
    SDL_RenderDrawLine(renderer,(height/40),width-(width/40),height-(height/40),width-(width/40));
    SDL_RenderDrawLine(renderer,height-(height/40),(width/40),height-(height/40),width-(width/40));
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

void drawstatgrid()
{
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = width;
    square.y = 0;
    square.h = height/2;
    square.w = width+600;
    SDL_RenderFillRect(renderer, &square);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer,width+20,height/40,width+560,height/40);
    SDL_RenderDrawLine(renderer,width+20,height/40,width+20,height/2-height/40);
    SDL_RenderDrawLine(renderer,width+20,height/2-height/40,width+560,height/2-height/40);
    SDL_RenderDrawLine(renderer,width+560,height/2-height/40,width+560,height/40);

    
}

void clear_grid()
{
    SDL_SetRenderDrawColor(renderer,22,22,22,SDL_ALPHA_OPAQUE);
    for (int i = 0; i<matlength; i++) {
        for (int j = 0; j<matwidth; j++) {
            SDL_RenderDrawPoint(renderer,mat.data[i][j].x,mat.data[i][j].y);
        }
    }
}



void drawcpudata()
{
    SDL_Rect bar, legend;
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    square.x = width;
    square.y = height/2;
    square.h = height/2;
    square.w = width+600;
    SDL_RenderFillRect(renderer, &square);
    SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);

    square.x = width+20;
    square.y = height/40+height/2;
    square.w = 540;
    square.h = height/2-height/20;
    SDL_RenderDrawRect(renderer,&square);




    sprintf(texte, "Utilisation du CPU en %% (%-2f%%)", cpu_tot);
    cpu_tot = 0;
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    rect_texte.x = width+600/2-rect_texte.w/2;
    rect_texte.y = height/2+height/20;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);



    square.x = width+100+5;
    square.y = height-height/3-height/14+5;
    square.h = 50;
    square.w = 400;
    for (int i = 0 ;i<5; i++) {
        square.x--;
        square.y--;
        SDL_RenderDrawRect(renderer,&square);
    }
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, SDL_ALPHA_OPAQUE);
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
            SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_OPAQUE);
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