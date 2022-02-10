set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Algorithm
CertainZeroFPA.cpp
FixedPointAlgorithm.cpp
LocalFPA.cpp
CZCycleDetectFPA.cpp
)

add_dependencies(Algorithm ptrie-ext glpk-ext)
