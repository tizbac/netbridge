project(netbridge)

add_executable(netbridge routingmanager.cpp wpasupplicantinstance.cpp nl80211iface.cpp main.cpp libnetlink.c rt_names.c wpa_ctrl.c)
add_executable(testconnect testConnect.cpp nl80211iface.cpp)

target_link_libraries(testconnect nl pthread)
target_link_libraries(netbridge nl pthread)
set_target_properties(netbridge PROPERTIES COMPILE_FLAGS "-pthread" )
install(TARGETS netbridge RUNTIME DESTINATION bin)
