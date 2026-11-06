// SPDX-License-Identifier: GPL-3.0-or-later
#include "appwindow.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("oregontrail");
    AppWindow w;      // opens on the title screen; a game starts only when the player chooses one
    w.show();
    return app.exec();
}
