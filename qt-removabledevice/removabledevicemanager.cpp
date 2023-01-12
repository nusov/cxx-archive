#include "removabledevicemanager.h"


#include <QDebug>

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef Q_OS_MAC
#include <stdio.h>
#include <dirent.h>

#include <DiskArbitration/DiskArbitration.h>
#include <QRegExp>
#include <QUrl>


char * MYCFStringCopyUTF8String(CFStringRef aString) {
  if (aString == NULL) {
    return NULL;
  }

  CFIndex length = CFStringGetLength(aString);
  CFIndex maxSize =
  CFStringGetMaximumSizeForEncoding(length,
                                    kCFStringEncodingUTF8);
  char *buffer = (char *)malloc(maxSize);
  if (CFStringGetCString(aString, buffer, maxSize,
                         kCFStringEncodingUTF8)) {
    return buffer;
  }
  return NULL;
}
#endif

RemovableDeviceManager::RemovableDeviceManager()
{
}


QMap<QString, RemovableDevice> RemovableDeviceManager::getDeviceList()
{
    QMap<QString, RemovableDevice> result;
#ifdef WIN32
    DWORD cchBuffer;
    DWORD dwTmp;

    WCHAR drive[MAX_PATH+1], mount[MAX_PATH+1];
    WCHAR *driveStrings, *driveStrings0;
    WCHAR volumeName[MAX_PATH+1];
    WCHAR fsName[MAX_PATH+1];
    WCHAR targetVolume[MAX_PATH+1];

    HANDLE hDisk;
    DISK_GEOMETRY_EX info;

    cchBuffer = GetLogicalDriveStrings(0, NULL);
    driveStrings0 = (WCHAR*) malloc((cchBuffer+1) * sizeof(WCHAR));
    driveStrings  = driveStrings0;


    GetLogicalDriveStrings(cchBuffer, driveStrings);
    while (*driveStrings)
    {
        wcscpy_s(drive, driveStrings);
        wcscpy_s(mount, driveStrings);

        driveStrings += lstrlen(driveStrings) + 1;

        if (GetDriveTypeW(drive) == DRIVE_REMOVABLE)
        {
            // make target device string
            swprintf(targetVolume, MAX_PATH+1, TEXT("\\\\.\\%c:"), drive[0]);

            // open volume for getting info
            hDisk = CreateFileW(targetVolume,
                0,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
                NULL);

            if (hDisk == INVALID_HANDLE_VALUE)
            {
                continue;
            }

            // get disk geometry
            if (!DeviceIoControl(hDisk,
                                 IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                 NULL, GENERIC_READ, &info, sizeof(info),
                                 &dwTmp, NULL))
            {
                CloseHandle(hDisk);
                continue;
            }

            CloseHandle(hDisk);

            // remove last slash
            if ((lstrlen(drive) == 3) && (drive[2] == '\\')) {
                drive[2] = '\0';
            }

            // work with data
            RemovableDevice device;
            device.bytesPerSector = info.Geometry.BytesPerSector;
            device.sectorsCount   = info.DiskSize.QuadPart / info.Geometry.BytesPerSector;
            device.size  = info.DiskSize.QuadPart;
            device.path  = QString::fromWCharArray(targetVolume);
            device.name  = QString::fromWCharArray(drive);
            device.mount = QString::fromWCharArray(mount);

            if (GetVolumeInformationW(drive,
                                      volumeName,
                                      MAX_PATH+1,
                                      NULL,
                                      NULL,
                                      NULL,
                                      fsName,
                                      MAX_PATH+1))
            {
                device.label = QString::fromWCharArray(volumeName);
                device.fs = QString::fromWCharArray(fsName);
            }

            QString key = device.path;
            if (key.length())
                result[key] = device;

        }
    }

    free(driveStrings0);
#endif

#ifdef Q_OS_MAC
    // create session
    DASessionRef session = DASessionCreate(NULL);

    // open directory and find devices
    struct dirent *pDirent;
    DIR *pDir = opendir("/dev");
    while((pDirent = readdir(pDir)) != NULL) {
        QString f(pDirent->d_name);
        if (f.contains(QRegExp("^disk(\\d)s(\\d)$"))) {
            qDebug() << "Processing" << f;

            // get Disk information
            DADiskRef disk = DADiskCreateFromBSDName(NULL, session, QString("/dev/%1").arg(f).toUtf8().constData());
            if (!disk)
                continue;

            // get Disk info dictionary
            CFDictionaryRef descDict = DADiskCopyDescription(disk);
            if (!descDict)
                continue;

            // check if device is removable
            CFBooleanRef isRemovableRef = (CFBooleanRef)CFDictionaryGetValue(descDict, kDADiskDescriptionMediaRemovableKey);
            if (!CFBooleanGetValue(isRemovableRef))
                continue;



            // check if device is ejectable
            CFBooleanRef isEjectableRef = (CFBooleanRef)CFDictionaryGetValue(descDict, kDADiskDescriptionMediaEjectableKey);
            if (!CFBooleanGetValue(isEjectableRef))
                continue;



            // check if device is not network
            CFBooleanRef isNetworkRef = (CFBooleanRef)CFDictionaryGetValue(descDict, kDADiskDescriptionVolumeNetworkKey);
            if (CFBooleanGetValue(isNetworkRef))
                continue;



            // get bus name and filter mounted DMG images
            CFStringRef busNameRef = (CFStringRef)CFDictionaryGetValue(descDict, kDADiskDescriptionBusNameKey);
            const char *busNameCStr = MYCFStringCopyUTF8String(busNameRef);
            QString busName(busNameCStr);
            free((void*)busNameCStr);

            if (!busName.compare(QString("/")))
                continue;

            qDebug() << "getting volume path";

            // get volume path
            CFURLRef volumePathUrlRef = (CFURLRef)CFDictionaryGetValue(descDict, kDADiskDescriptionVolumePathKey);
            QString volumePath(QString::null);
            if (volumePathUrlRef) {
                CFStringRef volumePathRef = (CFStringRef) CFURLCopyPath(volumePathUrlRef);
                const char *volumePathCStr = MYCFStringCopyUTF8String(volumePathRef);
                QString volumePath(QUrl::fromPercentEncoding(QByteArray(volumePathCStr)));

                free((void *)volumePathCStr);
            }

            // get volume name
            CFStringRef volumeNameRef = (CFStringRef)CFDictionaryGetValue(descDict, kDADiskDescriptionVolumeNameKey);
            const char *volumeNameCStr = MYCFStringCopyUTF8String(volumeNameRef);
            QString volumeName(volumeNameCStr);
            free((void*)volumeNameCStr);

            // get media content
            CFStringRef mediaContentRef = (CFStringRef)CFDictionaryGetValue(descDict, kDADiskDescriptionMediaContentKey);
            const char *mediaContentCStr = MYCFStringCopyUTF8String(mediaContentRef);
            QString mediaContent(mediaContentCStr);
            free((void*)mediaContentCStr);
            mediaContent.replace('_', ' ');



            // get device file
            QString deviceFile = QString("/dev/%1").arg(f);

            // get block size
            const void *blockSizeRef = (CFStringRef)CFDictionaryGetValue(descDict, kDADiskDescriptionMediaBlockSizeKey);
            CFStringRef blockSizeStrRef = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@"), blockSizeRef);
            const char *blockSizeStr = MYCFStringCopyUTF8String(blockSizeStrRef);
            qint32 blockSize = QString(blockSizeStr).toInt();
            free((void*)blockSizeStr);

            // get media size
            CFNumberRef mediaSizeRef = (CFNumberRef)CFDictionaryGetValue(descDict, kDADiskDescriptionMediaSizeKey);
            qint64 mediaSize;
            CFNumberGetValue(mediaSizeRef, kCFNumberLongLongType, &mediaSize);

            RemovableDevice device;
            device.bytesPerSector = blockSize;
            device.sectorsCount   = mediaSize / blockSize;
            device.size  = mediaSize;
            device.path  = deviceFile;
            device.name  = f;
            device.mount = volumePath;
            device.label = volumeName;
            device.fs = mediaContent;

            qDebug() << "VOLUME" << volumePath;

            QString key = device.path;
            if (key.length())
                result[key] = device;

            qDebug() << "END";

            // release coreservices objects
            if (descDict)
                CFRelease(descDict);
            if (disk)
                CFRelease(disk);
        }
    }
    rewinddir(pDir);
    if (session)
        CFRelease(session);

    closedir(pDir);

#endif
    return result;
}
