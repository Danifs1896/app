#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

float obtenerTemperaturaCPU() {
    std::ifstream archivo("/sys/class/thermal/thermal_zone0/temp");
    if (!archivo.is_open()) return -1.0f;
    std::string linea;
    std::getline(archivo, linea);
    archivo.close();
    try {
        return std::stol(linea) / 1000.0f;
    } catch (...) {
        return -1.0f;
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    if (TTF_Init() < 0) {
        SDL_Quit();
        return 1;
    }

    SDL_Window* ventana = SDL_CreateWindow(
        "Monitor de Temperatura IPTV",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN
    );
    if (!ventana) { TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* renderizador = SDL_CreateRenderer(ventana, -1, SDL_RENDERER_ACCELERATED);

    // Intentamos cargar una fuente estándar que viene en Raspberry Pi OS
    TTF_Font* fuente = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 48);
    if (!fuente) {
        // Si no está esa, probamos con otra ruta común
        fuente = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 48);
    }

    bool ejecutando = true;
    SDL_Event evento;

    while (ejecutando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT || (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE)) {
                ejecutando = false;
            }
        }

        float temp = obtenerTemperaturaCPU();

        // Color de fondo (Rojo si > 60ºC, Azul oscuro si normal)
        if (temp > 60.0f) {
            SDL_SetRenderDrawColor(renderizador, 150, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderizador, 15, 23, 42, 255);
        }
        SDL_RenderClear(renderizador);

        // Renderizar el texto de la temperatura si la fuente cargó bien
        if (fuente) {
            std::string textoTemp = "CPU Temp: " + (temp > 0 ? std::to_string(temp).substr(0, 5) + " C" : "Error");
            SDL_Color colorBlanco = {255, 255, 255, 255};
            
            SDL_Surface* superficieTexto = TTF_RenderText_Blended(fuente, textoTemp.c_str(), colorBlanco);
            if (superficieTexto) {
                SDL_Texture* texturaTexto = SDL_CreateTextureFromSurface(renderizador, superficieTexto);
                
                // Posición y tamaño del texto en el centro de la pantalla
                SDL_Rect rectDestino = { 200, 250, superficieTexto->w, superficieTexto->h };
                
                SDL_RenderCopy(renderizador, texturaTexto, NULL, &rectDestino);
                
                SDL_DestroyTexture(texturaTexto);
                SDL_FreeSurface(superficieTexto);
            }
        }

        SDL_RenderPresent(renderizador);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    if (fuente) TTF_CloseFont(fuente);
    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(ventana);
    TTF_Quit();
    SDL_Quit();

    return 0;
}