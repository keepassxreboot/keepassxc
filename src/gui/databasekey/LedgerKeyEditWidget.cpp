#include "LedgerKeyEditWidget.h"
#include "ui_LedgerKeyEditWidget.h"
#include <gui/dbsettings/DatabaseSettingsWidget.h>

#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "keys/CompositeKey.h"
#include "keys/LedgerKey.h"
#include "keys/drivers/LedgerHardwareKey.h"

#define COMBO_TYPE_NAME_IDX 0
#define COMBO_TYPE_SLOT_IDX 1

LedgerKeyEditWidget::LedgerKeyEditWidget(DatabaseSettingsWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::LedgerKeyEditWidget())
    , m_parent(parent)
    , m_msgBox(new QMessageBox(QMessageBox::Information, "Ledger Key", "", QMessageBox::NoButton, this))
{
    m_msgBox->setStandardButtons(QMessageBox::StandardButton());
    setComponentName(tr("Ledger Key"));
    setComponentDescription(tr("<p>TODO</p>"));
}

LedgerKeyEditWidget::~LedgerKeyEditWidget()
{
}

bool LedgerKeyEditWidget::addToCompositeKey(QSharedPointer<CompositeKey> key)
{
    if (!LKey_) {
        return false;
    }
    key->addKey(LKey_);
    LKey_.reset();
    return true;
}

bool LedgerKeyEditWidget::validate(QString& errorMessage) const
{
    const int typeIdx = m_compUi->ledgerKeyTypeCombo->currentIndex();
    LKey_.reset();
    switch (typeIdx) {
    case COMBO_TYPE_NAME_IDX: {
        QString SKey = m_compUi->ledgerKeyLineEdit->text();
        if (SKey.isEmpty()) {
            errorMessage = tr("Ledger key: a key name must be provided.");
            return false;
        }
        LKey_ = LedgerKey::fromDeviceDeriveName(SKey);
        break;
    }
    case COMBO_TYPE_SLOT_IDX: {
        const int idx = m_compUi->ledgerKeySlotCombo->currentIndex();
        QVariant Slot = m_compUi->ledgerKeySlotCombo->itemData(idx);
        LKey_ = LedgerKey::fromDeviceSlot(Slot.toUInt());
        break;
    }
    default:
        return false;
    }
    if (LKey_.isNull()) {
        errorMessage = "Ledger key: error while getting the key for the device";
        return false;
    }
    return true;
}

QWidget* LedgerKeyEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);

    m_compUi->ledgerKeyLineEdit->setMaxLength(LedgerHardwareKey::maxNameSize());
    connect(&LedgerHardwareKey::instance(),
            SIGNAL(detectComplete(int, int, int)),
            this,
            SLOT(hardwareLedgerKeyResponse(int, int, int)),
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    connect(m_compUi->buttonRedetectLedger, SIGNAL(clicked()), SLOT(pollLedgerKey()), Qt::UniqueConnection);
    connect(
        m_compUi->ledgerKeyTypeCombo,
        QOverload<int>::of(&QComboBox::activated),
        this,
        [this](int) { this->updateLedgerWidget(); },
        Qt::UniqueConnection);
    updateLedgerWidget();

    connect(
        &LedgerHardwareKey::instance(),
        &LedgerHardwareKey::userInteractionRequest,
        this,
        [this] {
            m_msgBox->setText(tr("Please accept accessing the database key on your Ledger device."));
            m_msgBox->show();
        },
        Qt::UniqueConnection);
    connect(
        &LedgerHardwareKey::instance(), &LedgerHardwareKey::userInteractionDone, this, [this] { m_msgBox->hide(); });

    return m_compEditWidget;
}

void LedgerKeyEditWidget::updateLedgerWidget()
{
    const int idx = m_compUi->ledgerKeyTypeCombo->currentIndex();
    switch (idx) {
    case COMBO_TYPE_SLOT_IDX:
        m_compUi->ledgerKeyLineEdit->setVisible(false);
        m_compUi->ledgerKeySlotCombo->setVisible(true);
        break;
    case COMBO_TYPE_NAME_IDX:
        m_compUi->ledgerKeyLineEdit->setVisible(true);
        m_compUi->ledgerKeySlotCombo->setVisible(false);
        break;
    default:
        break;
    }
}

void LedgerKeyEditWidget::pollLedgerKey()
{
    if (m_pollingLedgerKey) {
        return;
    }
    m_pollingLedgerKey = true;
    m_compUi->ledgerKeyLineEdit->setEnabled(false);
    m_compUi->ledgerKeyTypeCombo->setEnabled(false);
    m_compUi->ledgerKeySlotCombo->setEnabled(false);
    m_compUi->buttonRedetectLedger->setEnabled(false);
    m_compUi->buttonRedetectLedger->setText(QObject::tr("Searching..."));
    LedgerHardwareKey::instance().findFirstDevice();
}

void LedgerKeyEditWidget::hardwareLedgerKeyResponse(int res, int appProto, int libProto)
{
    const bool found = (res == LedgerHardwareKey::Found);
    m_compUi->ledgerKeyLineEdit->setEnabled(found);
    m_compUi->ledgerKeySlotCombo->setEnabled(found);
    m_compUi->ledgerKeyTypeCombo->setEnabled(found);
    m_compUi->buttonRedetectLedger->setEnabled(true);
    m_compUi->buttonRedetectLedger->setText(QObject::tr("Refresh"));

    QComboBox* slotCombo = m_compUi->ledgerKeySlotCombo;
    slotCombo->clear();
    if (found) {
        auto Slots = LedgerHardwareKey::instance().getValidKeySlots();
        for (uint8_t S : Slots) {
            slotCombo->insertItem(slotCombo->count(), QString("# %1").arg(S), QVariant(S));
        }
    } else if (res == LedgerHardwareKey::ProtocolMismatch) {
        QString Msg = LedgerHardwareKey::protocolErrorMsg(appProto, libProto);
        QMessageBox::warning(this, "Ledger Key", Msg);
    }

    m_pollingLedgerKey = false;
}

void LedgerKeyEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
}
