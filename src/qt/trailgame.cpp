// SPDX-License-Identifier: GPL-3.0-or-later
#include "trailgame.h"
#include "graveyard.h"
#include "shootview.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

using namespace trail;

// ---------------------------------------------------------------- dialogs

namespace {

QPixmap pix(const QImage &img, int scale = 3)
{
    return QPixmap::fromImage(img.scaled(img.size() * scale, Qt::KeepAspectRatio, Qt::FastTransformation));
}

QFont monoFont(int pt = 11)
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(pt);
    return f;
}

// Store: used at Independence and the forts.
class StoreDialog : public QDialog {
public:
    StoreDialog(Trail &t, const QString &title, QWidget *parent) : QDialog(parent), t_(t)
    {
        setWindowTitle(title);
        auto *grid = new QGridLayout;
        const struct { Item it; const char *label; } rows[] = {
            {Item::Oxen, "Oxen (yokes of 2)"}, {Item::Food, "Food (pounds)"}, {Item::Clothing, "Clothing (sets)"},
            {Item::Bullets, "Ammunition (boxes of 20)"}, {Item::Wheel, "Spare wagon wheels"},
            {Item::Axle, "Spare wagon axles"}, {Item::Tongue, "Spare wagon tongues"}};
        int r = 0;
        for (auto &row : rows) {
            auto *spin = new QSpinBox;
            spin->setRange(0, 5000);
            spin->setFixedWidth(90);
            connect(spin, &QSpinBox::valueChanged, this, [this] { update(); });
            grid->addWidget(new QLabel(row.label), r, 0);
            grid->addWidget(new QLabel(QString("$%1").arg(t.price(row.it), 0, 'f', 2)), r, 1);
            grid->addWidget(spin, r, 2);
            grid->addWidget(new QLabel(QString("have %1").arg(have(row.it))), r, 3);
            items_.push_back({row.it, spin});
            ++r;
        }
        total_ = new QLabel;
        buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
        auto *lay = new QVBoxLayout(this);
        lay->addLayout(grid);
        lay->addWidget(total_);
        lay->addWidget(buttons_);
        update();
    }

    void apply()
    {
        for (auto &[it, spin] : items_)
            if (spin->value() > 0) t_.buy(it, spin->value());
    }

private:
    int have(Item it) const
    {
        switch (it) {
        case Item::Oxen: return t_.sup.oxen;
        case Item::Food: return int(t_.sup.food);
        case Item::Clothing: return t_.sup.clothing;
        case Item::Bullets: return t_.sup.bullets;
        case Item::Wheel: return t_.sup.wheels;
        case Item::Axle: return t_.sup.axles;
        default: return t_.sup.tongues;
        }
    }
    void update()
    {
        double sum = 0;
        for (auto &[it, spin] : items_) sum += t_.price(it) * spin->value();
        bool ok = sum <= t_.sup.cash + 1e-9;
        total_->setText(QString("Total: $%1     Cash: $%2%3")
                            .arg(sum, 0, 'f', 2).arg(t_.sup.cash, 0, 'f', 2).arg(ok ? "" : "   (too much!)"));
        buttons_->button(QDialogButtonBox::Ok)->setEnabled(ok);
    }

    Trail &t_;
    std::vector<std::pair<Item, QSpinBox *>> items_;
    QLabel *total_;
    QDialogButtonBox *buttons_;
};

void showPicture(QWidget *parent, const QImage &img, const QString &title, const QString &text)
{
    QDialog d(parent);
    d.setWindowTitle(title);
    auto *lay = new QVBoxLayout(&d);
    auto *pic = new QLabel;
    pic->setPixmap(pix(img));
    lay->addWidget(pic, 0, Qt::AlignHCenter);
    auto *lbl = new QLabel(text);
    lbl->setWordWrap(true);
    lbl->setMaximumWidth(art::W * 3);
    lay->addWidget(lbl);
    auto *ok = new QPushButton("OK");
    ok->setDefault(true);
    QObject::connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    lay->addWidget(ok);
    d.exec();
}

} // namespace

// ---------------------------------------------------------------- TrailGame

