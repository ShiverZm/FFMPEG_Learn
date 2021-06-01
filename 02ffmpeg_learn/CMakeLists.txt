project (ffmpeg_play_h264)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

#include header_files
SET(SDL_INCLUDE ${PROJECT_SOURCE_DIR}/../lib/SDL2-2.0.9/)
SET(EXTERN_INCLUDE ${PROJECT_SOURCE_DIR}/../include/)
SET(FFMPEG_INCLUDE ${PROJECT_SOURCE_DIR}/../lib/ffmpeg-win64-gpl-shared-4.4/include)

INCLUDE_DIRECTORIES (${SDL_INCLUDE} ${EXTERN_INCLUDE} ${SDL_TTF_INCLUDE} ${FFMPEG_INCLUDE})

#lib 

SET(FFMPGE_LIB ${PROJECT_SOURCE_DIR}/../lib/ffmpeg-win64-gpl-shared-4.4/lib)

IF(WIN32)
	SET(SDL_LIB ${PROJECT_SOURCE_DIR}/../lib/SDL2-2.0.9/lib/x86 ${PROJECT_SOURCE_DIR}/../lib/SDL2_ttf-devel-2.0.15-VC/SDL2_ttf-2.0.15/lib/x86)
ELSE()
	SET(SDL_LIB ${PROJECT_SOURCE_DIR}/../lib/SDL2-2.0.9/lib/x64  ${PROJECT_SOURCE_DIR}/../lib/SDL2_ttf-devel-2.0.15-VC/SDL2_ttf-2.0.15/lib/x64)
ENDIF(WIN32)
	
LINK_DIRECTORIES(${SDL_LIB} ${FFMPGE_LIB})

#SDL2
LINK_LIBRARIES(SDL2 SDL2main SDL2test SDL2_image SDL2_ttf avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

#extension lib SDL_IMAGE
#LINK_LIBRARIES(SDL2_image SDL2_ttf)

FILE (GLOB header_files *.h)
FILE (GLOB source_files *.c *.cpp)

ADD_EXECUTABLE (ffmpeg_play_h264 ${header_files} ${source_files})

TARGET_LINK_LIBRARIES (ffmpeg_play_h264)