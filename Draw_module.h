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




SDL_Color white = { 255, 255, 255, SDL_ALPHA_OPAQUE};
char texte[100];

void draw_Cartesian_axes()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
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
    SDL_RenderDrawLine(renderer,width+20,height/40,width+20,height-height/40);
    SDL_RenderDrawLine(renderer,width+20,height-height/40,width+560,height-height/40);
    SDL_RenderDrawLine(renderer,width+560,height-height/40,width+560,height/40);

    
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