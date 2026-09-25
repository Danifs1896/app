cmake_minimum_required(VERSION 3.10)
project(IPTVMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar paquetes necesarios
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Crear ejecutable
add_executable(hola_mundo main.cpp)

# Enlazar las librerías utilizando tanto los objetivos como las variables estándar
target_link_libraries(hola_mundo PRIVATE 
    Threads::Threads 
    SDL2::SDL2 
    SDL2_ttf::SDL2_ttf
)

# Incluir los directorios de cabeceras
target_include_directories(hola_mundo PRIVATE 
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS}
)