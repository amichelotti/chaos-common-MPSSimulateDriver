cmake_minimum_required(VERSION 2.8)
project(common_multichannelpowersupply_CAEN2527)
find_package(chaos)

ADD_DEFINITIONS(-DDEBUG -fpermissive)
FILE(GLOB src *.cpp)
ADD_LIBRARY(${PROJECT_NAME} STATIC ${src} )
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/)
find_library(RESULT caenhvwrapper PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64)
#MESSAGE("FOUND ${RESULT}")
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/libcaenhvwrapper.so DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_debug chaos_common caenhvwrapper )

FILE(GLOB model_src *.h)
INSTALL(FILES ${model_src} DESTINATION include/common/MultiChannelPowerSupply/models/CAEN2527)
INSTALL(TARGETS ${PROJECT_NAME}
	DESTINATION "lib"
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

