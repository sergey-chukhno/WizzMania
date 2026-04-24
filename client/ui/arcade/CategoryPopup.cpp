#include "CategoryPopup.h"
#include "../theme/ThemeEngine.h"
#include "../widgets/TitleBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QComboBox>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

CategoryPopup::CategoryPopup(const ArcadeCategory& category, const QList<QString>& onlineContacts, QWidget* parent)
    : QDialog(parent)
    , m_category(category)
    , m_onlineContacts(onlineContacts)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(360, 260);

    setupUI();
}

void CategoryPopup::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    auto* titleBar = new TitleBar(m_category.name, this);
    // Don't want maximize/minimize for this small dialog
    titleBar->setStyleSheet("background: transparent;"); // Keep it minimal
    mainLayout->addWidget(titleBar);

    auto* contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(16);

    // Icon & Title
    auto* titleRow = new QHBoxLayout();
    auto* iconLbl = new QLabel(m_category.icon);
    iconLbl->setStyleSheet("font-size: 32px; background: transparent;");
    
    auto* titleLbl = new QLabel(QString("Share %1 Activity").arg(m_category.name));
    titleLbl->setFont(TE::fontTitle());
    titleLbl->setStyleSheet(QString("color: %1; background: transparent;").arg(TE::onSurface().name()));
    
    titleRow->addWidget(iconLbl);
    titleRow->addWidget(titleLbl);
    titleRow->addStretch();
    contentLayout->addLayout(titleRow);

    // Input
    m_inputField = new QLineEdit();
    m_inputField->setPlaceholderText("What are you currently doing?");
    m_inputField->setFixedHeight(40);
    m_inputField->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: rgba(0,0,0,30);
            border: 1px solid rgba(255,255,255,20);
            border-radius: %1px;
            padding: 0 12px;
            color: %2;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid %3;
            background-color: rgba(0,0,0,50);
        }
    )").arg(TE::rMd).arg(TE::onSurface().name()).arg(m_category.colorHex));
    contentLayout->addWidget(m_inputField);

    // Recipient selection (if sharing to chat)
    auto* targetCombo = new QComboBox();
    targetCombo->setFixedHeight(36);
    targetCombo->addItem("Broadcast to Profile (Rich Presence)");
    targetCombo->insertSeparator(1);
    for (const auto& contact : m_onlineContacts) {
        targetCombo->addItem(QString("Share with %1").arg(contact), contact); // Data is username
    }
    targetCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: rgba(0,0,0,30);
            border: 1px solid rgba(255,255,255,20);
            border-radius: %1px;
            padding: 0 12px;
            color: %2;
        }
    )").arg(TE::rSm).arg(TE::onSurface().name()));
    contentLayout->addWidget(targetCombo);

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

    auto* shareBtn = new QPushButton("Share");
    shareBtn->setFixedSize(100, 32);
    shareBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: %2px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: %3; }
    )").arg(m_category.colorHex)
       .arg(TE::rSm)
       .arg(QColor(m_category.colorHex).lighter(115).name()));
    
    connect(shareBtn, &QPushButton::clicked, this, [this, targetCombo]() {
        if (m_inputField->text().trimmed().isEmpty()) return;
        
        int idx = targetCombo->currentIndex();
        if (idx == 0) { // Broadcast
            emit broadcastRichPresenceRequested(m_category, m_inputField->text());
        } else { // Share
            QString targetUser = targetCombo->itemData(idx).toString();
            emit shareToChatRequested(m_category, m_inputField->text(), targetUser);
        }
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

    // Draw rounded background
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), TE::rLg, TE::rLg);

    p.fillPath(path, TE::surfaceHigh());
    
    p.setPen(QPen(QColor(255,255,255,30), 1));
    p.drawPath(path);
}

} // namespace wizz::ui::arcade
