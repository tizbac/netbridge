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

#ifndef NL80211AP_H
#define NL80211AP_H

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <string>
#include <vector>
class NL80211Iface
{
public:
    NL80211Iface(std::string ifname);
    ~NL80211Iface();
    std::vector<std::string> enumSta();
    static int staListPopulate(struct nl_msg *msg, void *arg);
    
    bool createNewVirtualIface(std::string name , std::string mac_addr);
    bool deleteVirtualIface(std::string name);
    bool disconnectVirtualIface(std::string name);
    bool connectVirtualIfaceTo(std::string name, std::string ssid);
    bool isConnected();
private:
    std::string m_ifname;
    struct nl_sock * m_sck;
    int nl802111_id;
};

#endif // NL80211AP_H
