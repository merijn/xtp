#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/2aac7050/xtp_tools.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../../../tools/src/libtools -L../../src/libxtp/ -lvotca_xtp -lvotca_tools -lboost_program_options -lboost_serialization -lboost_filesystem -lboost_timer -lboost_system

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/xtp_app

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/xtp_app: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/xtp_app ${OBJECTFILES} ${LDLIBSOPTIONS} -fopenmp

${OBJECTDIR}/_ext/2aac7050/xtp_tools.o: nbproject/Makefile-${CND_CONF}.mk ../../src/tools/xtp_tools.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/2aac7050
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../include -I../../../tools/include -I../../../csg/include -I../../../xtp/include -I../../../ctp/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/2aac7050/xtp_tools.o ../../src/tools/xtp_tools.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/xtp_app

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