TrailGame::TrailGame(QWidget *parent) : QWidget(parent)
{
    view_ = new ShootView;
    info_ = new QLabel;
    info_->setFont(monoFont(10));
    info_->setTextFormat(Qt::PlainText);
    log_ = new QLabel;
    log_->setFont(monoFont(10));
    log_->setMinimumHeight(60);
    log_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto mk = [this](const char *text, void (TrailGame::*slot)()) {
        auto *b = new QPushButton(text);
        b->setFocusPolicy(Qt::NoFocus);
        connect(b, &QPushButton::clicked, this, slot);
        return b;
    };
    go_ = mk("Go  [Space]", &TrailGame::toggleGo);
    pace_ = mk("Pace", &TrailGame::choosePace);
    rations_ = mk("Rations", &TrailGame::chooseRations);
    rest_ = mk("Rest", &TrailGame::restClicked);
    hunt_ = mk("Hunt", &TrailGame::startHunt);
    shop_ = mk("Shop", &TrailGame::shop);
    supplies_ = mk("Supplies", &TrailGame::showSupplies);

    auto *row = new QHBoxLayout;
    for (auto *b : {go_, pace_, rations_, rest_, hunt_, shop_, supplies_}) row->addWidget(b);

    auto *lay = new QVBoxLayout(this);
    lay->addWidget(view_, 1);
    lay->addWidget(info_);
    lay->addWidget(log_);
    lay->addLayout(row);
    setFocusPolicy(Qt::StrongFocus);

    connect(view_, &ShootView::finished, this, &TrailGame::endShooting);

    timer_ = new QTimer(this);
    timer_->setInterval(90);
    connect(timer_, &QTimer::timeout, this, &TrailGame::tick);

}

TrailGame::~TrailGame() = default;

void TrailGame::setDayTickMs(int ms) { timer_->setInterval(ms); }

bool TrailGame::setup()
{
    const QStringList profs = {"Banker from Boston ($1600)", "Doctor ($1200)", "Merchant ($1200)", "Blacksmith ($800)",
                               "Carpenter from Ohio ($800)", "Saddlemaker ($800)", "Farmer from Illinois ($400)",
                               "Teacher ($400)"};
    bool ok = false;
    QString p = QInputDialog::getItem(this, "The Oregon Trail", "Many kinds of people made the trip to Oregon.\nYou may:",
                                      profs, 0, false, &ok);
    if (!ok) return false;
    Profession prof = Profession(profs.indexOf(p));

    QDialog nd(this);
    nd.setWindowTitle("Your party");
    auto *form = new QFormLayout(&nd);
    const char *defaults[5] = {"You", "Anna", "Ben", "Clara", "Daniel"};
    QLineEdit *edits[5];
    for (int i = 0; i < 5; ++i) {
        edits[i] = new QLineEdit(defaults[i]);
        form->addRow(i == 0 ? "Wagon leader:" : QString("Family member %1:").arg(i), edits[i]);
    }
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &nd, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &nd, &QDialog::reject);
    form->addRow(bb);
    if (nd.exec() != QDialog::Accepted) return false;
    std::array<std::string, 5> names;
    for (int i = 0; i < 5; ++i) {
        QString n = edits[i]->text().trimmed();
        names[i] = (n.isEmpty() ? QString(defaults[i]) : n).toStdString();
    }

    const QStringList months = {"March", "April", "May", "June", "July"};
    QString m = QInputDialog::getItem(
        this, "When to leave",
        "It is 1848. Your jumping-off place is Independence, Missouri.\n"
        "Leave too early and the weather is bitter; too late and the mountains\n"
        "will be buried in snow before you reach Oregon.\n\nIn which month do you leave?",
        months, 1, false, &ok);
    if (!ok) return false;

    t_ = std::make_unique<Trail>(rng_, prof, 3 + months.indexOf(m), names);

    // Tombstones from earlier games stand along the trail (this game's own are added as they happen).
    yard_.load();
    std::vector<trail::Grave> ahead;
    for (const Tombstone &gr : yard_.graves())
        ahead.push_back({gr.name.toStdString(), gr.cause.toStdString(), gr.date.toStdString(), gr.epitaph.toStdString(), gr.mile});
    t_->setGraves(std::move(ahead));

    for (;;) {
        StoreDialog sd(*t_, "Matt's General Store, Independence", this);
        if (sd.exec() == QDialog::Accepted) sd.apply();
        if (t_->sup.oxen < 2) {
            QMessageBox::warning(this, "Matt's General Store", "You need oxen to pull the wagon. Buy at least one yoke.");
            continue;
        }
        if (t_->sup.food < 50 &&
            QMessageBox::question(this, "Matt's General Store",
                                  "You have very little food. Leave anyway?") != QMessageBox::Yes)
            continue;
        break;
    }
    return true;
}

