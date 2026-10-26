// SPDX-License-Identifier: GPL-3.0-or-later
// Drives the real GUI offscreen: auto-answers every dialog, fills the store,
// hits Space to travel, and lets time pass. Passes if nothing crashes or hangs.
#include "qt/classicgame.h"
#include "qt/graveyard.h"
#include "qt/shootview.h"
#include "qt/trailgame.h"

#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <string>

// Plausible answers to the 1978 game's prompts, by the text on the prompt line.
static std::string stockAnswer(const std::string &p)
{
    auto has = [&](const char *x) { return p.find(x) != std::string::npos; };
    if (has("INSTRUCTIONS")) return "NO";
    if (has("OXEN")) return "250";
    if (has("SPEND ON FOOD")) return "200";
    if (has("AMMUNITION") && has("SPEND ON")) return "50";
    if (has("CLOTHING") && has("SPEND ON")) return "100";
    if (has("MISCELLANEOUS") && has("SPEND ON")) return "50";
    if (has("HUNT")) return "1";
    if (has("NEXT FORT")) return "3";
    if (has("(3) WELL")) return "2";
    if (has("KIN") || has("MINISTER") || has("FUNERAL")) return "YES";
    if (has("FOOD") || has("AMMUNITION") || has("CLOTHING") || has("MISCELLANEOUS")) return "10";
    return "3";
}

