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
    SDL_snprintf(texte, sizeof(texte), "1m\0");
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
    draw_rect(x_right-widthstats,height/40,widthstats-widthstats/15,height/4-height/40);    
}

void clear_grid()
{
    SDL_SetRenderDrawColor(renderer,RGB_background,RGB_background,RGB_background,SDL_ALPHA_OPAQUE);
    square.x = 0;
    square.y = 0;
    square.h = (y_down - y_up);
    square.w = (x_right - x_left)+200;
    SDL_RenderFillRect(renderer,&square);
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
    draw_rect(x_right-widthstats, height/4+height/40, widthstats-widthstats/15, height/4-height/40);
    bar.x = x_right-widthstats+widthstats/15;
    bar.y = height/4+height/20;
    bar.w = widthstats/2;
    bar.h = height/400;
    SDL_RenderFillRect(renderer,&bar);
    double x_dot = x_right-widthstats + widthstats / 15;

    rect_texte.x = bar.x+bar.w+widthstats/16+widthstats/100;
    rect_texte.y = bar.y-bar.h*6;
    SDL_snprintf(texte, sizeof(texte), "FPS = %.0lf\0", FPS);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * FPS/FPS_MAX,yFPS,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);

    rect_texte.y += (height/4-height/10)/5;
    SDL_snprintf(texte, sizeof(texte), "h = %.0lf\0", smoothing_radius);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * smoothing_radius / smoothing_radius_MAX,yh,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);

    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    SDL_snprintf(texte, sizeof(texte), "m = %.0lf\0", m);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * m / mass_MAX,ym,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    SDL_snprintf(texte, sizeof(texte), "t_dens = %.6lf\0", target_density);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * target_density / target_density_MAX,y_tdens,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    SDL_snprintf(texte, sizeof(texte), "p = %.0lf\0", pressure_multiplier);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * pressure_multiplier / pressure_multiplier_MAX,yk,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);

    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    SDL_snprintf(texte, sizeof(texte),"visco = %.3lf\0", viscosity_strength);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * viscosity_strength / viscosity_strength_MAX,y_vs,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);


    bar.y += (height/4-height/10)/5;
    SDL_RenderFillRect(renderer,&bar);
    rect_texte.y += (height/4-height/10)/5;
    SDL_snprintf(texte, sizeof(texte), "npm = %.3lf\0", near_pressure_multiplier);
    surface_texte = TTF_RenderText_Blended(font, texte, white);
    texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
    rect_texte.w = surface_texte->w;
    rect_texte.h = surface_texte->h;
    SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
    drawCircle(x_dot + bar.w * near_pressure_multiplier / near_pressure_multiplier_MAX,y_np,7);
    draw_rect(bar.x+bar.w+widthstats/16,bar.y-bar.h*6,rect_texte.w+10,bar.h*12);

}



void draw_help()
{
    SDL_Rect help;
    help.h = height/2.5;
    help.w = width/4;
    help.x = 0;
    help.y = 0;
    SDL_SetRenderDrawColor(renderer, RGB_txt, RGB_txt, RGB_txt, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(renderer,&help);


    rect_texte.x = width/50;
    rect_texte.y = height/50;
    
    char command[12][100] = {
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
        {"[D] : Draw obstacles\0"},
        {"[H] : Un/show help\0"},

    };
    for (int i = 0; i<12; i++) {
        SDL_snprintf(texte, sizeof(texte), command[i]);
        surface_texte = TTF_RenderText_Blended(font, texte, white);
        texture_texte = SDL_CreateTextureFromSurface(renderer, surface_texte);
        rect_texte.w = surface_texte->w;
        rect_texte.h = surface_texte->h;
        SDL_RenderCopy(renderer, texture_texte, NULL, &rect_texte);
        rect_texte.y += surface_texte->h*1.2;
    }
}