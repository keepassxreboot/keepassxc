

#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include "gui/kmessagewidget.h"

class MessageWidget : public KMessageWidget
{
    Q_OBJECT

public:
    explicit MessageWidget(QWidget* parent = 0);

public Q_SLOTS:
    void showMessage(const QString& text, MessageWidget::MessageType type);
    void hideMessage();

};

#endif // MESSAGEWIDGET_H
