configure_file(c_make_lists.mustache.in c_make_lists.mustache @ONLY)
configure_file(object.cc src/object.cc COPYONLY)
configure_file(object.hh include/object.hh COPYONLY)

add_custom_command(OUTPUT CMakeLists.txt include/linphone.hh
	COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/genwrapper.py" "${PROJECT_BINARY_DIR}/coreapi/help/doc/xml"
	DEPENDS *.py *.mustache linphone-doc
	"${PROJECT_BINARY_DIR}/coreapi/help/doc/xml/index.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/c_make_lists.mustache"
)
add_custom_target(make_cpp_wrapper ALL ${CMAKE_COMMAND} -E make_directory build/
	COMMAND ${CMAKE_COMMAND} -E chdir build/ ${CMAKE_COMMAND} .. -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_INSTALL_RPATH="${CMAKE_INSTALL_RPATH}" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	COMMAND ${CMAKE_COMMAND} --build build/
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt
	${CMAKE_CURRENT_BINARY_DIR}/include/linphone.hh
)
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/build --target install)")
