cmake_minimum_required(VERSION 2.8)
project(CaenOwnerLib)
set (lib_src
	CAENHVWrapper.c
	a290cinterface.c
	defcomandi.c
	libutil.c
	n470interface.c
	n568interface.c
	sy127interface.c
	sy1527interface.c
	sy403interface.c
	sy527interface.c
	transport.c
	HSCAENETLib.c
	a303.c )
if (WIN32)
ELSE(WIN32)
ADD_DEFINITIONS(-DUNIX -DLINUX)
ENDIF(WIN32)
ADD_LIBRARY(${PROJECT_NAME}  STATIC ${lib_src} )
