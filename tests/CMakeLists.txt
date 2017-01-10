find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )

add_executable(MyTests
    module.cpp
    chamfer_fillet_test.cpp
    collision_test.cpp
    polygon_iterator.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/polygon
    ${CMAKE_SOURCE_DIR}/common/geometry
)

target_link_libraries(MyTests
    ${CMAKE_BINARY_DIR}/polygon/libpolygon.a
    ${CMAKE_BINARY_DIR}/common/libcommon.a
    ${CMAKE_BINARY_DIR}/bitmaps_png/libbitmaps.a
    ${CMAKE_BINARY_DIR}/polygon/libpolygon.a
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${wxWidgets_LIBRARIES}
)
