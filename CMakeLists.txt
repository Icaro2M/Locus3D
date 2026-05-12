cmake_minimum_required(VERSION 3.10)
project(Locus3D)

# 1. Definir o padrão C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Configurar caminhos das Bibliotecas (Vendor)
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad)
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw)
set(GLM_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)
set(NLOHMANN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann)
set(STB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb)

# 3. Coletar arquivos de código (Sources)
# Aqui incluímos os códigos-fonte do projeto e das bibliotecas
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "${GLAD_DIR}/src/glad.c"
    "${IMGUI_DIR}/*.cpp"
)

# 4. Criar o executável
add_executable(${PROJECT_NAME} ${SOURCES})

# 5. Configurar Diretórios de Inclusão (Headers)
target_include_directories(${PROJECT_NAME} PRIVATE 
    "src"
    "${GLAD_DIR}/include"
    "${GLFW_DIR}/include"
    "${GLM_DIR}/include"
    "${IMGUI_DIR}"
    "${NLOHMANN_DIR}"
    "${STB_DIR}"
)

# 6. Linkar Bibliotecas Binárias
# Para o GLFW, apontamos diretamente para o seu arquivo .lib
# Comdlg32.lib é usado pelo explorador nativo de arquivos do Windows
target_link_libraries(${PROJECT_NAME} PRIVATE 
    "${GLFW_DIR}/lib/glfw3.lib"
    opengl32.lib
    Comdlg32.lib
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

# 7. Definir Macros (Necessário para ImGui e GLFW no Windows)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)