#include "updater.h"
#include "version.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

#include <QMessageBox>
#include <QDebug>

#define CHECK_FOR_UPDATES_URL "http://example.com/api/1/updates/check"

Updater::Updater(QObject *parent) :
    QObject(parent)
{
}

void Updater::checkForUpdates()
{
    QString currentVersion = QString::number(VER_NUMERIC);

    QUrl params;

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    params.addQueryItem("version", currentVersion);
#else
    QUrlQuery q;
    q.addQueryItem("version", currentVersion);
#endif

    emit netStarted();

    QNetworkRequest req(QUrl(CHECK_FOR_UPDATES_URL));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    reply = manager.post(req, params.encodedQuery());
#else
    QByteArray postData;
    postData.append(params.query());
    reply = manager.post(req, postData);
#endif

    connect(reply, SIGNAL(finished()), this, SLOT(httpFinished()));

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(httpTimeout()));
    timer.setInterval(5000);
    timer.start();
}

void Updater::httpReadyRead()
{

}

void Updater::httpFinished()
{
    timer.stop();
    emit netFinished();

    if (reply->error() == QNetworkReply::NoError) {
        QString data = reply->readAll();
        if (data == "yes") {
            emit updateIsAvailable();
        }
        else {
            emit updateIsNotAvailable();
            return;
        }
    }
    else {
        emit updateError();
    }
}

void Updater::httpTimeout()
{
    disconnect(reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
    disconnect(reply, SIGNAL(finished()), this, SLOT(httpFinished()));

    // abort request
    reply->abort();

    // show error
    emit netFinished();
    emit updateError();
}

