#include "TotpDialog.h"
#include "ui_TotpDialog.h"

#include "core/Config.h"
#include "core/Entry.h"
#include "gui/DatabaseWidget.h"
#include "gui/Clipboard.h"

#include <QTimer>
#include <QDateTime>
#include <QPushButton>


TotpDialog::TotpDialog(DatabaseWidget* parent, Entry* entry)
    : QDialog(parent)
    , m_ui(new Ui::TotpDialog())
{
    m_entry = entry;
    m_parent = parent;

    m_ui->setupUi(this);

    updateProgressBar();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateSeconds()));
    timer->start(300);

    uCounter = resetCounter();

    updateTotp();

    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Copy"));

    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(copyToClipboard()));
}

void TotpDialog::copyToClipboard()
{
    clipboard()->setText(m_entry->totp());
    if (config()->get("MinimizeOnCopy").toBool()) {
        m_parent->window()->showMinimized();
    }
}

void TotpDialog::updateProgressBar()
{
    if (uCounter <= 100) {
        m_ui->progressBar->setValue(100 - uCounter);
        uCounter++;
    } else {
        updateTotp();
        uCounter = resetCounter();
    }
}


void TotpDialog::updateSeconds()
{
    m_ui->timerLabel->setText(tr("Expires in") + " <b>" + QString::number(30 - (uCounter * 30 / 100)) + "</b> " + tr("seconds")); 
}

void TotpDialog::updateTotp()
{
    m_ui->totpLabel->setText(m_entry->totp());
}

unsigned TotpDialog::resetCounter()
{
    double curSeconds = QDateTime::currentDateTime().toString("s").toInt();

    if (curSeconds >= 30)
        curSeconds = curSeconds - 30;
    return curSeconds / 30 * 100;
}

TotpDialog::~TotpDialog()
{
}
