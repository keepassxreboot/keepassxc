

#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include "gui/kmessagewidget.h"

class MessageWidget : public KMessageWidget
{
public:
    explicit MessageWidget(QWidget* parent = 0);
    void showMessageError(const QString& text);
    void showMessageWarning(const QString& text);
    void showMessageInformation(const QString& text);
    void showMessagePositive(const QString& text);

private:
    void showMessage(const QString& text, MessageType type);
};

#endif // MESSAGEWIDGET_H
