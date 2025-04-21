#include "alarmgui.h" // Include the GUI window header
#include <QtWidgets/QApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QDebug> // For qInfo

int main(int argc, char *argv[])
{
    // Optional: Configure logging
    // QLoggingCategory::setFilterRules("*.debug=false\n*.info=true");
    qSetMessagePattern("[%{time hh:mm:ss.zzz} %{type}] %{message}"); // Example format

    QApplication a(argc, argv);
    QApplication::setApplicationName("RTEP_GUI");
    QApplication::setApplicationVersion("0.5"); // Match CMake project version

    qInfo() << "Starting RTEP GUI Application...";

    AlarmGui w; // Create the main window instance
    w.show();

    int result = a.exec(); // Start the Qt event loop

    qInfo() << "Application event loop finished with code:" << result;
    return result;
}