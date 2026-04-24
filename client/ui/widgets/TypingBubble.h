#pragma once

#include <QWidget>
#include <QTimer>

namespace wizz::ui {

class TypingBubble : public QWidget {
    Q_OBJECT

public:
    explicit TypingBubble(QWidget* parent = nullptr);
    ~TypingBubble();

    void startAnimation();
    void stopAnimation();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QTimer* m_timer;
    int m_step = 0;
};

} // namespace wizz::ui
