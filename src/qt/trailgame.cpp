// SPDX-License-Identifier: GPL-3.0-or-later
#include "trailgame.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
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

// ---------------------------------------------------------------- PixelView

PixelView::PixelView(QWidget *parent) : QWidget(parent), image(art::newCanvas())
{
    setMinimumSize(art::W * 2, art::H * 2);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QRect PixelView::target() const
{
    // Fit the canvas, keeping the aspect ratio (nearest-neighbour keeps it crisp).
    double s = std::min(double(width()) / art::W, double(height()) / art::H);
    int w = int(art::W * s), h = int(art::H * s);
    return QRect((width() - w) / 2, (height() - h) / 2, w, h);
}

QPoint PixelView::toCanvas(QPoint p) const
{
    QRect r = target();
    return QPoint((p.x() - r.x()) * art::W / std::max(1, r.width()), (p.y() - r.y()) * art::H / std::max(1, r.height()));
}

void PixelView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(target(), image);
}

void PixelView::mouseMoveEvent(QMouseEvent *e) { emit moved(toCanvas(e->pos())); }
void PixelView::mousePressEvent(QMouseEvent *e) { emit clicked(toCanvas(e->pos())); }

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
    view_ = new PixelView;
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

    connect(view_, &PixelView::moved, this, [this](QPoint p) { aim_ = p; });
    connect(view_, &PixelView::clicked, this, [this](QPoint p) {
        if (mode_ != Mode::Hunt || huntBullets_ <= 0) return;
        aim_ = p;
        --huntBullets_;
        ++huntShots_;
        if (huntBullets_ == 0) huntEnd_ = std::min(huntEnd_, huntTicks_ + 15);
        for (auto &b : beasts_) {
            if (!b.alive) continue;
            art::Sprite s = art::animalSize(b.kind);
            QRect r(int(b.x) - 3, int(b.y) - 3, s.w + 6, s.h + 6);
            if (r.contains(p)) {
                b.alive = false;
                meat_ += b.kind == art::Animal::Rabbit ? 2 : b.kind == art::Animal::Deer ? 60 + int(rng_.next() * 40) : 350;
                break;
            }
        }
    });

    timer_ = new QTimer(this);
    timer_->setInterval(90);
    connect(timer_, &QTimer::timeout, this, &TrailGame::tick);
}

TrailGame::~TrailGame() = default;

bool TrailGame::setup()
{
    const QStringList profs = {"Banker from Boston ($1600)", "Carpenter from Ohio ($800)", "Farmer from Illinois ($400)"};
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

void TrailGame::deaths(const std::vector<Note> &notes)
{
    static const QRegularExpression re("^(.*) has died of (.*)\\.$");
    for (auto &n : notes) {
        auto m = re.match(QString::fromStdString(n.text));
        if (!m.hasMatch()) continue;
        QImage img = art::newCanvas();
        art::tombstone(img, m.captured(1), m.captured(2), QString::fromStdString(t_->dateString()));
        going_ = false;
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
    if (mode_ == Mode::Hunt) {
        tickHunt();
        return;
    }
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
    refresh();
    if (t_->wiped) { defeat(); return; }
    if (t_->won) { victory(); return; }
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
                        "Score (%3 multiplier): %4\n\nChoose Game > New Game to play again.")
                    .arg(QString::fromStdString(t_->dateString())).arg(alive)
                    .arg(t_->profession == Profession::Banker ? "x1" : t_->profession == Profession::Carpenter ? "x2" : "x3")
                    .arg(t_->score()));
}

void TrailGame::defeat()
{
    over_ = true;
    going_ = false;
    refresh();
    QImage img = art::newCanvas();
    art::tombstone(img, QString::fromStdString(t_->party[0].name), "the hardships of the trail",
                   QString::fromStdString(t_->dateString()));
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
    mode_ = Mode::Hunt;
    beasts_.clear();
    huntBullets_ = std::min(t_->sup.bullets, 20);
    huntShots_ = 0;
    huntTicks_ = 0;
    huntEnd_ = 500;
    meat_ = 0;
    for (int i = 0; i < 3; ++i) spawnBeast();
    refresh();
    setFocus();
}

void TrailGame::spawnBeast()
{
    double k = rng_.next();
    art::Animal a = k < 0.5 ? art::Animal::Rabbit : k < 0.85 ? art::Animal::Deer : art::Animal::Bison;
    double speed = a == art::Animal::Rabbit ? 3.2 : a == art::Animal::Deer ? 2.4 : 1.1;
    bool fromLeft = rng_.next() < 0.5;
    art::Sprite s = art::animalSize(a);
    Beast b{a, double(fromLeft ? -s.w : art::W), 118 + rng_.next() * 50, fromLeft ? speed : -speed};
    beasts_.push_back(b);
}

void TrailGame::tickHunt()
{
    ++huntTicks_;
    ++frame_;
    for (auto &b : beasts_) {
        b.x += b.vx;
        if (b.vx > 0 ? b.x > art::W + 20 : b.x < -60) b.alive = false;
    }
    if (huntTicks_ % 25 == 0) spawnBeast();
    drawHunt();
    // 45 seconds, or a moment after the last bullet.
    if (huntTicks_ >= huntEnd_)
        endHunt();
}

void TrailGame::drawHunt()
{
    QImage &img = view_->image;
    art::huntBackdrop(img, terrainAt(int(t_->miles)));
    QPainter p(&img);
    for (auto &b : beasts_)
        if (b.alive) art::drawAnimal(p, b.kind, int(b.x), int(b.y), b.vx > 0, frame_ / 2);
    art::drawCrosshair(p, aim_.x(), aim_.y());
    p.setPen(art::White);
    QFont f("monospace");
    f.setPixelSize(9);
    p.setFont(f);
    p.fillRect(0, 0, art::W, 12, art::Black);
    p.drawText(4, 9, QString("BULLETS %1   MEAT %2 LB   TIME %3   [Enter] done")
                         .arg(huntBullets_).arg(int(meat_)).arg(std::max(0, (huntEnd_ - huntTicks_) * 90 / 1000)));
    view_->update();
}

void TrailGame::endHunt()
{
    mode_ = Mode::Travel;
    int used = huntShots_;
    auto notes = t_->huntDay(meat_, used);
    if (meat_ > 200) notes.insert(notes.begin(), {"That's more than the wagon can carry; you leave the rest behind.", false});
    absorb(notes);
    deaths(notes);
    refresh();
    if (t_->wiped) defeat();
}

// Keyboard: space toggles travel; Enter ends a hunt.
void TrailGame::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Space && mode_ == Mode::Travel)
        toggleGo();
    else if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && mode_ == Mode::Hunt)
        endHunt();
    else
        QWidget::keyPressEvent(e);
}

