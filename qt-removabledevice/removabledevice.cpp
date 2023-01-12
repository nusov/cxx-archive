#include "removabledevice.h"

#ifdef WIN32
#define NOMINMAX
#include <WinBase.h>
#include <Windows.h>
#endif

#include <QFile>
#include <QByteArray>
#include <QDebug>

RemovableDevice::RemovableDevice()
{

}


QString RemovableDevice::getItemString()
{
    QString s;

    // volume name (if exists)
    if (label.length()) {
        s.append(label);
    }
    else {
        s.append(QObject::tr("Removable Media"));
    }

    // disk name (like C: or D:)
    if (name.length()) {
        s.append(QString(" (%1)").arg(name));
    }

    // Disk size
    s.append(QString(QObject::tr(" - %1 MB")).arg(this->size / 1024 / 1024));

    // FS
    s.append("\n");
    if (fs.length()) {
        s.append(QString("%1 Volume").arg(fs));
    }
    else {
        s.append(QString("Not formatted"));
    }

    return s;
}

bool RemovableDevice::isAvailable()
{
    bool result = false;
#ifdef WIN32

    HANDLE hDisk;

    hDisk = CreateFileW(reinterpret_cast<LPWSTR>(this->path.data()),
        0,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
        NULL);

    if (hDisk != INVALID_HANDLE_VALUE)
    {
        result = TRUE;
        CloseHandle(hDisk);
    }
#endif

#ifdef Q_OS_MAC
    return true;
#endif

    return result;
}

bool RemovableDevice::open()
{
    bool result = false;
#ifdef WIN32
    this->hDisk = CreateFile(reinterpret_cast<LPWSTR>(this->path.data()),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        0,
        0);

    if (this->hDisk != INVALID_HANDLE_VALUE)
        result = TRUE;
#endif

#ifdef Q_OS_MAC
    system(QString("diskutil umount %1").arg(path).toUtf8().data());
    qDebug() << path;
    return true;
#endif
    return result;
}

bool RemovableDevice::close()
{
    bool result = false;
#ifdef WIN32
    result = CloseHandle(this->hDisk);
#endif
    return result;
}


bool RemovableDevice::isMounted()
{
    bool result = false;
#ifdef WIN32
    if (this->mount.length()) {
        if (GetDiskFreeSpaceEx(reinterpret_cast<LPCWSTR>(this->mount.data()), NULL, NULL, NULL)) {
            result = true;
        }
    }
#endif

#ifdef Q_OS_MAC
    return true;
#endif

    return result;
}

void RemovableDevice::dump(QString &fname, qint64 begin, qint64 size)
{
    qDebug() << "RemovableDevice::dump(" << fname << "," << begin << "," << size << ");";
#ifdef WIN32

    // set file pointer to start
    LARGE_INTEGER i = {0};
    i.QuadPart = (LONGLONG)begin;
    SetFilePointerEx(hDisk, i, NULL, FILE_BEGIN);

    // Create buffer
    int bufferSize = 1024 * 1024;
    int needToRead = (int) size;
    if(!needToRead)
        return;

    DWORD bytesRead;
    char *buffer = (char*)malloc(bufferSize * sizeof(char));

    // Read
    QFile file(fname);
    if (file.open(QIODevice::WriteOnly)) {
        while (needToRead > 0) {
            DWORD toRead = needToRead > bufferSize ? bufferSize : needToRead;

            qDebug() << "ReadFile(" << toRead << ")";

            if (ReadFile(hDisk, buffer, bufferSize, &bytesRead, 0)) {
                if (!bytesRead)
                    break;

                needToRead = needToRead - bytesRead;
                file.write(buffer, toRead);
            }
            else {
                break;
            }
        }

        file.close();
    }

    free(buffer);


#endif

#ifdef Q_OS_MAC
    FILE *f = fopen(path.toUtf8().data(), "r");
    fseek(f, begin, SEEK_SET);

    // Create buffer
    int bufferSize = 1024 * 1024;
    int needToRead = (int) size;
    if(!needToRead)
        return;

    qint32 bytesRead;
    bool ret;

    char *buffer = (char*)malloc(bufferSize * sizeof(char));

    // Read
    QFile file(fname);
    if (file.open(QIODevice::WriteOnly)) {
        while (needToRead > 0) {
            qint32 toRead = needToRead > bufferSize ? bufferSize : needToRead;

            qDebug() << "ReadFile(" << toRead << ")";

            if (!feof(f)) {
                bytesRead = fread(buffer, 1, toRead, f);
                if (!bytesRead)
                    break;

                    needToRead = needToRead - bytesRead;
                    file.write(buffer, toRead);
            }
            else {
                break;
            }
        }

    fclose(f);
    }

    free(buffer);


#endif
}
