set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(PQLExpression Expressions.cpp PQL.cpp Contexts.cpp QueryPrinter.cpp CTLVisitor.cpp)
add_dependencies(PQLExpression glpk-ext)
target_link_libraries(PQLExpression Simplification glpk)

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.5 REQUIRED)

flex_target(pql_lexer "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryTokens.l"
                      "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryTokens.lexer.cpp")

bison_target(pql_parser "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryParser.y"
                        "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryParser.parser.cpp")

add_flex_bison_dependency(pql_lexer pql_parser)

add_library(PQL OBJECT ${BISON_pql_parser_OUTPUTS} ${FLEX_pql_lexer_OUTPUTS})
set_target_properties(PQL PROPERTIES CXX_CLANG_TIDY "") # we do this trick to avoid tidy on the generated files
target_link_libraries(PQL PQLExpression)

