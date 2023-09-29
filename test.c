// #include <stdio.h>
// #include <stdlib.h>
// #include <SDL2/SDL.h>



// #define width 1000
// #define height 1000
// SDL_Renderer* renderer;
// SDL_Window* window;


// void initSDL()
// /*initialise SDL*/
// {
//     SDL_Init(SDL_INIT_VIDEO);
//     SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
//     // SDL_Surface* image = NULL;
//     // SDL_Texture* texture = NULL; 
//     SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
// }

// void drawCircle(int X, int Y, int radius) 
// {
//     for (int x = X - radius; x <= X + radius; x++) {
//         for (int y = Y - radius; y <= Y + radius; y++) {
//             if (pow(x - X, 2) + pow(y - Y, 2) <= pow(radius, 2)) {
//                 SDL_RenderDrawPoint(renderer, x, y);
//             }
//         }
//     }
// }

// void draw_rect(int x, int y, int xwidth, int yheight)
// {
//     SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
//     SDL_RenderDrawLine(renderer,x,y,x+xwidth,y);
//     SDL_RenderDrawLine(renderer,x,y,x,y+yheight);
//     SDL_RenderDrawLine(renderer,x+xwidth,y,x+xwidth,y+yheight);
//     SDL_RenderDrawLine(renderer,x,y+yheight,x+xwidth,y+yheight);
// }

// void clear_grid()
// {
//     SDL_Rect square;
//     SDL_SetRenderDrawColor(renderer,40,40,40,SDL_ALPHA_OPAQUE);
//     square.x = 0;
//     square.y = 0;
//     square.h = height;
//     square.w = width;
//     SDL_RenderFillRect(renderer,&square);
// }

// void draw_grid()
// {
//     clear_grid();
//     int number_of_drawn_lines = 20;
//     SDL_SetRenderDrawColor(renderer,255,255,255,SDL_ALPHA_TRANSPARENT);
//     for (int i = 1; i<number_of_drawn_lines ;i++) {
//         SDL_RenderDrawLine(renderer,(height/40)+(((height-(height/20))/number_of_drawn_lines)*i),(width/40),((height/40)+(((height-(height/20))/number_of_drawn_lines)*i)),width-(width/40));
//         SDL_RenderDrawLine(renderer,(height/40),(width/40)+(((width-(width/20))/number_of_drawn_lines)*i),height-(height/40),(width/40)+(((width-(width/20))/number_of_drawn_lines)*i));
//     }


//     draw_rect((width/40),(height/40),width-(width/40)*2,height-(height/40)*2);
// }


// double lagrangePolynomial(double x, double x0, double y0, double x1, double y1) {
//     double term1 = ((x - x1) * y0) / (x0 - x1);
//     double term2 = ((x - x0) * y1) / (x1 - x0);
//     return term1 + term2;
// }


// int main()
// {
//     int running = 1, mousex, mousey;
//     SDL_Event Event;
//     initSDL();
//     draw_grid();
//     SDL_RenderPresent(renderer);
//     while(running) {
//         while (SDL_PollEvent(&Event)) {
//             switch (Event.type) {
//                 case SDL_QUIT:
//                     running = 0;
//                     break;
//                 case SDL_MOUSEBUTTONDOWN:
//                     SDL_GetMouseState(&mousex,&mousey);
//                     SDL_SetRenderDrawColor(renderer,255,255,0,SDL_ALPHA_OPAQUE);
//                     drawCircle(mousex,mousey,3);
//                     SDL_RenderPresent(renderer);

//             }
//         }

//     }
//     SDL_DestroyRenderer(renderer);
//     SDL_DestroyWindow(window);
//     SDL_Quit();
//     return 0;

// }



#include <stdio.h>
#include <math.h>

int main() {
    // Créez un fichier de données
    FILE *dataFile = fopen("data.txt", "w");
    if (!dataFile) {
        printf("Erreur lors de la création du fichier de données.\n");
        return 1;
    }

    // Écrivez les données dans le fichier
    for (double x = -20.0; x <= 20.0; x += 0.1) {
        double y = (x);
        fprintf(dataFile, "%lf %lf\n", x, y);
    }

    fclose(dataFile);

    // Utilisez Gnuplot pour tracer le graphique
    FILE *gnuplotPipe = popen("gnuplot -persist", "w");
    if (!gnuplotPipe) {
        printf("Erreur lors de l'exécution de Gnuplot.\n");
        return 1;
    }

    fprintf(gnuplotPipe, "plot 'data.txt' with lines\n");
    fprintf(gnuplotPipe, "pause -1\n"); // Maintient le graphique ouvert

    pclose(gnuplotPipe);

    return 0;
}