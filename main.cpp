#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include <vlc/vlc.h>


/*Función para obtener la temperatura de la CPU */
/*float obtenerTemperaturaCPU() {
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
}*/

int main(int argc, char* argv[]) {
    // 1. Inicializar SDL2 para vídeo
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[ERROR] No se pudo inicializar SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Inicializar el subsistema de fuentes por si luego pintamos texto IPTV
    if (TTF_Init() < 0) {
        std::cerr << "[ADVERTENCIA] No se pudo inicializar SDL_ttf" << std::endl;
    }

    // 2. Crear una ventana SDL (ej: 1280x720)
    SDL_Window* ventana = SDL_CreateWindow(
        "IPTV Player - Raspberry Pi Zero 2W",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!ventana) {
        std::cerr << "[ERROR] No se pudo crear la ventana SDL: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // 3. Obtener el identificador nativo de la ventana e inicializar LibVLC
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    libvlc_media_player_t* vlc_player = nullptr;
    libvlc_instance_t* vlc_instancia = nullptr;

    if (SDL_GetWindowWMInfo(ventana, &wmInfo)) {
        vlc_instancia = libvlc_new(0, NULL);
        if (vlc_instancia) {
            // URL de streaming "dummy" de prueba (Big Buck Bunny)
            const char* url_stream = "https://www.w3schools.com/html/mov_bbb.mp4";
            
            libvlc_media_t* vlc_media = libvlc_media_new_location(vlc_instancia, url_stream);
            vlc_player = libvlc_media_player_new_from_media(vlc_media);
            libvlc_media_release(vlc_media);

            // Asignar la salida de vídeo de VLC a la ventana X11 de Linux
#if defined(SDL_VIDEO_DRIVER_X11)
            if (wmInfo.subsystem == SDL_SYSWM_X11) {
                libvlc_media_player_set_xwindow(vlc_player, (uint32_t)(uintptr_t)wmInfo.info.x11.window);
            }
#endif
            // Iniciar reproducción
            libvlc_media_player_play(vlc_player);
            std::cout << "=== Reproductor IPTV con VLC iniciado correctamente ===" << std::endl;
        }
    } else {
        std::cerr << "[ADVERTENCIA] No se pudo obtener la información de la ventana WM." << std::endl;
    }
}