void TrailGame::newGame()
{
    timer_->stop();
    mode_ = Mode::Idle;
    going_ = false;
    over_ = false;
    recent_.clear();
    leaderGrave_ = QImage();
    scroll_ = 0;
    if (!setup()) {
        if (!t_) return;
    }
    mode_ = Mode::Travel;
    absorb({{"You set out from Independence, Missouri.", false}});
    refresh();
    timer_->start();
    setFocus();
}

void TrailGame::absorb(const std::vector<Note> &notes)
{
    for (auto &n : notes) recent_.push_back(QString::fromStdString(n.text));
    while (recent_.size() > 3) recent_.removeFirst();
    log_->setText(recent_.join("\n"));
}

// The Apple II asked the player what to put on the stone; so do we.
QImage TrailGame::grave(const QString &name, const QString &cause)
{
    QInputDialog ask(this);
    ask.setWindowTitle("Tombstone");
    ask.setInputMode(QInputDialog::TextInput);
    ask.setLabelText(QString("%1 has died of %2.\n\nWould you like to carve something on the tombstone?\n"
                             "What do you want it to say? (leave blank for nothing)").arg(name, cause));
    if (auto *edit = ask.findChild<QLineEdit *>()) edit->setMaxLength(40);
    QString epitaph;
    if (ask.exec() == QDialog::Accepted) epitaph = ask.textValue().left(40);
    const QString date = QString::fromStdString(t_->dateString());
    QImage img = art::newCanvas();
    art::tombstone(img, name, cause, date, epitaph);
    // Keep it: later games will find this stone on the trail, where it happened.
    yard_.add({name, cause, date, epitaph, t_->burialMile(cause.toStdString())});
    yard_.save();
    return img;
}

// An event from the engine: the wagon has passed a tombstone left by an earlier game.
void TrailGame::visitGraves()
{
    for (const auto &gr : t_->takePassedGraves()) {
        going_ = false;                                   // stop and look, as for any event
        const QString epitaph = QString::fromStdString(gr.epitaph);
        QImage img = art::newCanvas();
        art::tombstone(img, QString::fromStdString(gr.name), QString::fromStdString(gr.cause),
                       QString::fromStdString(gr.date), epitaph);
        QString text = QString("A tombstone stands beside the trail, near mile %1.").arg(gr.mile);
        if (!epitaph.isEmpty()) text += QString("\n\nIt reads: \"%1\"").arg(epitaph);
        showPicture(this, img, "A grave beside the trail", text);
    }
}

void TrailGame::deaths(const std::vector<Note> &notes)
{
    static const QRegularExpression re("^(.*) has died of (.*)\\.$");
    for (auto &n : notes) {
        auto m = re.match(QString::fromStdString(n.text));
        if (!m.hasMatch()) continue;
        going_ = false;
        QImage img = grave(m.captured(1), m.captured(2));
        if (!t_->party[0].alive && leaderGrave_.isNull() && m.captured(1) == QString::fromStdString(t_->party[0].name))
            leaderGrave_ = img;
        showPicture(this, img, "Sad news", QString("%1 has died of %2.").arg(m.captured(1), m.captured(2)));
    }
}

