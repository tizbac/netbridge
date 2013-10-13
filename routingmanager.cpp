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

#include "routingmanager.h"
#include <net/if.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/in_route.h>
extern "C" {
#include "libnetlink.h"
#include "rt_names.h"
}
#include <sstream>
#include <iostream>
RoutingManager::RoutingManager()
{
    
}

RoutingManager::~RoutingManager()
{

}
void RoutingManager::addDefaultRouteToIface(std::string iface, std::string default_route, std::string table)
{
   /* unsigned int tid;

    if ( rtnl_rttable_a2n(&tid,table.c_str()) )
        return;
    filter_fn.t*/
   
   //HACK: I sorgenti di iproute2 sono incomprensibili , usiamo il comando route per ora
   std::stringstream cmd;
   cmd << "ip route add default via " << default_route << " dev " << iface << " table " << table;
   if ( int ret = system(cmd.str().c_str()) )
   {
       std::cerr << cmd.str() << " fallito con codice " << ret << std::endl;
   }
}

void RoutingManager::removeDefaultRouteFromTable(std::string table)
{
   std::stringstream cmd;
   cmd << "ip route del default table " << table;
   if ( int ret = system(cmd.str().c_str()) )
   {
       std::cerr << cmd.str() << " fallito con codice " << ret << std::endl;
   }
}

void RoutingManager::addTableMark(std::string table, int fwmark)
{
   std::stringstream cmd;
   cmd << "ip rule add fwmark " << fwmark << " table " << table;
   if ( int ret = system(cmd.str().c_str()) )
   {
       std::cerr << cmd.str() << " fallito con codice " << ret << std::endl;
   }
}

void RoutingManager::clearTableMark(std::string table)
{
   std::stringstream cmd;
   cmd << "ip rule del table " << table;
   if ( int ret = system(cmd.str().c_str()) )
   {
       std::cerr << cmd.str() << " fallito con codice " << ret << std::endl;
   }
}

