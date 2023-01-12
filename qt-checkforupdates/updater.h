#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = 0);
    void checkForUpdates();

private:
    QNetworkAccessManager manager;
    QNetworkReply *reply;

    QTimer timer;

private slots:
    void httpReadyRead();
    void httpFinished();
    void httpTimeout();

signals:
    void netStarted();
    void netFinished();

    void updateIsAvailable();
    void updateIsNotAvailable();
    void updateError();

public slots:

};

#endif // UPDATER_H
