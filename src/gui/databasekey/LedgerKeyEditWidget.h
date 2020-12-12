#ifndef KEEPASSXC_LEDGERKEYEDITWIDGET_H
#define KEEPASSXC_LEDGERKEYEDITWIDGET_H

#include "KeyComponentWidget.h"
#include <QMessageBox>
#include <QPointer>
#include <QSharedPointer>

namespace Ui
{
    class LedgerKeyEditWidget;
}

class DatabaseSettingsWidget;
class LedgerKey;

class LedgerKeyEditWidget : public KeyComponentWidget
{
    Q_OBJECT

public:
    explicit LedgerKeyEditWidget(DatabaseSettingsWidget* parent);
    Q_DISABLE_COPY(LedgerKeyEditWidget);
    ~LedgerKeyEditWidget() override;

    bool addToCompositeKey(QSharedPointer<CompositeKey> key) override;
    bool validate(QString& errorMessage) const override;

protected:
    QWidget* componentEditWidget() override;
    void initComponentEditWidget(QWidget* widget) override;

private slots:
    void hardwareLedgerKeyResponse(int res, int appProto, int libProto);
    void pollLedgerKey();

private:
    void updateLedgerWidget();

    bool m_pollingLedgerKey = false;

    const QScopedPointer<Ui::LedgerKeyEditWidget> m_compUi;
    QPointer<QWidget> m_compEditWidget;
    const QPointer<DatabaseSettingsWidget> m_parent;
    QMessageBox* m_msgBox;
    mutable QSharedPointer<LedgerKey> LKey_;
};

#endif
