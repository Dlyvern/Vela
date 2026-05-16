set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.hpp)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

add_library(VelaCore STATIC 
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

target_include_directories(VelaCore PUBLIC
        ${INCLUDE_DIR}
)

target_link_libraries(VelaCore
    PUBLIC 
    VelaUtility
    PRIVATE
    VelaBackend
)