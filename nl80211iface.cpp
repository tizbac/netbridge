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

#include "nl80211iface.h"
#include "nl80211.h"
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
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include "log.h"
static inline struct nl_handle *nl_socket_alloc(void)
{
    return nl_handle_alloc();
}
static inline void nl_socket_free(struct nl_sock *h)
{
    nl_handle_destroy((nl_handle*)h);
}
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
             void *arg)
{
    int *ret = (int*)arg;
    *ret = err->error;
    return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
    int *ret = (int*)arg;
    *ret = 0;
    return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
    int *ret = (int*)arg;
    *ret = 0;
    return NL_STOP;
}

static inline int nl_socket_set_buffer_size(struct nl_sock *sk,
                        int rxbuf, int txbuf)
{
    return nl_set_buffer_size((nl_handle*)sk, rxbuf, txbuf);
}

void mac_addr_n2a(char *mac_addr, unsigned char *arg)
{
    int i, l;

    l = 0;
    for (i = 0; i < 6 ; i++) {
        if (i == 0) {
            sprintf(mac_addr+l, "%02x", arg[i]);
            l += 2;
        } else {
            sprintf(mac_addr+l, ":%02x", arg[i]);
            l += 3;
        }
    }
}

int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
    int i;

    for (i = 0; i < 6 ; i++) {
        int temp;
        char *cp = strchr(arg, ':');
        if (cp) {
            *cp = 0;
            cp++;
        }
        if (sscanf(arg, "%x", &temp) != 1)
            return -1;
        if (temp < 0 || temp > 255)
            return -1;

        mac_addr[i] = temp;
        if (!cp)
            break;
        arg = cp;
    }
    if (i < 6 - 1)
        return -1;

    return 0;
}

int nl802111_id = -1;
nl_sock * nl_sck = 0;
pthread_mutex_t netlink_mutex;
void NL80211Iface::init()
{
    pthread_mutex_init(&netlink_mutex,NULL);
    nl_sck = (nl_sock*)nl_socket_alloc();
    if ( !nl_sck )
    {
        L->error("Unable to allocate netlink socket");
        abort();
    }
    nl_socket_set_buffer_size(nl_sck, 8192, 8192);
    if ( genl_connect((nl_handle*)nl_sck) )
    {
        L->error("Unable to estabilish a connection to netlink");
        abort();
    }
    
    nl802111_id = genl_ctrl_resolve((nl_handle*)nl_sck, "nl80211");
    if ( nl802111_id < 0 )
    {
        L->error("Unable to find nl80211");
        abort();
    }
}

NL80211Iface::NL80211Iface(std::string ifname)
{
    m_ifname = ifname;
    
    
}

NL80211Iface::~NL80211Iface()
{
    
}

bool NL80211Iface::isConnected()
{
    return enumSta().size() > 0;
}


bool NL80211Iface::connectVirtualIfaceTo(std::string name, std::string ssid, std::string bssid)
{
    pthread_mutex_lock(&netlink_mutex);
    struct nl_msg *msg;
    struct nl_cb *cb;
    signed long long devidx = 0;
    int err;
    msg = nlmsg_alloc();
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    
    genlmsg_put(msg,0,0, nl802111_id, 0, 0, NL80211_CMD_CONNECT, 0);
    
    devidx = if_nametoindex(name.c_str());
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    NLA_PUT(msg, NL80211_ATTR_SSID, ssid.length(), ssid.c_str());
    
    if ( bssid.length() == 17 )
    {
        
        unsigned char bssid_bin[6];
        char str[256];
        if ( bssid.length() > 255 )
            return false;
        strcpy(str,bssid.c_str());
        mac_addr_a2n(bssid_bin,str);
        {
            std::stringstream ss;
            ss << name << ":Connecting to specific mac address";
            L->error(ss.str());
        }
        //printf("%02x:%02x:%02x:%02x:%02x:%02x\n",(int)bssid_bin[0],(int)bssid_bin[1],(int)bssid_bin[2],(int)bssid_bin[3],(int)bssid_bin[4],(int)bssid_bin[5]);
        NLA_PUT(msg, NL80211_ATTR_MAC, 6 , bssid_bin);
    }
    
    struct nl_cb *s_cb;
    
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    
    nl_socket_set_cb((nl_handle*)nl_sck,s_cb);
    // std::cout << "F2" << std::endl;
    if ( nl_send_auto_complete((nl_handle*)nl_sck,msg) < 0 )
    {
        L->error("Netlink: Failed to send connect command");
        pthread_mutex_unlock(&netlink_mutex);
        
        return false;
    }
    
    err = 1;
    
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    
    while ( err > 0 )
        nl_recvmsgs((nl_handle*)nl_sck, cb);
    
    nl_cb_put(cb);
    
    
    
    nlmsg_free(msg);
    
    if ( ! err )
        L->info("Interface "+name+" is connecting to AP...");
    else
    {
        std::stringstream ss;
        
        ss << name << ": Connection failed:" << err << "-" << strerror(-err);
        
        L->error(ss.str());
     
    }

    nla_put_failure:
  //  std::cout << "F" << std::endl;
    
    pthread_mutex_unlock(&netlink_mutex);
    return true;
}

