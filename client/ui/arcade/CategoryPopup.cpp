#include "CategoryPopup.h"
#include "../theme/ThemeEngine.h"
#include "../widgets/TitleBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QComboBox>
#include <QLineEdit>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

CategoryPopup::CategoryPopup(const ArcadeCategory& category, const QList<QString>& onlineContacts, QWidget* parent)
    : QDialog(parent)
    , m_category(category)
    , m_onlineContacts(onlineContacts)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(380, 240);

    setupUI();
}

void CategoryPopup::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sleek minimalist header instead of full title bar
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(24, 16, 24, 0);
    
    auto* headerTitle = new QLabel(QString("SHARE %1 ACTIVITY").arg(m_category.name.toUpper()));
    headerTitle->setStyleSheet(QString("color: rgba(255, 255, 255, 180); font-weight: 900; letter-spacing: 3px; font-size: 11px;"));
    
    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(24, 24);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 15);
            color: white;
            border-radius: 12px;
            font-weight: bold;
        }
        QPushButton:hover { background: rgba(255, 255, 255, 30); }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(headerLayout);

    auto* contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(16);

    // Icon & Title
    auto* titleRow = new QHBoxLayout();
    auto* iconLbl = new QLabel();
    if (m_category.icon.startsWith(":/")) {
        QPixmap pix(m_category.icon);
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QRgb pixel = img.pixel(x, y);
                if (qRed(pixel) < 30 && qGreen(pixel) < 30 && qBlue(pixel) < 30) {
                    img.setPixel(x, y, qRgba(0, 0, 0, 0));
                }
            }
        }
        iconLbl->setPixmap(QPixmap::fromImage(img).scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        iconLbl->setText(m_category.icon);
        iconLbl->setStyleSheet("font-size: 32px; background: transparent;");
    }
    
    auto* titleLbl = new QLabel(QString("Share %1 Activity").arg(m_category.name));
    titleLbl->setFont(TE::fontTitle());
    titleLbl->setStyleSheet("color: #FFFFFF; background: transparent; font-weight: 800;");
    
    titleRow->addWidget(iconLbl);
    titleRow->addWidget(titleLbl);
    titleRow->addStretch();
    contentLayout->addLayout(titleRow);

    // Input
    m_inputField = new QLineEdit();
    m_inputField->setPlaceholderText("What are you currently doing?");
    m_inputField->setFixedHeight(44);
    m_inputField->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: rgba(0,0,0,50);
            border: 2px solid rgba(255,255,255,20);
            border-radius: %1px;
            padding: 0 12px;
            color: #FFFFFF;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 2px solid %2;
            background-color: rgba(0,0,0,70);
        }
    )").arg(TE::rMd).arg(m_category.colorHex));
    contentLayout->addWidget(m_inputField);

    contentLayout->addStretch();

    contentLayout->addStretch();

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setFixedSize(80, 32);
    cancelBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: transparent;
            color: %1;
            border: none;
        }
        QPushButton:hover { background: rgba(255,255,255,10); border-radius: %2px; }
    )").arg(TE::onSurface2().name()).arg(TE::rSm));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* shareBtn = new QPushButton("Share Activity");
    shareBtn->setFixedSize(140, 36);
    shareBtn->setCursor(Qt::PointingHandCursor);
    shareBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: %2px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover { background-color: %3; }
    )").arg(m_category.colorHex)
       .arg(TE::rSm)
       .arg(QColor(m_category.colorHex).lighter(115).name()));
    
    connect(shareBtn, &QPushButton::clicked, this, [this]() {
        if (m_inputField->text().trimmed().isEmpty()) return;
        emit broadcastRichPresenceRequested(m_category, m_inputField->text());
        accept();
    });

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(shareBtn);
    contentLayout->addLayout(btnLayout);

    mainLayout->addLayout(contentLayout);
}

void CategoryPopup::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 24, 24);

    // Unify with GameSelectionPopup gradient
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(40, 45, 60, 245));
    grad.setColorAt(1, QColor(20, 25, 35, 245));
    p.fillPath(path, grad);
    
    p.setPen(QPen(QColor(255, 255, 255, 40), 1));
    p.drawPath(path);
}

} // namespace wizz::ui::arcade
