configure_file(LinphoneCxxConfig.cmake.in LinphoneCxxConfig.cmake @ONLY)

add_custom_command(OUTPUT include/linphone++/linphone.hh src/linphone++.cc
	COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/genwrapper.py" "${PROJECT_BINARY_DIR}/coreapi/help/doc/xml"
	DEPENDS abstractapi.py genwrapper.py class_header.mustache class_impl.mustache enums_header.mustache main_header.mustache linphone-doc
	"${PROJECT_BINARY_DIR}/coreapi/help/doc/xml/index.xml"
)

add_library(linphone++ SHARED
	object.cc
	tools.cc
	${CMAKE_CURRENT_BINARY_DIR}/src/linphone++.cc
)
target_link_libraries(linphone++
	PRIVATE ${BCTOOLBOX_CORE_LIBRARIES} ${BELLESIP_LIBRARIES} linphone
)
target_include_directories(linphone++
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${BCTOOLBOX_INCLUDE_DIRS}
	PRIVATE ${BELLESIP_INCLUDE_DIRS}
)
set_target_properties(linphone++ PROPERTIES SOVERSION ${LINPHONE_SO_VERSION})

install(TARGETS linphone++ EXPORT LinphoneCxxTargets
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES object.hh
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/linphone++
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/linphone++
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/LinphoneCxxVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)
export(EXPORT LinphoneCxxTargets
	FILE "${CMAKE_CURRENT_BINARY_DIR}/LinphoneCxxTargets.cmake"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LinphoneCxxVersion.cmake
	${CMAKE_CURRENT_BINARY_DIR}/LinphoneCxxTargets.cmake
	${CMAKE_CURRENT_BINARY_DIR}/LinphoneCxxConfig.cmake
	DESTINATION ${CMAKE_INSTALL_DATADIR}/LinphoneCxx/cmake
)