bool NL80211Iface::disconnectVirtualIface(std::string name)
{
    pthread_mutex_lock(&netlink_mutex);
    struct nl_msg *msg;
    struct nl_cb *cb;

    signed long long devidx = 0;
    int err;
    msg = nlmsg_alloc();
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    
    
    genlmsg_put(msg,0,0, nl802111_id, 0, 0, NL80211_CMD_DISCONNECT, 0);
    
    devidx = if_nametoindex(name.c_str());
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    
    struct nl_cb *s_cb;
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    nl_socket_set_cb((nl_handle*)nl_sck,s_cb);
    if ( nl_send_auto_complete((nl_handle*)nl_sck,msg) < 0 )
    {
        L->error("Netlink: Failed to send disconnect command");
        pthread_mutex_unlock(&netlink_mutex);
        return false;
    }
    err = 1;
    
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    
    while ( err > 0 )
        nl_recvmsgs((nl_handle*)nl_sck, cb);
    
    nl_cb_put(cb);
    
    nla_put_failure:
    
    nlmsg_free(msg);
    if ( ! err )
        L->info("Interface "+name+" disconnected");
    else
    {
        std::stringstream ss;
        
        ss << name << ": Disconnection failed:" << err << "-" << strerror(-err);
        
        L->error(ss.str());
    }

    pthread_mutex_unlock(&netlink_mutex);
    return true;
}

bool NL80211Iface::deleteVirtualIface(std::string name)
{
    pthread_mutex_lock(&netlink_mutex);
    struct nl_msg *msg;
    struct nl_cb *cb;

    signed long long devidx = 0;
    int err;
    msg = nlmsg_alloc();
    cb = nl_cb_alloc(NL_CB_DEFAULT);

    genlmsg_put(msg,0,0, nl802111_id, 0, 0, NL80211_CMD_DEL_INTERFACE, 0);
    
    devidx = if_nametoindex(name.c_str());
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    
    struct nl_cb *s_cb;
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    nl_socket_set_cb((nl_handle*)nl_sck,s_cb);
    if ( nl_send_auto_complete((nl_handle*)nl_sck,msg) < 0 )
    {
        L->error("Netlink: Failed to send delete interface command");
        pthread_mutex_unlock(&netlink_mutex);
        return false;
    }
    err = 1;
    
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    
    while ( err > 0 )
        nl_recvmsgs((nl_handle*)nl_sck, cb);
    
    nl_cb_put(cb);
    
    nla_put_failure:
    
    nlmsg_free(msg);
    if ( ! err )
        L->info("Interface "+name+" destroyed");
    else
    {
        std::stringstream ss;
        
        ss << name << ": Failed to destroy virtual interface:" << err << "-" << strerror(-err);
        
        L->error(ss.str());
    }
    pthread_mutex_unlock(&netlink_mutex);
    return true;
}


