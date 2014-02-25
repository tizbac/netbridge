

#include <iostream>
#include "nl80211iface.h"

int main(int argc, char **argv) {
    NL80211Iface::init();
    NL80211Iface iface(argv[1]);
    iface.connectVirtualIfaceTo(argv[1],argv[2],argv[3]);
    
}