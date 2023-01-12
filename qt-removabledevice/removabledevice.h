#ifndef REMOVABLEDEVICE_H
#define REMOVABLEDEVICE_H

#include <QString>
#include <QObject>

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef Q_OS_MAC

#endif

class RemovableDevice
{
protected:
#ifdef WIN32
    HANDLE hDisk;
#endif

public:
    RemovableDevice();

    QString name;
    QString path;
    QString label;
    QString mount;
    QString fs;

    qint32 bytesPerSector;
    qint32 sectorsCount;
    qint64 size;

    QString getItemString();
    bool isAvailable();
    bool isMounted();
    bool open();
    bool close();

    void dump(QString &fname, qint64 begin, qint64 size);
};

#endif // REMOVABLEDEVICE_H
