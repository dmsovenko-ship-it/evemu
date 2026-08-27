#
# CMake build system file for EVEmu.
#
# Author: Bloody.Rabbit
#

##############
# Initialize #
##############
SET( TARGET_NAME        "eve-xmlpktgen" )
SET( TARGET_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/${TARGET_NAME}" )
SET( TARGET_SOURCE_DIR  "${PROJECT_SOURCE_DIR}/src/${TARGET_NAME}" )

#########
# Files #
#########
SET( INCLUDE
     "${TARGET_INCLUDE_DIR}/eve-xmlpktgen.h"
     "${TARGET_INCLUDE_DIR}/Generator.h"
     "${TARGET_INCLUDE_DIR}/CloneGenerator.h"
     "${TARGET_INCLUDE_DIR}/ConstructGenerator.h"
     "${TARGET_INCLUDE_DIR}/DecodeGenerator.h"
     "${TARGET_INCLUDE_DIR}/DestructGenerator.h"
     "${TARGET_INCLUDE_DIR}/DumpGenerator.h"
     "${TARGET_INCLUDE_DIR}/EncodeGenerator.h"
     "${TARGET_INCLUDE_DIR}/HeaderGenerator.h"
     "${TARGET_INCLUDE_DIR}/XMLPacketGen.h" )
SET( SOURCE
     "${TARGET_SOURCE_DIR}/eve-xmlpktgen.cpp"
     "${TARGET_SOURCE_DIR}/Generator.cpp"
     "${TARGET_SOURCE_DIR}/CloneGenerator.cpp"
     "${TARGET_SOURCE_DIR}/ConstructGenerator.cpp"
     "${TARGET_SOURCE_DIR}/DecodeGenerator.cpp"
     "${TARGET_SOURCE_DIR}/DestructGenerator.cpp"
     "${TARGET_SOURCE_DIR}/DumpGenerator.cpp"
     "${TARGET_SOURCE_DIR}/EncodeGenerator.cpp"
     "${TARGET_SOURCE_DIR}/HeaderGenerator.cpp"
     "${TARGET_SOURCE_DIR}/XMLPacketGen.cpp" )

########################
# Setup the executable #
########################
SOURCE_GROUP( "src" FILES ${INCLUDE} )
SOURCE_GROUP( "src"     FILES ${SOURCE} )

IF( NOT EVEMU_TARGETS_IMPORT )
  ADD_EXECUTABLE( "${TARGET_NAME}"
                  ${INCLUDE} ${SOURCE} )

  target_precompile_headers( "${TARGET_NAME}" PUBLIC
                  "${TARGET_INCLUDE_DIR}/eve-xmlpktgen.h" )
  TARGET_INCLUDE_DIRECTORIES( "${TARGET_NAME}"
                              ${eve-core_INCLUDE_DIRS}
                              "${TARGET_INCLUDE_DIR}" )
  TARGET_LINK_LIBRARIES( "${TARGET_NAME}"
                         "eve-core" )

  # eve-xmlpktgen is a build-time code generator only (it runs during the build
  # to emit packet headers). Instrumenting it with ASAN/UBSAN makes generation
  # orders of magnitude slower and can hang the build (observed: one packet file
  # taking >15 min). Strip sanitizer flags from just this target; the runtime
  # server (eve-server) keeps full sanitization.
  STRING(REPLACE "-fsanitize=address" "" _xmlpktgen_CFLAGS "${CMAKE_CXX_FLAGS}")
  STRING(REPLACE "-fsanitize=undefined" "" _xmlpktgen_CFLAGS "${_xmlpktgen_CFLAGS}")
  STRING(REPLACE "-fno-sanitize=alignment" "" _xmlpktgen_CFLAGS "${_xmlpktgen_CFLAGS}")
  IF(NOT "${_xmlpktgen_CFLAGS}" STREQUAL "${CMAKE_CXX_FLAGS}")
    SET_TARGET_PROPERTIES( "${TARGET_NAME}" PROPERTIES
                           COMPILE_FLAGS "${_xmlpktgen_CFLAGS}"
                           LINK_FLAGS "${_xmlpktgen_CFLAGS}" )
  ENDIF()

  IF( EVEMU_TARGETS_EXPORT )
    EXPORT( TARGETS "${TARGET_NAME}"
            FILE "${EVEMU_TARGETS_EXPORT}" )
  ENDIF( EVEMU_TARGETS_EXPORT )
ENDIF( NOT EVEMU_TARGETS_IMPORT )
