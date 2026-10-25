// SPDX-License-Identifier: GPL-3.0-or-later
// Drives the real GUI offscreen: auto-answers every dialog, fills the store,
// hits Space to travel, and lets time pass. Passes if nothing crashes or hangs.
#include "qt/trailgame.h"

#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    TrailGame game;
    game.resize(900, 760);
    game.show();

    int dialogs = 0, keys = 0;
    QTimer driver;
    QObject::connect(&driver, &QTimer::timeout, [&] {
        if (QWidget *m = QApplication::activeModalWidget()) {
            ++dialogs;
            const auto spins = m->findChildren<QSpinBox *>();
            if (spins.size() == 7) {                    // the store
                const int want[7] = {4, 900, 5, 6, 1, 1, 1};
                for (int i = 0; i < 7; ++i) spins[i]->setValue(want[i]);
            }
            if (auto *mb = qobject_cast<QMessageBox *>(m)) {
                mb->accept();
            } else if (auto *d = qobject_cast<QDialog *>(m)) {
                // River crossing: pick the first enabled method (Ford); everything else: OK.
                d->accept();
            }
            return;
        }
        // Not in a dialog: keep the wagon rolling.
        if (++keys % 40 == 1) {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " ");
            QApplication::sendEvent(&game, &press);
        }
    });
    driver.start(30);

    if (argc > 2) {   // optional: screenshot a few seconds in
        QTimer::singleShot(6000, [&game, path = QString(argv[2])] { game.grab().save(path); });
    }
    QTimer::singleShot(0, [&] { game.newGame(); });
    QTimer::singleShot(argc > 1 ? QString(argv[1]).toInt() * 1000 : 20000, &app, &QApplication::quit);
    int rc = app.exec();
    std::printf("smoke: ran, %d dialogs auto-answered\n", dialogs);
    return rc;
}
