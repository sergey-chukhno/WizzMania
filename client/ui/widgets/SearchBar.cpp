#include "SearchBar.h"
#include <QGraphicsOpacityEffect>
#include <QEvent>

namespace wizz::ui {

using TE = ThemeEngine;

SearchBar::SearchBar(QWidget* parent)
    : QWidget(parent)
    , m_isFocused(false)
{
    setFixedHeight(36);
    
    // Background frame styling
    setObjectName("searchBarContainer");
    setStyleSheet(QString(R"(
        QWidget#searchBarContainer {
            background-color: %1;
            border: 1px solid rgba(255, 255, 255, 10);
            border-radius: %2px;
        }
    )").arg(TE::surface().name()).arg(TE::rMd));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(8);

    // Search Icon
    m_iconLabel = new QLabel("🔍", this);
    m_iconLabel->setStyleSheet(QString("color: %1; font-size: 14px; background: transparent;")
                               .arg(TE::onSurface2().name()));
    layout->addWidget(m_iconLabel);

    // Line Edit
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText("Search citizens...");
    m_lineEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background: transparent;
            border: none;
            color: %1;
            font-size: 13px;
        }
        QLineEdit::placeholder {
            color: %2;
        }
    )").arg(TE::onSurface().name()).arg(TE::onSurface3().name()));
    m_lineEdit->setFont(TE::fontBody());
    
    layout->addWidget(m_lineEdit, 1);

    // Events
    m_lineEdit->installEventFilter(this);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &SearchBar::onTextChanged);

    // Focus Animation Setup
    m_focusAnim = new QPropertyAnimation(this, "styleSheet");
    m_focusAnim->setDuration(200);
}

QString SearchBar::text() const {
    return m_lineEdit->text();
}

void SearchBar::clear() {
    m_lineEdit->clear();
}

void SearchBar::onTextChanged(const QString& text) {
    emit textChanged(text);
}

bool SearchBar::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_lineEdit) {
        if (event->type() == QEvent::FocusIn) {
            animateFocus(true);
        } else if (event->type() == QEvent::FocusOut) {
            animateFocus(false);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SearchBar::animateFocus(bool hasFocus) {
    if (m_isFocused == hasFocus) return;
    m_isFocused = hasFocus;

    QString startStyle = styleSheet();
    QString endStyle;

    if (hasFocus) {
        endStyle = QString(R"(
            QWidget#searchBarContainer {
                background-color: %1;
                border: 1px solid %2;
                border-radius: %3px;
            }
        )").arg(TE::surfaceHigh().name())
           .arg(TE::accent().name())
           .arg(TE::rMd);
        m_iconLabel->setStyleSheet(QString("color: %1; font-size: 14px; background: transparent;")
                                   .arg(TE::accent().name()));
    } else {
        endStyle = QString(R"(
            QWidget#searchBarContainer {
                background-color: %1;
                border: 1px solid rgba(255, 255, 255, 10);
                border-radius: %2px;
            }
        )").arg(TE::surface().name())
           .arg(TE::rMd);
        m_iconLabel->setStyleSheet(QString("color: %1; font-size: 14px; background: transparent;")
                                   .arg(TE::onSurface2().name()));
    }

    // Qt doesn't natively animate stylesheets, so we jump directly. 
    // Alternatively, we could animate an overlay color, but jumping is snappy enough for now.
    setStyleSheet(endStyle);
}

} // namespace wizz::ui
