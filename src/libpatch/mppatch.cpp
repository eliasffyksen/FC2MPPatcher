#include <QNetworkInterface>

#include "mppatch.h"
#include "global.h"

void MPPatch::readSettings()
{
    if (!settings) {
        settings = new QSettings(patch_configuration_file, QSettings::IniFormat);
        settings->beginGroup(patch_configuration_network);
            QNetworkInterface networkInterface = QNetworkInterface::interfaceFromIndex(settings->value(patch_configuration_network_interface_index).toInt());

            // Scan thru addresses for this interface.
            for (const QNetworkAddressEntry &addressEntry : networkInterface.addressEntries()) {
                // We're onlt looking for IPv4 addresses.
                if (addressEntry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    address = addressEntry.ip().toString();
                    broadcast = addressEntry.broadcast().toString();
                }
            }

            //address = settings->value(patch_configuration_network_address).toString();
            //broadcast = settings->value(patch_configuration_network_broadcast).toString();
        settings->endGroup();
    }

    // TODO: Delete settings?
}

QNetworkInterface MPPatch::findValidInterface(int hintIndex)
{
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();

    // Replace stored interface in list.
    if (hintIndex > 0) {
        const QNetworkInterface networkInterface = QNetworkInterface::interfaceFromIndex(hintIndex);

        // Removed identical network interface from list.
        if (!list.contains(networkInterface)) {
            const QNetworkInterface interfaceToRemove = networkInterface;
            list.removeAt(list.indexOf(interfaceToRemove));
        }

        // Insert network interface.
        list.prepend(networkInterface);
    }

    // Loop thru all of the systems network interfaces.
    for (const QNetworkInterface &networkInterface : list) {
        const QNetworkInterface::InterfaceFlags &flags = networkInterface.flags();

        // We only want active network interfaces and not loopback interfaces.
        if (flags.testFlag(QNetworkInterface::IsUp) && !flags.testFlag(QNetworkInterface::IsLoopBack)) {
            QNetworkAddressEntry selectedAddressEntry;

            // Scan thru addresses for this interface.
            for (const QNetworkAddressEntry &addressEntry : networkInterface.addressEntries()) {
                // We're onlt looking for IPv4 addresses.
                if (addressEntry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    return networkInterface;
                }
            }
        }
    }

    return QNetworkInterface();
}

unsigned long __stdcall MPPatch::getAdaptersInfo_patch(IP_ADAPTER_INFO* adapterInfo, unsigned long* sizePointer)
{
    unsigned long result = GetAdaptersInfo(adapterInfo, sizePointer);

    if (result == ERROR_BUFFER_OVERFLOW) {
        return result;
    }

    IP_ADAPTER_INFO* adapter = adapterInfo;

    readSettings();

    while (strcmp(adapter->IpAddressList.IpAddress.String, address.toStdString().c_str()) != 0) {
        adapter = adapter->Next;
    }

    adapter->Next = nullptr;
    memcpy(adapterInfo, adapter, sizeof(IP_ADAPTER_INFO));

    return result;
}

hostent* WSAAPI __stdcall MPPatch::getHostByName_patch(const char* name)
{
    Q_UNUSED(name);

    readSettings();

    return gethostbyname(address.toStdString().c_str());
}

int WSAAPI __stdcall MPPatch::sendTo_patch(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen)
{
    sockaddr_in* to_in = reinterpret_cast<sockaddr_in*>(const_cast<sockaddr*>(to));

    readSettings();

    // If destination address is 255.255.255.255, use subnet broadcast address instead.
    if (to_in->sin_addr.s_addr == inet_addr("255.255.255.255")) {
        to_in->sin_addr.s_addr = inet_addr(broadcast.toStdString().c_str());
    }

    return sendto(s, buf, len, flags, to, tolen);
}

int WSAAPI __stdcall MPPatch::connect_patch(SOCKET s, const sockaddr *name, int namelen)
{ 
    sockaddr_in* name_in = reinterpret_cast<sockaddr_in*>(const_cast<sockaddr*>(name));

    // If connecting to lobbyserver on port 3100, use default lobby server port instead.
    if (name_in->sin_addr.s_addr == inet_addr(patch_network_lobbyserver_address) && name_in->sin_port == htons(3100)) {
        name_in->sin_port = htons(patch_network_lobbyserver_port);
    }

    return connect(s, name, namelen);
}