bool NL80211Iface::createNewVirtualIface(std::string name, std::string mac_addr)
{
    pthread_mutex_lock(&netlink_mutex);
    struct nl_msg *msg;
    struct nl_cb *cb;

    signed long long devidx = 0;
    int err;
    msg = nlmsg_alloc();
    cb = nl_cb_alloc(NL_CB_DEFAULT);

    
    genlmsg_put(msg,0,0, nl802111_id, 0, 0, NL80211_CMD_NEW_INTERFACE, 0);
    
    devidx = if_nametoindex(m_ifname.c_str());
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    
    
    NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, name.c_str());
    NLA_PUT_U32(msg,NL80211_ATTR_IFTYPE, NL80211_IFTYPE_STATION);
    
    struct nl_cb *s_cb;
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    nl_socket_set_cb((nl_handle*)nl_sck,s_cb);
    if ( nl_send_auto_complete((nl_handle*)nl_sck,msg) < 0 )
    {
        L->error("Netlink: Failed to send create interface command");
        pthread_mutex_unlock(&netlink_mutex);
        return false;
    }
    err = 1;
    
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    
    while ( err > 0 )
        nl_recvmsgs((nl_handle*)nl_sck, cb);
    
    nl_cb_put(cb);
    
    nla_put_failure:
    
    nlmsg_free(msg);
    
    //Impostazione mac address
    struct ifreq ifr;int s;
    sscanf(mac_addr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&ifr.ifr_hwaddr.sa_data[0],
        &ifr.ifr_hwaddr.sa_data[1],
        &ifr.ifr_hwaddr.sa_data[2],
        &ifr.ifr_hwaddr.sa_data[3],
        &ifr.ifr_hwaddr.sa_data[4],
        &ifr.ifr_hwaddr.sa_data[5]
        );
    s = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, name.c_str());
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    ifr.ifr_flags = IFF_UP;
    ioctl(s, SIOCSIFHWADDR, &ifr);
    ioctl(s, SIOCSIFFLAGS, &ifr);
    L->info(std::string("New interface created: ")+name+" on "+m_ifname+" with HW addr: "+mac_addr);
    
    
    
    pthread_mutex_unlock(&netlink_mutex);
    return true;
}


std::vector< std::string > NL80211Iface::enumSta()
{
    pthread_mutex_lock(&netlink_mutex);
    std::vector< std::string > ret;
    struct nl_msg *msg;
    struct nl_cb *cb;
    
    signed long long devidx = 0;
    int err;
    msg = nlmsg_alloc();
    cb = nl_cb_alloc(NL_CB_DEFAULT);

    genlmsg_put(msg,0,0, nl802111_id, 0, NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
    devidx = if_nametoindex(m_ifname.c_str());
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
    struct nl_cb *s_cb;
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    nl_socket_set_cb((nl_handle*)nl_sck,s_cb);

    if ( nl_send_auto_complete((nl_handle*)nl_sck,msg) < 0 )
    {
        L->error("Netlink: Failed to send get station command");
        pthread_mutex_unlock(&netlink_mutex);
        return ret;
    }
    err = 1;
    
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, &NL80211Iface::staListPopulate, &ret);
    
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    
    while ( err > 0 )
        nl_recvmsgs((nl_handle*)nl_sck, cb);
    
    nl_cb_put(cb);
    
    
    nla_put_failure:
    if ( err )
    {
        L->warn("Failed to enumerate stations on "+m_ifname+", returning empty list");
    }
    nlmsg_free(msg);
    pthread_mutex_unlock(&netlink_mutex);
    return ret;
}
int NL80211Iface::staListPopulate(nl_msg* msg, void* arg)
{
    std::vector<std::string> * v = (std::vector<std::string> * )arg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
          genlmsg_attrlen(gnlh, 0), NULL);
    char mac_addr[20];
    mac_addr_n2a(mac_addr, (unsigned char*)nla_data(tb[NL80211_ATTR_MAC]));
    v->push_back(std::string(mac_addr));
    return NL_SKIP;
}
