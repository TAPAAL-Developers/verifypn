set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLStubborn EvalAndSetVisitor.cpp VisibleTransitionVisitor.cpp VisibleLTLStubbornSet.cpp AutomatonStubbornSet.cpp SafeAutStubbornSet.cpp)
add_dependencies(LTLStubborn PetriEngine)
