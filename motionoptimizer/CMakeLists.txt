set(LINK_LIBS)

add_subdirectory( utils )
include_directories( utils )

add_subdirectory( containers )
include_directories( containers )

add_subdirectory( optimizer )
include_directories( optimizer )

add_library(motionoptimizer ${LIBRARY_TYPE} MotionOptimizer.cpp)
target_link_libraries( motionoptimizer optimizer )


