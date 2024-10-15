cmake_minimum_required (VERSION 3.20)
project( "Interval" )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(LavaCake)    

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/envmaps DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/objects DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)



file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/crystal)
clearShader()
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/crystal/interval.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/crystal/interval.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/crystal/mesh.mesh ${CMAKE_CURRENT_BINARY_DIR}/shaders/crystal/mesh.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/crystal/postProcess.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/crystal/postProcess.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/crystal/postProcess.vert ${CMAKE_CURRENT_BINARY_DIR}/shaders/crystal/postProcess.vert.spv spv1.4)
add_executable( crystal  "./src/crystal.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
AutoSPIRV(crystal)
target_link_libraries( crystal LavaCake )




file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/singleTet)
clearShader()
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/singleTet/interval.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/singleTet/interval.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/singleTet/mesh.mesh ${CMAKE_CURRENT_BINARY_DIR}/shaders/singleTet/mesh.mesh.spv spv1.4)

add_compile_definitions(PROJECT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/")
add_executable( singleTet  "./src/singleTet.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
AutoSPIRV(singleTet)
target_link_libraries( singleTet LavaCake )





file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/armadillo)
clearShader()
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/armadillo/interval.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/armadillo/interval.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/armadillo/mesh.mesh ${CMAKE_CURRENT_BINARY_DIR}/shaders/armadillo/mesh.mesh.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/armadillo/postProcess.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/armadillo/postProcess.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/armadillo/postProcess.vert ${CMAKE_CURRENT_BINARY_DIR}/shaders/armadillo/postProcess.vert.spv spv1.4)
add_executable( armadillo  "./src/armadillo.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
AutoSPIRV(armadillo)
target_link_libraries( armadillo LavaCake )





file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/bunny)
clearShader()
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/bunny/interval.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/bunny/interval.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/bunny/mesh.mesh ${CMAKE_CURRENT_BINARY_DIR}/shaders/bunny/mesh.mesh.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/bunny/postProcess.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/bunny/postProcess.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/bunny/postProcess.vert ${CMAKE_CURRENT_BINARY_DIR}/shaders/bunny/postProcess.vert.spv spv1.4)
add_executable( bunny  "./src/bunny.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
AutoSPIRV(bunny)
target_link_libraries( bunny LavaCake )




file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/asteroid)
clearShader()
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/asteroid/interval.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/asteroid/interval.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/asteroid/mesh.mesh ${CMAKE_CURRENT_BINARY_DIR}/shaders/asteroid/mesh.mesh.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/asteroid/model.comp ${CMAKE_CURRENT_BINARY_DIR}/shaders/asteroid/model.comp.spv spv1.4)
add_executable( asteroids  "./src/asteroids.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
AutoSPIRV(asteroids)
target_link_libraries( asteroids LavaCake )


file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders/jet)
clearShader()
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/jet/interval.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/jet/interval.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/jet/mesh.mesh ${CMAKE_CURRENT_BINARY_DIR}/shaders/jet/mesh.mesh.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/jet/postProcess.frag ${CMAKE_CURRENT_BINARY_DIR}/shaders/jet/postProcess.frag.spv spv1.4)
addShader(${CMAKE_CURRENT_SOURCE_DIR}/src/shader/jet/postProcess.vert ${CMAKE_CURRENT_BINARY_DIR}/shaders/jet/postProcess.vert.spv spv1.4)
add_executable( jet  "./src/jet.cpp")
set( CMAKE_CXX_FLAGS_DEBUG_INIT "-fstack-protector" )
AutoSPIRV(jet)
target_link_libraries( jet LavaCake )

