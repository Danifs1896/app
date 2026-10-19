#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vlc/vlc.h>

// Datos compartidos entre el hilo de VLC (decodifica) y el hilo principal (dibuja)
struct VideoContexto {
    std::mutex mtx;
    std::vector<uint8_t> buffer;
    unsigned ancho = 0;
    unsigned alto = 0;
    unsigned pitch = 0;
    bool nuevoFrame = false;
};

// --- Callbacks de LibVLC (se ejecutan en hilos de VLC) ---

// VLC nos pregunta en qué formato queremos los fotogramas
static unsigned vlc_formato(void** opaque, char* chroma, unsigned* ancho, unsigned* alto,
                            unsigned* pitches, unsigned* lines) {
    VideoContexto* ctx = static_cast<VideoContexto*>(*opaque);
    std::lock_guard<std::mutex> lock(ctx->mtx);

    memcpy(chroma, "RV32", 4);              // 32 bits por píxel
    ctx->ancho = *ancho;
    ctx->alto = *alto;
    ctx->pitch = *ancho * 4;
    pitches[0] = ctx->pitch;
    lines[0] = *alto;
    ctx->buffer.assign(static_cast<size_t>(ctx->pitch) * (*alto), 0);
    return 1;                               // un solo plano
}

// VLC va a escribir un fotograma: le damos nuestro buffer
static void* vlc_lock(void* opaque, void** planes) {
    VideoContexto* ctx = static_cast<VideoContexto*>(opaque);
    ctx->mtx.lock();
    planes[0] = ctx->buffer.data();
    return nullptr;
}

// VLC terminó de escribir el fotograma
static void vlc_unlock(void* opaque, void* /*picture*/, void* const* /*planes*/) {
    VideoContexto* ctx = static_cast<VideoContexto*>(opaque);
    ctx->nuevoFrame = true;
    ctx->mtx.unlock();
}

// VLC pide mostrar el fotograma: lo dibuja el hilo principal, aquí no hacemos nada
static void vlc_display(void* /*opaque*/, void* /*picture*/) {}

int main(int argc, char* argv[]) {
    // Si no se indica driver por entorno, usar KMSDRM
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "kmsdrm");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[ERROR] No se pudo inicializar SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "Driver de vídeo: " << SDL_GetCurrentVideoDriver() << std::endl;

    if (TTF_Init() < 0) {
        std::cerr << "[ADVERTENCIA] No se pudo inicializar SDL_ttf" << std::endl;
    }

    // En KMSDRM la ventana ocupa siempre toda la pantalla
    SDL_Window* ventana = SDL_CreateWindow(
        "IPTV Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        0, 0,
        SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    if (!ventana) {
        std::cerr << "[ERROR] No se pudo crear la ventana SDL: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* render = SDL_CreateRenderer(
        ventana, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!render) {
        std::cerr << "[ERROR] No se pudo crear el renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(ventana);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_ShowCursor(SDL_DISABLE);

    // LibVLC sin salida de vídeo propia: nos entrega los fotogramas
    VideoContexto ctx;
    const char* vlc_args[] = { "--no-xlib", "--quiet" };
    libvlc_instance_t* vlc_instancia = libvlc_new(2, vlc_args);
    libvlc_media_player_t* vlc_player = nullptr;

    if (vlc_instancia) {
        const char* url_stream = "https://www.w3schools.com/html/mov_bbb.mp4";
        libvlc_media_t* media = libvlc_media_new_location(vlc_instancia, url_stream);
        vlc_player = libvlc_media_player_new_from_media(media);
        libvlc_media_release(media);

        libvlc_video_set_callbacks(vlc_player, vlc_lock, vlc_unlock, vlc_display, &ctx);
        libvlc_video_set_format_callbacks(vlc_player, vlc_formato, nullptr);
        libvlc_media_player_play(vlc_player);
        std::cout << "=== Reproductor IPTV con VLC iniciado ===" << std::endl;
    } else {
        std::cerr << "[ERROR] No se pudo iniciar LibVLC" << std::endl;
    }

    SDL_Texture* textura = nullptr;
    unsigned texAncho = 0, texAlto = 0;
    bool ejecutando = true;
    SDL_Event evento;

    while (ejecutando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT ||
                (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE)) {
                ejecutando = false;
            }
        }

        // Copiar el último fotograma de VLC a la textura
        {
            std::lock_guard<std::mutex> lock(ctx.mtx);
            if (ctx.ancho > 0 && ctx.alto > 0) {
                if (!textura || texAncho != ctx.ancho || texAlto != ctx.alto) {
                    if (textura) SDL_DestroyTexture(textura);
                    textura = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                ctx.ancho, ctx.alto);
                    texAncho = ctx.ancho;
                    texAlto = ctx.alto;
                }
                if (textura && ctx.nuevoFrame) {
                    SDL_UpdateTexture(textura, nullptr, ctx.buffer.data(), ctx.pitch);
                    ctx.nuevoFrame = false;
                }
            }
        }

        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        SDL_RenderClear(render);
        if (textura) SDL_RenderCopy(render, textura, nullptr, nullptr);
        // Aquí se podría pintar texto con SDL_ttf encima del vídeo
        SDL_RenderPresent(render);
    }

    // Limpieza
    if (vlc_player) {
        libvlc_media_player_stop(vlc_player);
        libvlc_media_player_release(vlc_player);
    }
    if (vlc_instancia) libvlc_release(vlc_instancia);

    if (textura) SDL_DestroyTexture(textura);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(ventana);
    TTF_Quit();
    SDL_Quit();
    return 0;
}