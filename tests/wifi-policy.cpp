#include <windows.h>
#include "wifi-policy.h"
#include <stdio.h>
int wmain()
{
    const WCHAR Wifi[]=L"PCI\\VEN_8086&DEV_51F1&SUBSYS_00348086\0PCI\\VEN_8086&DEV_51F1\0";
    const WCHAR Wlan[]=L"PCI\\VEN_8086&CC_028000\0PCI\\CC_028000\0PCI\\CC_0280\0";
    const WCHAR Ethernet[]=L"PCI\\VEN_8086&DEV_100E\0";
    const WCHAR WiredClass[]=L"PCI\\CC_020000\0PCI\\CC_0200\0";
    const WCHAR Bluetooth[]=L"USB\\VID_8087&PID_0AAA\0";
    const WCHAR Realtek[]=L"USB\\VID_0B05&PID_184C\0";
    const WCHAR Lower[]=L"pci\\ven_8086&dev_51f1\0";
    if (!ThyfosIntelWifiIds(Wifi,Wlan) || !ThyfosIntelWifiIds(Lower,Wlan)) return 1;
    if (ThyfosIntelWifiIds(Ethernet,WiredClass)) return 2;
    if (ThyfosIntelWifiIds(Bluetooth,Wlan) || ThyfosIntelWifiIds(Realtek,Wlan)) return 3;
    if (ThyfosIntelWifiIds(L"",Wlan) || ThyfosIntelWifiIds(Wifi,L"")) return 4;
    puts("Intel wireless matching passed; virtual Ethernet, Bluetooth, other vendors and empty IDs excluded");
    return 0;
}