// The 1978 text game: answer its prompts from the last line of text, and shoot the rounds.
static int runClassic(QApplication &app, int seconds)
{
    ClassicGame game;
    game.resize(900, 760);
    game.show();
    int answered = 0, shots = 0, tick = 0;
    QTimer driver;
    QObject::connect(&driver, &QTimer::timeout, [&] {
        ++tick;
        auto *view = game.findChild<ShootView *>();
        if (view && view->shooting()) {
            for (int k : {Qt::Key_W, Qt::Key_A, Qt::Key_S, Qt::Key_D, Qt::Key_Q, Qt::Key_E}) {
                QKeyEvent ev(tick % 7 < 3 ? QEvent::KeyPress : QEvent::KeyRelease, k, Qt::NoModifier);
                if ((tick / 7 + k) % 3 == 0) QApplication::sendEvent(view, &ev);
            }
            QPointF pos(20 + (tick * 37) % (view->width() - 40), 20 + (tick * 53) % (view->height() - 40));
            QMouseEvent click(QEvent::MouseButtonPress, pos, view->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(view, &click);
            ++shots;
            return;
        }
        auto *edit = game.findChild<QLineEdit *>();
        auto *log = game.findChild<QPlainTextEdit *>();
        if (!edit || !log || !edit->isEnabled()) return;
        QString last = log->toPlainText().section('\n', -1);
        std::string answer = stockAnswer(last.toStdString());
        // "TOUGH---YOU NEED MORE BULLETS TO GO HUNTING" re-asks until you stop trying to hunt.
        if (log->toPlainText().right(120).contains("CAN'T EAT THAT WELL"))
            answer = "1";                                  // too little food to eat better; eat poorly
        if (log->toPlainText().right(160).contains("TOUGH---YOU NEED MORE BULLETS"))
            answer = last.contains("NEXT FORT") ? "3" : "2";
        if (last.contains("TYPE") || last.isEmpty()) answer = "3";
        edit->setText(QString::fromStdString(answer));
        QKeyEvent ret(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(edit, &ret);
        QMetaObject::invokeMethod(edit, "returnPressed");
        ++answered;
    });
    driver.start(10);
    QTimer::singleShot(0, &game, [&] { game.newGame(); });
    QTimer::singleShot(seconds * 1000, &app, &QApplication::quit);
    int rc = app.exec();
    std::printf("smoke classic: ran, %d prompts answered, %d shooting inputs\n", answered, shots);
    if (auto *log = game.findChild<QPlainTextEdit *>())
        std::printf("---- end of the game text ----\n%s\n", log->toPlainText().right(700).toStdString().c_str());
    return rc;
}

// Plays whole games to their end, starting a new one each time, and reports how they went.
static int runFull(QApplication &app, int seconds)
{
    TrailGame game;
    game.resize(900, 760);
    game.setFastForTests();                    // fast wagon and river animations; fights run in real time
    game.show();
    int won = 0, lost = 0, dialogs = 0, keys = 0, shots = 0;
    QStringList seen;
    QTimer driver;
    QObject::connect(&driver, &QTimer::timeout, [&] {
        if (QWidget *m = QApplication::activeModalWidget()) {
            ++dialogs;
            const QString title = m->windowTitle();
            if (!seen.contains(title) && seen.size() < 60) seen << title;
            const auto spins = m->findChildren<QSpinBox *>();
            if (spins.size() == 7) {
                const int want[7] = {4, 900, 5, 6, 1, 1, 1};
                for (int i = 0; i < 7; ++i) spins[i]->setValue(want[i]);
            }
            bool river = false;
            for (auto *b : m->findChildren<QPushButton *>())
                if (b->text().contains("Ford") && b->isEnabled()) river = true;
            if (river) {                       // sometimes look for a better crossing first; then ferry, else caulk
                static int searches = 0;
                for (auto *b : m->findChildren<QPushButton *>())
                    if (b->text().startsWith("Look for") && b->isEnabled() && searches < 12 && (++searches % 3) == 1) {
                        b->click();
                        return;
                    }
                QPushButton *pick = nullptr;
                for (auto *b : m->findChildren<QPushButton *>()) {
                    if (b->text().contains("ferry") && b->isEnabled()) pick = b;
                    if (!pick && b->text().contains("Caulk")) pick = b;
                }
                if (pick) { pick->click(); return; }
            }
            const bool victory = title == "Oregon City", defeat = title == "The end of the trail";
            if (victory) ++won;
            if (defeat) ++lost;
            if (auto *d = qobject_cast<QDialog *>(m)) d->accept();
            if (victory || defeat) QTimer::singleShot(0, &game, [&game] { game.newGame(); });
            return;
        }
        if (auto *view = game.findChild<ShootView *>(); view && view->shooting()) {
            // Fights and hunts: wander and shoot; end hunts early.
            static const int pool[] = {Qt::Key_W, Qt::Key_A, Qt::Key_S, Qt::Key_D, Qt::Key_Q, Qt::Key_E};
            int k = pool[(keys / 3) % 6];
            QKeyEvent down(QEvent::KeyPress, k, Qt::NoModifier), up(QEvent::KeyRelease, k, Qt::NoModifier);
            QApplication::sendEvent(&game, keys % 3 == 0 ? &down : &up);
            QPointF pos(20 + (keys * 37) % (view->width() - 40), 20 + (keys * 53) % (view->height() - 40));
            QMouseEvent click(QEvent::MouseButtonPress, pos, view->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(view, &click);
            if (view->field() && view->field()->hunting() && keys % 40 == 0) {
                QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                QApplication::sendEvent(&game, &enter);
            }
            ++shots;
            ++keys;
            return;
        }
        if (++keys % 8 == 1) {                 // keep the wagon rolling; it stops at every event
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " ");
            QApplication::sendEvent(&game, &press);
        }
    });
    driver.start(5);
    QTimer::singleShot(0, [&] { game.newGame(); });
    QTimer::singleShot(seconds * 1000, &app, &QApplication::quit);
    int rc = app.exec();
    std::printf("(searched for a better crossing in the GUI)\n");
    std::printf("smoke full: %d games reached Oregon, %d ended on the trail, %d dialogs, %d shooting inputs\n",
                won, lost, dialogs, shots);
    std::printf("dialog titles seen: %s\n", seen.join(" | ").toStdString().c_str());
    return rc;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    // Never touch the real save: use a scratch data dir, pre-seeded with old graves to find.
    QTemporaryDir dataDir;
    qputenv("OREGONTRAIL_DATA_DIR", dataDir.path().toUtf8());
    {
        GraveYard seed;
        for (int mile : {40, 90, 140, 200, 260, 330, 420, 520, 640, 800})
            seed.add({QString("Old%1").arg(mile), "cholera", "May 5, 1848", "Gone west", mile});
        seed.save();
    }
    if (argc > 3 && QString(argv[3]) == "full")
        return runFull(app, argc > 1 ? QString(argv[1]).toInt() : 60);
    if (argc > 3 && QString(argv[3]) == "classic")
        return runClassic(app, argc > 1 ? QString(argv[1]).toInt() : 20);
    TrailGame game;
    game.resize(900, 760);
    game.show();

    int dialogs = 0, keys = 0, clicks = 0;
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
        // Not in a dialog: shoot at random spots (hunts and fights), press Hunt now and
        // then, and keep the wagon rolling.
        if (auto *view = game.findChild<ShootView *>(); view && keys % 2 == 0) {
            QPointF pos(20 + (keys * 37) % (view->width() - 40), 20 + (keys * 53) % (view->height() - 40));
            QMouseEvent click(QEvent::MouseButtonPress, pos, view->mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(view, &click);
            ++clicks;
        }
        {   // wiggle through the fight controls (ignored outside a fight)
            static const int pool[] = {Qt::Key_W, Qt::Key_A, Qt::Key_S, Qt::Key_D, Qt::Key_Q, Qt::Key_E};
            int k = pool[(keys / 3) % 6];
            QKeyEvent down(QEvent::KeyPress, k, Qt::NoModifier), up(QEvent::KeyRelease, k, Qt::NoModifier);
            if (keys % 3 == 0) QApplication::sendEvent(&game, &down);
            else if (keys % 3 == 2) QApplication::sendEvent(&game, &up);
        }
        if (keys % 500 == 250)
            for (auto *b : game.findChildren<QPushButton *>())
                if (b->text() == "Hunt" && b->isEnabled()) b->click();
        if (keys % 90 == 60) {   // finish any hunt in progress
            QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(&game, &enter);
        }
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
    GraveYard after;
    after.load();
    std::printf("smoke: ran, %d dialogs auto-answered, %d clicks, %d graves on file (10 seeded)\n", dialogs, clicks,
                int(after.graves().size()));
    return rc;
}
