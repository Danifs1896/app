#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

// Función para leer la temperatura del procesador de la Raspberry Pi
float obtenerTemperaturaCPU() {
    std::ifstream archivo("/sys/class/thermal/thermal_zone0/temp");
    
    if (!archivo.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir el sensor de temperatura." << std::endl;
        return -1.0f;
    }

    std::string linea;
    std::getline(archivo, linea);
    archivo.close();

    try {
        // Convierte el texto leído a número y lo pasa a grados Celsius
        long miligrados = std::stol(linea);
        return miligrados / 1000.0f; 
    } catch (...) {
        return -1.0f;
    }
}

int main() {
    std::cout << "=== Monitor de Temperatura de la Raspberry Pi Zero 2W ===" << std::endl;
    std::cout << "Presiona Ctrl + C para salir.\n" << std::endl;

    // Bucle para leer la temperatura continuamente cada 2 segundos
    while (true) {
        float temp = obtenerTemperaturaCPU();

        if (temp > 0) {
            std::cout << "Temperatura actual de la CPU: " << temp << " °C" << std::endl;
        } else {
            std::cout << "No se pudo leer la temperatura." << std::endl;
        }

        // Esperar 2 segundos antes de la siguiente lectura
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}