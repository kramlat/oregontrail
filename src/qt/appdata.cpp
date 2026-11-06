// SPDX-License-Identifier: GPL-3.0-or-later
#include "appdata.h"

#include <QStandardPaths>

QString appDataDir()
{
    QString dir = qEnvironmentVariable("OREGONTRAIL_DATA_DIR");
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir;
}
