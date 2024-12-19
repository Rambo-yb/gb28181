cmake_minimum_required(VERSION 2.8)
project(gb28181)

add_definitions(-DUSE_EASYLOGGER)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/example MAIN_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/general_src/common SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/general_src/list SRC)

#aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/third_src/cjson SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/third_src/easylogger SRC)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/general_src/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/general_src/list)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_src/cjson)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_src/easylogger)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/pjsip/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/pjsip/lib)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/libuuid/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/libuuid/lib)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/alsa/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/alsa/lib)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/ffmpeg/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/ffmpeg/lib)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/x264/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/x264/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/exosip/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/exosip/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/osip2/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/osip2/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/mxml/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/mxml/lib)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/live/include/liveMedia)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/live/include/groupsock)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/live/include/BasicUsageEnvironment)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/live/include/UsageEnvironment)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/live/lib)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/openssl/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/openssl/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/jthread/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/jthread/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/jrtplib/include/jrtplib3)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/_install/jrtplib/lib)

if (CHIP_TYPE STREQUAL "rv1126")
add_definitions(-DCHIP_TYPE_RV1126)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/example/rv1126/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/example/rv1126/lib)
endif()

add_library(gb28181 SHARED ${SRC})

add_executable(gb28181_test ${MAIN_SRC})
target_link_libraries(gb28181_test gb28181 pthread m dl resolv)
# target_link_libraries(gb28181_test g7221codec pjsua pjsua2 pjnath pjsip pjsip-simple pjsip-ua pjmedia-codec pjmedia-audiodev pjmedia-videodev pjmedia gsmcodec ilbccodec resample speex srtp pj pjlib-util)
# target_link_libraries(gb28181_test uuid asound mxml4 avcodec avdevice avfilter avformat avutil postproc swresample swscale x264)
target_link_libraries(gb28181_test eXosip2 osip2 osipparser2 mxml4)
# target_link_libraries(gb28181_test liveMedia groupsock BasicUsageEnvironment UsageEnvironment ssl crypto)
target_link_libraries(gb28181_test jrtp jthread)


if (CHIP_TYPE STREQUAL "rv1126")
target_link_libraries(gb28181_test easymedia RKAP_3A RKAP_ANR RKAP_Common rknn_runtime rknn_api md_share rkaiq VSC ArchModelSw NNArchPerf rockx rockface drm)
target_link_libraries(gb28181_test rockchip_mpp liveMedia groupsock BasicUsageEnvironment UsageEnvironment asound v4l2 v4lconvert rga sqlite3 od_share OpenVX GAL)
endif()

install(TARGETS gb28181_test DESTINATION bin)
install(TARGETS gb28181 DESTINATION lib)
#install(DIRECTORY include/ DESTINATION include)
