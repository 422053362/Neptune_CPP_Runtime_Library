/*****************************************************************
 |
 |      Neptune - Network :: BSD Implementation
 |
 |      (c) 2001-2005 Gilles Boccon-Gibod
 |      Author: Gilles Boccon-Gibod (bok@bok.net)
 |
 ****************************************************************/

/*
 History:
 
 @2014-2-24, Zhikuan.Yan, revised it to work with BSD network library.
 
 */

/*----------------------------------------------------------------------
 |       includes
 +---------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>


#include "NptConfig.h"
#include "NptTypes.h"
#include "NptStreams.h"
#include "NptThreads.h"
#include "NptNetwork.h"
#include "NptUtils.h"
#include "NptConstants.h"

#if defined(NPT_CONFIG_HAVE_NET_IF_DL_H)
#include <net/if_dl.h>
#endif
#if defined(NPT_CONFIG_HAVE_NET_IF_TYPES_H)
#include <net/if_types.h>
#endif

/*----------------------------------------------------------------------
 |       platform adaptation
 +---------------------------------------------------------------------*/
#if !defined(IFHWADDRLEN)
#define IFHWADDRLEN 6 // default to 48 bits
#endif


/*----------------------------------------------------------------------
 |       NPT_NetworkInterface::GetNetworkInterfaces
 +---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    int result = 0;
    struct ifaddrs * ifaddrsList = NULL;
    struct ifaddrs * ifaddr = NULL;
    NPT_Flags flags = 0;
    
    result = getifaddrs(&ifaddrsList);
    
    if( result != 0 || ifaddrsList == NULL)
        return NPT_FAILURE;
    
    for(ifaddr = ifaddrsList; NULL!= ifaddr; ifaddr = ifaddr->ifa_next) {
        
        if(ifaddr->ifa_addr == NULL /*|| ifaddr->ifa_addr->sa_family == AF_INET6*/)
            continue;
        
        // process the flags
        if ((ifaddr->ifa_flags & IFF_UP) == 0) {
            // the interface is not up, ignore it
            continue;
        }
        if (ifaddr->ifa_flags & IFF_BROADCAST) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_BROADCAST;
        }
        if (ifaddr->ifa_flags & IFF_LOOPBACK) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_LOOPBACK;
        }
#if defined(IFF_POINTOPOINT)
        if (ifaddr->ifa_flags & IFF_POINTOPOINT) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT;
        }
#endif // defined(IFF_POINTOPOINT)
        if (ifaddr->ifa_flags & IFF_PROMISC) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_PROMISCUOUS;
        }
        if (ifaddr->ifa_flags & IFF_MULTICAST) {
            flags |= NPT_NETWORK_INTERFACE_FLAG_MULTICAST;
        }
        
        NPT_NetworkInterface* interface = NULL;
        for (NPT_List<NPT_NetworkInterface*>::Iterator iface_iter = interfaces.GetFirstItem();
             iface_iter;
             ++iface_iter) {
            if ((*iface_iter)->GetName() == (const char*)ifaddr->ifa_name) {
                interface = *iface_iter;
                break;
            }
        }
        
        // create a new interface object
        if(interface == NULL)
            interface = new NPT_NetworkInterface(ifaddr->ifa_name, flags);
        
        if (interface == NULL)
            continue;
            
            // get the mac address
            NPT_MacAddress::Type mac_addr_type;
            unsigned int         mac_addr_length = IFHWADDRLEN;
            switch (ifaddr->ifa_addr->sa_family) {
                case AF_LOCAL:
                case AF_INET:
                case AF_LINK:
                    mac_addr_type = NPT_MacAddress::TYPE_ETHERNET;
#if defined(ARPHRD_LOOPBACK)      
                    mac_addr_type = NPT_MacAddress::TYPE_LOOPBACK;
                    length = 0;
#endif               
                    break;
                    
                case AF_PPP:
                    mac_addr_type = NPT_MacAddress::TYPE_PPP;
                    mac_addr_length = 0;
                    break;

                case AF_IEEE80211:
                    mac_addr_type = NPT_MacAddress::TYPE_IEEE_802_11;
                    break;
                    
                default:
                    mac_addr_type = NPT_MacAddress::TYPE_UNKNOWN;
                    mac_addr_length = sizeof(ifaddr->ifa_addr->sa_data);
                    break;
            }


            if(interface->GetMacAddress().GetLength() == 0)
                interface->SetMacAddress(mac_addr_type, (const unsigned char*)ifaddr->ifa_addr->sa_data, mac_addr_length);
        
#if defined(NPT_CONFIG_HAVE_NET_IF_DL_H)        
        if (ifaddr->ifa_addr->sa_family == AF_LINK) {
            //Refer to LLADDR
            struct sockaddr_dl * socket_dl = (struct sockaddr_dl *)ifaddr->ifa_addr;
            
            interface->SetMacAddress(mac_addr_type, (const unsigned char*)socket_dl->sdl_data+socket_dl->sdl_nlen, socket_dl->sdl_alen);
        }
#endif
            switch (ifaddr->ifa_addr->sa_family) {
                case AF_INET: {
                    // primary address
                    NPT_IpAddress primary_address( ntohl(((struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr.s_addr));
                    
                    // broadcast address
                    NPT_IpAddress broadcast_address;
                    if (flags & NPT_NETWORK_INTERFACE_FLAG_BROADCAST) {
                        broadcast_address.Set(ntohl(((struct sockaddr_in*)ifaddr->ifa_dstaddr)->sin_addr.s_addr));
                    }
                    
                    // point to point address
                    NPT_IpAddress destination_address;
                    if (flags & NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT) {
                        destination_address.Set(ntohl(((struct sockaddr_in*)ifaddr->ifa_dstaddr)->sin_addr.s_addr));
                    }
                    
                    // netmask
                    NPT_IpAddress netmask(0xFFFFFFFF);
                    netmask.Set(ntohl(((struct sockaddr_in*)ifaddr->ifa_netmask)->sin_addr.s_addr));
                    
                    // add the address to the interface
                    NPT_NetworkInterfaceAddress iface_address(primary_address,
                                                              broadcast_address,
                                                              destination_address,
                                                              netmask);

                    interface->AddAddress(iface_address);
                    
                    break;
                }
                    
            }
            // add the interface to the list
            interfaces.Add(interface);
        
    }
    freeifaddrs(ifaddrsList);
    
    return NPT_SUCCESS;
}
