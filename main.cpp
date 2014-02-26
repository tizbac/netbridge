#include <iostream>
#include "nl80211iface.h"
#include "wpasupplicantinstance.h"
#include "routingmanager.h"
#include <stdio.h>
#include <unistd.h>
#include <set>
#include <map>
#include <vector>
#include <sstream>
#include <signal.h>
#include <sys/stat.h>
std::set<std::string> stations;
std::map<std::string,int> clients; // Mac address connessione verso sapienza , indice slot
std::vector<std::string> slots;
std::vector<time_t> last_connect_check;
std::vector<WPASupplicantInstance*> wpa_supplicants;
std::string mac_addr_translate(std::string in)
{
    unsigned char mac0,mac1,mac2,mac3,mac4,mac5;
    sscanf(in.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &mac0,
        &mac1,
        &mac2,
        &mac3,
        &mac4,
        &mac5
        );
    mac3 ^= 135;
    mac4 ^= 242;
    mac5 ^= 167;
    char newmac[32];
    sprintf(newmac,"%02x:%02x:%02x:%02x:%02x:%02x",(unsigned int)mac0,(unsigned int)mac1,(unsigned int)mac2,(unsigned int)mac3,(unsigned int)mac4,(unsigned int)mac5);
    return std::string(newmac);
    
}
std::string vifname(int index)
{
    char ifname[64];
    sprintf(ifname,"client_%03d",index);
    return std::string(ifname);
}
std::string rtablename(int index)
{
    char ifname[64];
    sprintf(ifname,"client%03d",index);
    return std::string(ifname);
}
void on_terminate(int sig)
{
  RoutingManager rmgr;
  std::cerr << "Uscita..." << std::endl;
  for ( std::set<std::string>::iterator it = stations.begin(); it != stations.end(); it++ )
  {

    int slot = clients[mac_addr_translate(*it)];
    slots[slot] = "";
    NL80211Iface sta(vifname(slot));
    sta.disconnectVirtualIface(vifname(slot));
    sta.deleteVirtualIface(vifname(slot));
    rmgr.clearTableMark(rtablename(slot));
    rmgr.removeDefaultRouteFromTable(rtablename(slot));
    std::stringstream cmd;
    cmd << "iptables -D PREROUTING -m mac --mac-source " << *it << " -t mangle -j MARK --set-mark " << slot+1;
    std::stringstream cmd2;
    cmd2 << "iptables -t nat -D POSTROUTING -s 172.16.0.0/16 -o " << vifname(slot) << " -j MASQUERADE";
    system(cmd2.str().c_str());
    system(cmd.str().c_str());
    clients.erase(mac_addr_translate(*it));
    delete wpa_supplicants[slot];

  }
  exit(0);
}
int main(int argc, char **argv) {
   // std::cout << "Hello, world!" << std::endl;
    NL80211Iface::init();
    mkdir("/var/run/netbridge",0557);
    if ( argc < 5 )
    {
        std::cerr << "Utilizzo: netbridge ssid gateway interfaccia_client interfaccia_ap" << std::endl;
        return 1;
    }
    signal(SIGINT,on_terminate);
    signal(SIGTERM,on_terminate);
    
    std::string ssid = argv[1];
    std::string gateway = argv[2];
    //Test di sanità
    std::string mac_test = "00:76:a1:44:b0:24";
    std::string translated = mac_addr_translate(mac_test);
    std::string retranslated = mac_addr_translate(translated);
    std::cout << mac_test << " " << translated << " " << retranslated << std::endl;
    
    NL80211Iface sta(argv[3]);
    std::vector<NL80211Iface*> aps;
    for ( int i = 4; i < argc; i++ )
    {
        aps.push_back(new NL80211Iface(argv[i]));
    }
    RoutingManager rmgr;
   /* for ( std::vector<std::string>::iterator it = stations.begin(); it != stations.end(); it++ )
    {
        std::cout << *it << std::endl;
    }*/
    std::cout << "Disconnessione e rimozione di eventuali client vecchi..." << std::endl;
    for ( int i = 0; i < 200; i++ )
    {
        std::string ifname = vifname(i);
        sta.disconnectVirtualIface(ifname);
        sta.deleteVirtualIface(ifname);
    }
    std::cout << "Pulizia delle regole di routing ed iptables..." << std::endl;
    for ( int i = 0; i < 200; i++ )
    {
        rmgr.removeDefaultRouteFromTable(rtablename(i));
        rmgr.clearTableMark(rtablename(i));
    }
    system("iptables -F");
    system("iptables -t nat -F");
    system("iptables -t mangle -F");
    
    std::cout << "Abilito il forwarding..." << std::endl;
    
    system("sysctl -w net.ipv4.ip_forward=1");

    slots.resize(200);
    last_connect_check.resize(200);
    wpa_supplicants.resize(200);
    while ( true )
    {
        std::vector<std::string> vstations;
        for ( std::vector<NL80211Iface*>::iterator it = aps.begin(); it != aps.end(); it++ )
        {
            std::vector<std::string> stas_1 = (*it)->enumSta();
            vstations.insert(vstations.end(),stas_1.begin(),stas_1.end());
        }
        std::set<std::string> curr_stations;
        for ( std::vector<std::string>::iterator it = vstations.begin(); it != vstations.end(); it++ )
        {
            std::string s = *it;
            if ( stations.find(s) == stations.end() ) // Nuovo client appena connesso
            {
                //Trova il primo slot libero
                for ( int i = 0; i < slots.size(); i++ )
                {
                    if ( slots[i].length() == 0 )
                    {
                        sta.createNewVirtualIface(vifname(i),mac_addr_translate(s));
                        
                        std::stringstream cmd;
                        cmd << "iptables -A PREROUTING -m mac --mac-source " << s << " -t mangle -j MARK --set-mark " << i+1;
                        system(cmd.str().c_str());
                        std::stringstream cmd2;
                        cmd2 << "iptables -t nat -A POSTROUTING -s 172.16.0.0/16 -o " << vifname(i) << " -j MASQUERADE";
                        system(cmd2.str().c_str());
                        std::stringstream cmd3;
                        cmd3 << "sysctl -w net.ipv4.conf." << vifname(i) << ".rp_filter=0";
                        system(cmd3.str().c_str());
                        //sta.connectVirtualIfaceTo(vifname(i),ssid); Gestito da wpa_supplicant
                        wpa_supplicants[i] = new WPASupplicantInstance(vifname(i),ssid,rtablename(i),gateway,i+1);
                        slots[i] = s;
                        clients.insert(std::pair<std::string,int>(mac_addr_translate(s),i));
                        break;
                    }
                    
                }
                
            }
            curr_stations.insert(s);
        }
        
        for ( std::set<std::string>::iterator it = stations.begin(); it != stations.end(); it++ )
        {
            if ( curr_stations.find(*it) == curr_stations.end() ) // Client disconnesso
            {
                int slot = clients[mac_addr_translate(*it)];
                slots[slot] = "";
                sta.disconnectVirtualIface(vifname(slot));
                sta.deleteVirtualIface(vifname(slot));
                rmgr.clearTableMark(rtablename(slot));
                rmgr.removeDefaultRouteFromTable(rtablename(slot));
                std::stringstream cmd;
                cmd << "iptables -D PREROUTING -m mac --mac-source " << *it << " -t mangle -j MARK --set-mark " << slot+1;
                std::stringstream cmd2;
                cmd2 << "iptables -t nat -D POSTROUTING -s 172.16.0.0/16 -o " << vifname(slot) << " -j MASQUERADE";
                system(cmd2.str().c_str());
                system(cmd.str().c_str());
                clients.erase(mac_addr_translate(*it));
                delete wpa_supplicants[slot];
                
            }
        }
        for ( int i = 0; i < slots.size(); i++ )
        {
            if ( slots[i].length() > 0 )
                wpa_supplicants[i]->pollConnection();
        }
        stations = curr_stations;
        std::cout << stations.size() << " connessi" << std::endl;
        
        
        
        sleep(3);
    }
    
    
    //ap.createNewVirtualIface("client_000","00:63:a1:b7:aa:24");
    return 0;
}
