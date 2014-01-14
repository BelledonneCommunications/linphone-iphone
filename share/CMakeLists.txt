install(FILES archived-rootca.pem
		RENAME rootca.pem
		COMPONENT COMP_liblinphone
        DESTINATION share/linphone
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install(FILES ringback.wav
		COMPONENT COMP_liblinphone
        DESTINATION share/sounds/linphone
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

add_subdirectory(rings)

install(FILES ../mediastreamer2/src/voip/nowebcamCIF.jpg
		COMPONENT COMP_liblinphone
        DESTINATION share/images
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
