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

#include "wpasupplicantinstance.h"
#include "nl80211iface.h"
#include "routingmanager.h"
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <fstream>
#include <stdio.h>
void WPASupplicantInstance::dhcp_thread()
{
    std::cout << "Thread DHCP avviato" << std::endl;
    NL80211Iface iface(m_ifname);
    RoutingManager rmgr;
    while ( dhcp_thread_run )
    {
        if ( iface.isConnected() )
            break;
        std::cout << "Attendo la connessione..." << std::endl;
        usleep(500000);
    }
    if ( !dhcp_thread_run )
        return;
    std::stringstream ss;
    ss << "/var/run/netbridge/dhclient_pid_" << m_ifname << ".pid";
    m_dhcp_pid = fork();
    if ( m_dhcp_pid == 0 )
    {
        execl("/usr/sbin/dhclient","dhclient",m_ifname.c_str(),"-pf",ss.str().c_str());
        exit(0);
    }else{
        while ( dhcp_thread_run )
        {
            int fd;
            
            struct ifreq ifr;
            memset(&ifr,sizeof(ifr),0);
            fd = socket(AF_INET, SOCK_DGRAM, 0);

            /* I want to get an IPv4 IP address */
            ifr.ifr_addr.sa_family = AF_INET;

            /* I want IP address attached to "eth0" */
            strncpy(ifr.ifr_name, m_ifname.c_str(), IFNAMSIZ-1);

            ioctl(fd, SIOCGIFADDR, &ifr);

            close(fd);
            
            std::string ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
            std::cout << "[" << ip << "]" << std::endl;
            if ( ip != "0.0.0.0" )
            {
                break;
            }
            
            
            usleep(100000);
            
        }
        if ( !dhcp_thread_run )
            return;
        
        std::cout << "DHCP Completato, impostazione del routing..." << std::endl;
        rmgr.addDefaultRouteToIface(m_ifname,m_gateway,m_rtname);
        rmgr.addTableMark(m_rtname,m_mark);
    }
}

WPASupplicantInstance::WPASupplicantInstance(std::string ifname, std::string ssid, std::string routing_table, std::string gateway, int mark)
{
    std::stringstream confn;
    m_ifname = ifname;
    dhcp_thread_run = true;
    m_rtname = routing_table;
    m_gateway = gateway;
    m_mark = mark;
   /* std::stringstream ss2;
    ss2 << "/var/run/netbridge/" << m_ifname << "ctrl_interface";
    unlink(ss2.str().c_str());*/
    pthread_create(&dhcp_th,NULL,(void* (*)(void*))&WPASupplicantInstance::dhcp_thread,this);
    
    confn << "/var/run/netbridge/" << ifname << ".conf";
    std::ofstream ss(confn.str().c_str());
    ss << "ctrl_interface=/var/run/netbridge/\n";
    ss << "network={\n ssid=\"" << ssid << "\"\n key_mgmt=NONE\n }";
    ss.close();
    pid_t cp = fork();
    if ( cp ==  0 )
    {
        setsid();
        std::stringstream iface;
        iface << "-i" << ifname;
        std::stringstream conf;
        conf << "-c" << "/var/run/netbridge/" << ifname << ".conf";
        int ret = execl("/usr/sbin/wpa_supplicant","wpa_supplicant","-onl80211",iface.str().c_str(),conf.str().c_str());
        exit(ret);
    }
    std::cout << "Avviato wpa_supplicant, PID: " << cp << std::endl;
    m_pid = cp;
}
WPASupplicantInstance::~WPASupplicantInstance()
{
    int status;
    kill(m_pid,SIGTERM);
    waitpid(m_pid,&status,0);
    kill(m_pid,SIGKILL);
    waitpid(m_pid,&status,0);
    std::cout << "wpa_supplicant con PID: " << m_pid << " terminato " << std::endl;
    dhcp_thread_run = false;
    pthread_join(dhcp_th,NULL);
    std::cout << "Thread dhcp terminato" << std::endl;
    std::stringstream ss;
    ss << "/var/run/netbridge/dhclient_pid_" << m_ifname << ".pid";
    
    
    int pid;
    FILE * f = fopen(ss.str().c_str(),"r");
    if ( f )
    {
        fscanf(f,"%d",&pid);
        std::cout << "Terminazione di dhclient... associato all'interfaccia " << m_ifname << "con pid " << pid << std::endl;
        fclose(f);
        kill(pid,SIGTERM);
    }
    unlink(ss.str().c_str());
    
}

