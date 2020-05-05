cmake_minimum_required(VERSION 2.8)
project(common_multichannelpowersupply_MultiPSSim)
find_package(chaos)
ADD_DEFINITIONS(-DDEBUG -fpermissive)
FILE(GLOB src *.cpp)
ADD_LIBRARY(${PROJECT_NAME} STATIC ${src} )
TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_debug chaos_common )
FILE(GLOB model_src *.h)
INSTALL(FILES ${model_src} DESTINATION include/common/MultiChannelPowerSupply/models/MultiPSSim)
#INSTALL_TARGETS(/lib ${PROJECT_NAME})
INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION "lib"
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)