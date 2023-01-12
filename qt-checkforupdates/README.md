# qt-checkforupdates
Check for updates for Qt application

Sample usage
```
void MainWindow::on_pushButton_clicked()
{
    updater = new Updater();
    connect(updater, SIGNAL(updateIsAvailable()), this, SLOT(updateIsAvailable()));
    connect(updater, SIGNAL(updateIsNotAvailable()), this, SLOT(updateIsNotAvailable()));
    connect(updater, SIGNAL(updateError()), this, SLOT(updateError()));
    updater->checkForUpdates();
}

void MainWindow::updateIsAvailable()
{
    if (QMessageBox::Yes == QMessageBox::information(
                this, "", tr("New version is available. Download update?"),
                QMessageBox::Yes,
                QMessageBox::No)) {
        QDesktopServices::openUrl(QUrl("http://example.com/"));
    }
    this->updater->deleteLater();
}

void MainWindow::updateIsNotAvailable()
{
    QMessageBox::information(this, "", tr("You're currenly running the latest version"));
    this->updater->deleteLater();
}

void MainWindow::updateError()
{
    QMessageBox::information(NULL, "",
                             tr("Error. Please check your network connection and try again."));
    this->updater->deleteLater();
}
```

