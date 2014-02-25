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
#include <time.h>
void WPASupplicantInstance::dhcp_thread()
{
    std::cout << "Thread DHCP avviato" << std::endl;
    //NL80211Iface iface(m_ifname); Netlink si scazza con il multithreading
    RoutingManager rmgr;
    while ( dhcp_thread_run )
    {
        if ( isconnected )
            break;
        std::cout << "Attendo la connessione..." << std::endl;
        usleep(500000);
    }
    if ( !dhcp_thread_run )
        return;
    std::stringstream ss;
    ss << "dhclient " << m_ifname << " -pf /var/run/netbridge/dhclient_pid_" << rand() << rand() << rand() << rand() << ".pid";

    system(ss.str().c_str());
        
    std::cout << "DHCP Completato, impostazione del routing..." << std::endl;
    rmgr.addDefaultRouteToIface(m_ifname,m_gateway,m_rtname);
    rmgr.addTableMark(m_rtname,m_mark);

}

void WPASupplicantInstance::keepalive_thread()
{
    NL80211Iface iface(m_ifname);
    iface.connectVirtualIfaceTo(m_ifname,m_ssid,m_bssid);//La prima volta deve lanciare subito la connessione
    struct timeval now;
    struct timespec ts;
    gettimeofday(&now,NULL);
    ts.tv_sec = now.tv_sec;
    ts.tv_nsec = now.tv_usec *1000;
    ts.tv_sec += 10;

    pthread_cond_timedwait(&keepalive_cond,&keepalive_mutex,&ts);
    while ( !iface.isConnected() && keepalive_run)
    {
        std::cerr << m_ifname << ":Connessione fallita, ritento..." << std::endl;
        iface.connectVirtualIfaceTo(m_ifname,m_ssid,m_bssid);
        gettimeofday(&now,NULL);
        ts.tv_sec = now.tv_sec;
        ts.tv_nsec = now.tv_usec *1000;
        ts.tv_sec += 10;
        pthread_cond_timedwait(&keepalive_cond,&keepalive_mutex,&ts);
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
    isconnected = false;
    keepalive_run = true;
    if ( ssid.find(',') != ssid.npos )
    {
      m_bssid = ssid.substr(ssid.find(',')+1);
      ssid = ssid.substr(0,ssid.find(','));
    }
    m_ssid = ssid;
   /* std::stringstream ss2;
    ss2 << "/var/run/netbridge/" << m_ifname << "ctrl_interface";
    unlink(ss2.str().c_str());*/
   
   
    pthread_mutex_init(&keepalive_mutex,NULL);
    pthread_cond_init(&keepalive_cond,NULL);
    pthread_create(&dhcp_th,NULL,(void* (*)(void*))&WPASupplicantInstance::dhcp_thread,this);
    pthread_create(&keepalive_th,NULL,(void* (*)(void*))&WPASupplicantInstance::keepalive_thread,this);
   // confn << "/var/run/netbridge/" << ifname << ".conf";
   /* std::ofstream ss(confn.str().c_str());
    ss << "ctrl_interface=/var/run/netbridge/\n";
    ss << "network={\n ssid=\"" << ssid << "\"\n key_mgmt=NONE";
    if ( bssid.length() > 0 )
      ss << "\n bssid=" << bssid;
    ss << "\n}";
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
    std::cout << "Avviato wpa_supplicant, PID: " << cp << std::endl;*/

}
void WPASupplicantInstance::pollConnection()
{
    NL80211Iface iface(m_ifname);
    isconnected = iface.isConnected();
}

WPASupplicantInstance::~WPASupplicantInstance()
{
    int status;

    std::cout << "wpa_supplicant con PID: " << m_pid << " terminato " << std::endl;
    
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
    
    dhcp_thread_run = false;
    pthread_join(dhcp_th,NULL);
    keepalive_run = false;
    pthread_cond_broadcast(&keepalive_cond);
    pthread_join(keepalive_th,NULL);
}

