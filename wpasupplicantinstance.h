/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2013  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WPASUPPLICANTINSTANCE_H
#define WPASUPPLICANTINSTANCE_H
#include <string>
#include <unistd.h>
#include <pthread.h>
extern "C" {
#include "wpa_ctrl.h"
}
class WPASupplicantInstance
{
public:
    WPASupplicantInstance(std::string ifname, std::string ssid, std::string routing_table, std::string gateway, int mark);
    ~WPASupplicantInstance();
    void pollConnection();
private:
    void dhcp_thread();
    void keepalive_thread();
    std::string config;
    pid_t m_pid;
    struct wpa_ctrl * c;
    pthread_t dhcp_th;
    pthread_t keepalive_th;
    std::string m_ifname;
    bool dhcp_thread_run;
    std::string m_rtname;
    std::string m_gateway;
    int m_mark;
    pid_t m_dhcp_pid;
    bool isconnected;
    std::string m_bssid;
    std::string m_ssid;
    bool keepalive_run;
    
    
    pthread_cond_t keepalive_cond;
    pthread_mutex_t keepalive_mutex;
    pthread_mutex_t dhcp_pidname_mutex;
    std::string m_dhcp_pid_file;
};

#endif // WPASUPPLICANTINSTANCE_H
