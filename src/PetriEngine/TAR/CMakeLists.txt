


set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_library(TAR ${HEADER_FILES} TraceSet.cpp Solver.cpp TARReachability.cpp RangeContext.cpp RangeEvalContext.cpp)
target_link_libraries(TAR pardibaal PQL)
add_dependencies(TAR pardibaal-ext glpk-ext ptrie-ext)