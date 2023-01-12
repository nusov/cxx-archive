#ifndef REMOVABLEDEVICEMANAGER_H
#define REMOVABLEDEVICEMANAGER_H

#include <QString>
#include <QMap>

#include "removabledevice.h"

class RemovableDeviceManager
{
public:
    RemovableDeviceManager();

    static QMap<QString, RemovableDevice> getDeviceList();
};

#endif // REMOVABLEDEVICEMANAGER_H
