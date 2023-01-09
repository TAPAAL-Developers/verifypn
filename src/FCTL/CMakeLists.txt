set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Algorithm)
add_subdirectory(DependencyGraph)
add_subdirectory(PetriNets)
add_subdirectory(SearchStrategy)

add_library(FCTL ${HEADER_FILES}
FCTLEngine.cpp FCTLResult.cpp)

target_link_libraries(FCTL FeaturedAlgorithm FeaturedDependencyGraph FeaturedPetriNets FeaturedSearchStrategy)

