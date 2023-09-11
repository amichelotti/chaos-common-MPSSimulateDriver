project(common_multichannelpowersupply_CAEN2527)
find_package(chaos)

ADD_DEFINITIONS(-DDEBUG -fpermissive)
FILE(GLOB src *.cpp)
ADD_LIBRARY(${PROJECT_NAME} STATIC ${src} )
if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
  MESSAGE("++ 64 bit architecture") 
  #set(LIBSUFFIX "64")
  SET(CAEN_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib64)
  

 else() 
   MESSAGE("++ 32 bit architecture")
   SET(CAEN_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)


endif()

LINK_DIRECTORIES(${CAEN_LIBDIR})
find_library(RESULT caenhvwrapper PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64)
#MESSAGE("FOUND ${RESULT}")
FILE(COPY ${CAEN_LIBDIR}/libcaenhvwrapper.so DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_debug chaos_common caenhvwrapper )

FILE(GLOB model_src *.h)
INSTALL(FILES ${model_src} DESTINATION include/common/MultiChannelPowerSupply/models/CAEN2527)
INSTALL(TARGETS ${PROJECT_NAME}
	DESTINATION "lib"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