void TrailGame::refresh()
{
    if (!t_) return;
    const Trail &t = *t_;
    QString party;
    for (auto &m : t.party)
        party += QString("%1 (%2)  ").arg(QString::fromStdString(m.name), m.alive ? name(m.health()) : "dead");
    QString next = t.won ? "" : QString("%1 (%2 mi)")
        .arg(QString::fromLatin1(landmarks()[std::min<size_t>(t.nextIndex(), landmarks().size() - 1)].name))
        .arg(t.milesToNext());
    info_->setText(QString("%1   weather: %2 (%3°F)\n"
                           "pace: %4   rations: %5   next: %6   traveled: %7 mi\n"
                           "food: %8 lb   oxen: %9   clothes: %10   bullets: %11   spares W/A/T: %12/%13/%14   cash: $%15\n"
                           "%16")
                       .arg(QString::fromStdString(t.dateString()), name(t.weather)).arg(int(t.tempF))
                       .arg(name(t.pace), name(t.rations), next).arg(int(t.miles))
                       .arg(int(t.sup.food)).arg(t.sup.oxen).arg(t.sup.clothing).arg(t.sup.bullets)
                       .arg(t.sup.wheels).arg(t.sup.axles).arg(t.sup.tongues).arg(t.sup.cash, 0, 'f', 2)
                       .arg(party));
    bool traveling = mode_ == Mode::Travel && !over_;
    go_->setText(going_ ? "Stop  [Space]" : "Go  [Space]");
    for (auto *b : {go_, pace_, rations_, rest_, hunt_, supplies_}) b->setEnabled(traveling);
    shop_->setEnabled(traveling && t.atLandmark() &&
                      (t.here().kind == Kind::Fort || t.here().kind == Kind::Start));
}

void TrailGame::tick()
{
    if (mode_ == Mode::Shoot) return;   // the ShootView drives hunts and fights
    if (mode_ != Mode::Travel || !t_) return;
    ++frame_;
    if (going_) {
        scroll_ += 2.4;
        if (++dayTicks_ >= 6) {
            dayTicks_ = 0;
            stepDay();
        }
    }
    if (mode_ != Mode::Travel) return;
    if (t_->atLandmark() && !going_)
        art::landmarkScene(view_->image, t_->here(), frame_ / 3);
    else
        art::travelScene(view_->image, terrainAt(int(t_->miles)), t_->weather, scroll_, going_ ? frame_ / 2 : 0);
    view_->update();
}

void TrailGame::stepDay()
{
    auto notes = t_->travelDay();
    absorb(notes);
    deaths(notes);
    visitGraves();
    refresh();
    if (t_->wiped) { defeat(); return; }
    if (t_->won) { victory(); return; }
    if (t_->pendingEncounter() != trail::Encounter::None) {
        going_ = false;
        startFight();
        return;
    }
    if (!notes.empty() && notes.front().bad && !t_->atLandmark()) going_ = false;   // trouble: stop and look
    if (t_->atLandmark()) {
        going_ = false;
        refresh();
        if (t_->here().kind == Kind::River) riverArrival();
        else landmarkArrival();
    }
}

void TrailGame::toggleGo()
{
    if (mode_ != Mode::Travel || over_) return;
    going_ = !going_;
    if (going_ && t_->atLandmark()) t_->leave();
    refresh();
}

void TrailGame::choosePace()
{
    const QStringList items = {"steady", "strenuous", "grueling"};
    bool ok;
    QString s = QInputDialog::getItem(this, "Pace", "How fast do you want to travel?\n(Faster is more tiring.)", items,
                                      int(t_->pace), false, &ok);
    if (ok) t_->pace = Pace(items.indexOf(s));
    refresh();
}

void TrailGame::chooseRations()
{
    const QStringList items = {"filling (3 lb each per day)", "meager (2 lb)", "bare bones (1 lb)"};
    bool ok;
    QString s = QInputDialog::getItem(this, "Rations", "How much should everyone eat?", items, int(t_->rations), false, &ok);
    if (ok) t_->rations = Rations(items.indexOf(s));
    refresh();
}

void TrailGame::restClicked()
{
    bool ok;
    int d = QInputDialog::getInt(this, "Rest", "How many days do you want to rest?", 2, 1, 9, 1, &ok);
    if (!ok) return;
    going_ = false;
    auto notes = t_->rest(d);
    absorb(notes);
    deaths(notes);
    refresh();
    if (t_->wiped) defeat();
}

