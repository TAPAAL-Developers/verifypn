set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(FeaturedPN
        PetriNetSerializer.cpp
        EnumeratingPetriNetBuilder.cpp
)

add_dependencies(FeaturedPN ptrie-ext glpk-ext)
