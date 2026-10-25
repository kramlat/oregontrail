// SPDX-License-Identifier: GPL-3.0-or-later
#include "appwindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("oregontrail");
    AppWindow w;
    w.show();
    // Start a game once the window is up (the setup dialogs need a parent).
    QTimer::singleShot(0, &w, [&w] { QMetaObject::invokeMethod(&w, "playTrail"); });
    return app.exec();
}