void TrailGame::shop()
{
    StoreDialog sd(*t_, QString("Store at %1").arg(t_->here().name), this);
    if (sd.exec() == QDialog::Accepted) sd.apply();
    refresh();
}

void TrailGame::showSupplies()
{
    const Supplies &s = t_->sup;
    QMessageBox::information(this, "Supplies",
                             QString("oxen: %1\nfood: %2 lb\nclothing: %3 sets\nbullets: %4\nspare wheels: %5\n"
                                     "spare axles: %6\nspare tongues: %7\ncash: $%8")
                                 .arg(s.oxen).arg(int(s.food)).arg(s.clothing).arg(s.bullets).arg(s.wheels)
                                 .arg(s.axles).arg(s.tongues).arg(s.cash, 0, 'f', 2));
}

void TrailGame::landmarkArrival()
{
    QImage img = art::newCanvas();
    art::landmarkScene(img, t_->here(), 0);
    view_->image = img;
    view_->update();
    const Landmark &l = t_->here();
    QString text = QString("%1\n%2").arg(QString::fromLatin1(l.name), QString::fromStdString(t_->dateString()));
    if (l.kind == Kind::Fort) text += "\n\nYou can shop here (prices are higher the further west you go).";
    showPicture(this, img, "Landmark", text);
    if (l.kind == Kind::Fort) shop();
}

void TrailGame::riverArrival()
{
    for (;;) {
        QImage img = art::newCanvas();
        art::landmarkScene(img, t_->here(), 0);
        view_->image = img;
        view_->update();
        const River &r = t_->river();

        QDialog d(this);
        d.setWindowTitle(QString::fromLatin1(t_->here().name));
        auto *lay = new QVBoxLayout(&d);
        auto *pic = new QLabel;
        pic->setPixmap(pix(img));
        lay->addWidget(pic, 0, Qt::AlignHCenter);
        lay->addWidget(new QLabel(QString("%1\nRiver width: %2 feet     Depth: %3 feet\n\nHow will you cross?")
                                      .arg(QString::fromStdString(t_->dateString()))
                                      .arg(int(r.widthFt)).arg(r.depthFt, 0, 'f', 1)));
        Method chosen = Method::Wait;
        auto add = [&](const QString &label, Method m, bool enabled = true) {
            auto *b = new QPushButton(label);
            b->setEnabled(enabled);
            connect(b, &QPushButton::clicked, &d, [&, m] { chosen = m; d.accept(); });
            lay->addWidget(b);
        };
        add("Ford the river", Method::Ford);
        add("Caulk the wagon and float it across", Method::Caulk);
        add(QString("Take the ferry ($%1)").arg(r.ferryCost, 0, 'f', 0), Method::Ferry, t_->sup.cash >= r.ferryCost);
        add("Wait a day", Method::Wait);
        d.exec();

        auto notes = t_->cross(chosen);
        absorb(notes);
        deaths(notes);
        refresh();
        if (t_->wiped) { defeat(); return; }
        if (!t_->atLandmark()) {
            QString msg;
            for (auto &n : notes) msg += QString::fromStdString(n.text) + "\n";
            QMessageBox::information(this, "River crossing", msg.trimmed());
            return;
        }
    }
}

void TrailGame::victory()
{
    over_ = true;
    going_ = false;
    QImage img = art::newCanvas();
    art::oregonCity(img);
    view_->image = img;
    view_->update();
    refresh();
    int alive = t_->livingCount();
    showPicture(this, img, "Oregon City",
                QString("Congratulations! You have made it to Oregon!\n\nArrived: %1\nSurvivors: %2 of 5\n"
                        "Score (x%3 for a %5): %4\n\nChoose Game > New Game to play again.")
                    .arg(QString::fromStdString(t_->dateString())).arg(alive)
                    .arg(t_->scoreMultiplier()).arg(t_->score()).arg(name(t_->profession)));
}

