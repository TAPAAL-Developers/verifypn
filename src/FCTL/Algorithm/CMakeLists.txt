set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(FeaturedAlgorithm
        FCertainZeroFPA.cpp
        FeatFixedPointAlgorithm.cpp
        )

add_dependencies(FeaturedAlgorithm ptrie-ext glpk-ext)
