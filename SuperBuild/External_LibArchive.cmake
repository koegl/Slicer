
set(proj LibArchive)

# Set dependency list
set(${proj}_DEPENDENCIES "zlib")
if(WIN32)
  list(APPEND ${proj}_DEPENDENCIES zlib)
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(Slicer_USE_SYSTEM_${proj} AND (WIN32 OR APPLE))
  message(FATAL_ERROR "Enabling Slicer_USE_SYSTEM_${proj} is not supported !")
endif()

if(Slicer_USE_SYSTEM_${proj})
  unset(LibArchive_INCLUDE_DIR CACHE)
  unset(LibArchive_LIBRARY CACHE)
  find_package(LibArchive REQUIRED MODULE)
endif()

if((NOT DEFINED LibArchive_INCLUDE_DIR
   OR NOT DEFINED LibArchive_LIBRARY) AND NOT Slicer_USE_SYSTEM_${proj})

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/Slicer/libarchive.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_TAG
    "30a9657b6c12f675fd5057cea017b2c9bd924dda" # slicer-v3.8.1-2025-06-01-9525f90ca
    QUIET
    )

  # When updating the version of LibArchive, consider also
  # updating the soversion number hard-coded below in the
  # "fix_rpath" macOS external project step.
  #
  # To find out if the soversion number should be updated,
  # review the logic computing INTERFACE_VERSION in the
  # top-level LibArchive CMakeLists.txt and/or inspect the
  # prefix associated with the library generated in the build
  # tree.

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(EP_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${Slicer_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${Slicer_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    INSTALL_DIR ${EP_INSTALL_DIR}
    CMAKE_CACHE_ARGS
    # Not used -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    # Not used -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_TESTING:BOOL=OFF
      -DENABLE_ACL:BOOL=OFF
      -DENABLE_BZip2:BOOL=OFF
      -DENABLE_CAT:BOOL=OFF
      -DENABLE_CNG:BOOL=OFF
      -DENABLE_CPIO:BOOL=OFF
      -DENABLE_EXPAT:BOOL=OFF
      -DENABLE_ICONV:BOOL=OFF
      -DENABLE_LIBB2:BOOL=OFF
      -DENABLE_LibGCC:BOOL=OFF
      -DENABLE_LIBXML2:BOOL=OFF
      -DENABLE_LZ4:BOOL=OFF
      -DENABLE_LZMA:BOOL=OFF
      -DENABLE_LZO:BOOL=OFF
      -DENABLE_MBEDTLS:BOOL=OFF
      -DENABLE_NETTLE:BOOL=OFF
      -DENABLE_OPENSSL:BOOL=OFF
      -DENABLE_PCREPOSIX:BOOL=OFF
      -DENABLE_TAR:BOOL=OFF
      -DENABLE_TEST:BOOL=OFF
      -DENABLE_XATTR:BOOL=OFF
      -DENABLE_ZSTD:BOOL=OFF
      -DARCHIVE_CRYPTO_MD5_LIBSYSTEM:BOOL=OFF
      -DZLIB_ROOT:PATH=${ZLIB_ROOT}
      -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
      -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY}
      # Install directories
      -DCMAKE_INSTALL_LIBDIR:STRING=lib  # Override value set in GNUInstallDirs CMake module
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )
  if(APPLE)
    ExternalProject_Add_Step(${proj} fix_rpath
      COMMAND install_name_tool -id ${EP_INSTALL_DIR}/lib/libarchive.13.dylib ${EP_INSTALL_DIR}/lib/libarchive.13.dylib
      DEPENDEES install
      )
  endif()

  ExternalProject_GenerateProjectDescription_Step(${proj})

  set(LibArchive_DIR ${CMAKE_BINARY_DIR}/LibArchive-install)

  set(LibArchive_INCLUDE_DIR ${LibArchive_DIR}/include)
  if(WIN32)
    set(LibArchive_LIBRARY ${LibArchive_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}archive.lib)
  else()
    set(LibArchive_LIBRARY ${LibArchive_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}archive${CMAKE_SHARED_LIBRARY_SUFFIX})
  endif()

  set(_lib_subdir lib)
  if(WIN32)
    set(_lib_subdir bin)
  endif()

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD ${LibArchive_DIR}/${_lib_subdir})
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(LibArchive_DIR:PATH)

mark_as_superbuild(
  VARS
    LibArchive_INCLUDE_DIR:PATH
    LibArchive_LIBRARY:FILEPATH
  LABELS "FIND_PACKAGE"
  )

if(Slicer_USE_SYSTEM_${proj})
  ExternalProject_Message(${proj} "LibArchive_INCLUDE_DIR:${LibArchive_INCLUDE_DIR}")
  ExternalProject_Message(${proj} "LibArchive_LIBRARY:${LibArchive_LIBRARY}")
endif()