void TrailGame::defeat()
{
    over_ = true;
    going_ = false;
    refresh();
    // The leader's own tombstone was shown when they died; if the party was lost another
    // way (the winter snows), the leader gets one now.
    QImage img = leaderGrave_.isNull() ? grave(QString::fromStdString(t_->party[0].name), "exposure") : leaderGrave_;
    view_->image = img;
    view_->update();
    showPicture(this, img, "The end of the trail",
                QString("Your journey ends after %1 miles.\n\nChoose Game > New Game to try again.").arg(int(t_->miles)));
}

// ---------------------------------------------------------------- hunting

void TrailGame::startHunt()
{
    if (t_->sup.bullets < 5) {
        QMessageBox::information(this, "Hunting", "You need at least a few bullets to go hunting.");
        return;
    }
    going_ = false;
    QMessageBox::information(this, "Hunting",
                             "Game animals are about: rabbits, deer and bison. They bolt when you get close.\n\n"
                             "Walk with W A S D, turn with the mouse (or Q/E, arrows), fire with a click or Space.\n"
                             "Bullets take time to arrive, so lead what's running. Trees and bushes stop bullets.\n"
                             "Press Enter when you're done; the wagon can carry 200 lb of meat home.");
    beginShooting(std::make_unique<fight::Field>(fight::Field::HuntTag{}, std::min(t_->sup.bullets, 20), rng_));
}

// Keyboard: space toggles travel; Enter ends a hunt.
void TrailGame::keyPressEvent(QKeyEvent *e)
{
    if (mode_ == Mode::Shoot) {
        if (!view_->handleKey(e, true)) QWidget::keyPressEvent(e);
        return;
    }
    if (e->key() == Qt::Key_Space && mode_ == Mode::Travel)
        toggleGo();
    else
        QWidget::keyPressEvent(e);
}

void TrailGame::keyReleaseEvent(QKeyEvent *e)
{
    if (mode_ == Mode::Shoot && view_->handleKey(e, false)) return;
    QWidget::keyReleaseEvent(e);
}

// ---------------------------------------------------------------- shooting (fights and hunts)

void TrailGame::beginShooting(std::unique_ptr<fight::Field> field)
{
    mode_ = Mode::Shoot;
    view_->begin(std::move(field));
    refresh();
    setFocus();
}

void TrailGame::startFight()
{
    const bool bandits = t_->pendingEncounter() == Encounter::Bandits;
    const int total = t_->encounterSize();
    if (t_->sup.bullets < 5) {
        QMessageBox::warning(this, bandits ? "Bandits!" : "Wolves!",
                             QString(bandits ? "Bandits attack, " : "Wolves attack, ") +
                                 "and you have almost no bullets to fight back with!");
        auto notes = t_->resolveEncounter(0, 0);
        absorb(notes);
        deaths(notes);
        refresh();
        if (t_->wiped) defeat();
        return;
    }
    QMessageBox::information(this, bandits ? "Bandits!" : "Wolves!",
                             QString(bandits ? "%1 bandits are closing in on the wagon from every side!"
                                             : "A pack of %1 wolves is closing in from every side!").arg(total) +
                                 "\n\nWalk with W A S D, turn with the mouse (or Q/E, arrows), fire with a click or Space.\n"
                                 "Bullets take time to arrive, so lead fast targets. Trees and bushes stop bullets.\n"
                                 "Arrows at the screen edge point to enemies you can't see. Don't let them reach the wagon.\n" +
                                 QString(bandits ? "The bandits have guns and shoot back: keep moving, three hits and you're down." : ""));
    beginShooting(std::make_unique<fight::Field>(bandits ? fight::Kind::Bandit : fight::Kind::Wolf, total,
                                                 std::min(t_->sup.bullets, 25), rng_));
}

void TrailGame::endShooting()
{
    auto field = view_->take();
    mode_ = Mode::Travel;
    std::vector<Note> notes;
    if (field->hunting()) {
        notes = t_->huntDay(field->meat, field->shots);
        if (field->meat > 200)
            notes.insert(notes.begin(), {"That's more than the wagon can carry; you leave the rest behind.", false});
    } else {
        notes = t_->resolveEncounter(field->killed, field->shots, field->hits, field->wagonHits);
    }
    absorb(notes);
    deaths(notes);
    refresh();
    if (t_->wiped) defeat();
}
