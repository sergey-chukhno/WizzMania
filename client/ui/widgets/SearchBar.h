#pragma once

#include "../theme/ThemeEngine.h"
#include <QLineEdit>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>

namespace wizz::ui {

/**
 * @brief SearchBar — A sleek, animated search input field.
 *
 * Features:
 * - Integrated search icon
 * - Animated clear button (appears when text is entered)
 * - Focus animation (expands or highlights on focus)
 * - Emits textChanged signals for live filtering
 */
class SearchBar : public QWidget {
    Q_OBJECT

public:
    explicit SearchBar(QWidget* parent = nullptr);

    QString text() const;
    void clear();

signals:
    void textChanged(const QString& text);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void animateFocus(bool hasFocus);

private:
    QLineEdit* m_lineEdit;
    QLabel* m_iconLabel;
    QPropertyAnimation* m_focusAnim;
    bool m_isFocused;
};

} // namespace wizz::ui
