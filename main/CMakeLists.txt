idf_component_register(SRCS "tab5_main.cpp" "usb_hid.c" "audio.c" "cd_cue.c"
					"eth_connect.c" "font_8x16.c" "input.c" "tab5_input.cpp" "tab5_display.cpp"
                    INCLUDE_DIRS ".")

#hack: otherwise audio.c is not linked
#should actually factor refactor all things called by quake into a separate component
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u snd_inited -u CDAudio_Init")
