#include "mainwindow.h"
#include "logger.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Informazioni applicazione per QSettings
    QCoreApplication::setOrganizationName("ShellyLogger");
    QCoreApplication::setApplicationName("Shelly 3EM 3-Phase Monitor");
    QCoreApplication::setApplicationVersion("2.5.1");

    // Initialize logging system
    Logger::instance().initialize();
    Logger::instance().setLogLevel(Logger::Info);  // Default: Info level (less verbose than Debug)
    Logger::instance().setConsoleOutput(true);     // Enable console output

    LOG_INFO("=== Shelly 3EM 3-Phase Monitor ===");
    LOG_INFO(QString("Version: %1").arg(QCoreApplication::applicationVersion()));
    LOG_INFO(QString("Qt Version: %1").arg(qVersion()));

    qDebug() << "=== Shelly 3EM 3-Phase Monitor ===";
    qDebug() << "Versione:" << QCoreApplication::applicationVersion();
    qDebug() << "Qt Version:" << qVersion();

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
