# ===================================================================
# CMAKE のおまじない
# ===================================================================
cmake_minimum_required (VERSION 3.0)


# ===================================================================
# プロジェクト名，バージョンの設定
# ===================================================================
project (YmApps)
set (YMAPPS_VERSION_MAJOR 1)
set (YMAPPS_VERSION_MINOR 0)
set (YMAPPS_VERSION "${YMAPPS_VERSION_MAJOR}.${YMAPPS_VERSION_MINOR}")

# リリースモード
#set (CMAKE_BUILD_TYPE RELEASE)

# デバッグモード
#set (CMAKE_BUILD_TYPE DEBUG)


# ===================================================================
# オプション
# ===================================================================

option (OPT_GPERFTOOLS "use google-perftools" OFF)

option (BUILD_IGF      "build igf" ON)
option (BUILD_BNET2AIG "build bnet2aig" ON)
option (BUILD_MAKEBDD  "build makebdd" ON)


# ===================================================================
# CTest モジュールの読み込み
# ===================================================================
include (CTest)


# ===================================================================
# パッケージの検査
# ===================================================================
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include (FindDoxygen)

find_package(BISON REQUIRED)

include (FindPythonInterp)
include (FindPythonLibs)

include (FindQt4)
find_package(Qt4 4.8.0 REQUIRED QtCore QtGui QtXml)
#include(${QT_USE_FILE})

set( CMAKE_INCLUDE_CURRENT_DIR ON )
find_package(Qt5Widgets)
find_package(OpenGL)

find_package(TCL)

if ( OPT_GPERFTOOLS )
  find_package(Gperftools)
endif (OPT_GPERFTOOLS)

find_package (GTest)

#include (FindGMP)

include (FindPOPT)

include (FindReadline)

find_package(YmTools REQUIRED)


# ===================================================================
# サブディレクトリの設定
# ===================================================================

if (BUILD_BNET2AIG)
  add_subdirectory (programs/bnet2aig)
endif (BUILD_BNET2AIG)

if (BUILD_IGF)
  add_subdirectory (programs/igf)
endif (BUILD_IGF)

if (BUILD_MAKEBDD)
  add_subdirectory (programs/makebdd)
endif (BUILD_MAKEBDD)



# ===================================================================
# doxygen 起動用のコマンド
# ===================================================================

if (DOXYGEN_FOUND)
  configure_file (
    "${PROJECT_SOURCE_DIR}/etc/Doxyfile.in"
    "${PROJECT_BINARY_DIR}/Doxyfile"
    )

  add_custom_target(dox
    "${DOXYGEN_EXECUTABLE}" ">" "${PROJECT_BINARY_DIR}/doxygen.log"
    DEPENDS "${PROJECT_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "generating doxygen documents"
    )
endif (DOXYGEN_FOUND)


# ===================================================================
# インストールターゲット
# ===================================================================
