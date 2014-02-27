project(netbridge)

add_executable(netbridge log.cpp routingmanager.cpp wpasupplicantinstance.cpp nl80211iface.cpp main.cpp libnetlink.c rt_names.c wpa_ctrl.c)
add_executable(testconnect testConnect.cpp nl80211iface.cpp log.cpp)

execute_process(COMMAND git log --pretty=format:%h -n 1 OUTPUT_VARIABLE GITREV)
file(WRITE "version.h" "#define VERSION \"0.1-${GITREV}\"")

target_link_libraries(testconnect nl pthread)
target_link_libraries(netbridge nl pthread)
set_target_properties(netbridge PROPERTIES COMPILE_FLAGS "-pthread" )
install(TARGETS netbridge RUNTIME DESTINATION bin)
