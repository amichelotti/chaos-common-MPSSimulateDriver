cmake_minimum_required(VERSION 2.6)
add_subdirectory(MultiPSSim)
add_subdirectory(CAEN2527)
SET(DRIVERS_LIB ${DRIVERS_LIB} PARENT_SCOPE)
