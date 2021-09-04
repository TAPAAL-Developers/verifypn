set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Simplification LinearProgram.cpp)
add_dependencies(Simplification glpk-ext)
target_link_libraries(Simplification glpk)
