#pragma once

#include <QDialog>
#include <QString>
#include "ArcadeCategory.h"

namespace wizz::ui::arcade {

/**
 * @brief GameSelectionPopup — A modal dialog for choosing which game to launch.
 */
class GameSelectionPopup : public QDialog {
    Q_OBJECT

public:
    explicit GameSelectionPopup(const ArcadeCategory& category, QWidget* parent = nullptr);

signals:
    void gameSelected(const QString& gameId);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    ArcadeCategory m_category;
};

} // namespace wizz::ui::arcade
