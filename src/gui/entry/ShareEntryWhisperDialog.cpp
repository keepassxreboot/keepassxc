/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ShareEntryWhisperDialog.h"
#include "ui_ShareEntryWhisperDialog.h"

#include "core/Config.h"
#include "core/Entry.h"
#include "gui/Clipboard.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"
#include "networking/WhisperClient.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QPushButton>

namespace
{
    void setMuted(QLabel* label)
    {
        label->setForegroundRole(QPalette::PlaceholderText);
    }
} // namespace

ShareEntryWhisperDialog::ShareEntryWhisperDialog(const Entry* entry, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ShareEntryWhisperDialog())
    , m_entry(entry)
    , m_client(new WhisperClient(this))
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    m_ui->messageWidget->setHidden(true);
    // Only the visible page should count towards the dialog size
    m_ui->resultPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    setupHeader();
    setupFields();
    setupControls();
    connectSignals();
    updateShareButtonState();
}

ShareEntryWhisperDialog::~ShareEntryWhisperDialog() = default;

bool ShareEntryWhisperDialog::confirmConsent(QWidget* parent)
{
    if (config()->get(Config::Whisper_ConsentGiven).toBool()) {
        return true;
    }

    auto checkbox = new QCheckBox(tr("Don't ask me again"));
    bool dontAsk = false;
    QObject::connect(checkbox, &QCheckBox::toggled, [&](bool checked) { dontAsk = checked; });

    const auto host = QUrl(config()->get(Config::Whisper_ServerUrl).toString()).host();
    auto result = MessageBox::question(
        parent,
        tr("Share via Whisper"),
        tr("The selected data is encrypted on this device before it is sent to %1. The decryption key stays in "
           "the link and never reaches the server, so anyone with the link can read the data until it expires, "
           "or until it is opened once if self-destruct is on.\n\n"
           "Whisper is a third-party service and is not affiliated with the KeePassXC project.")
            .arg(host),
        MessageBox::Continue | MessageBox::Cancel,
        MessageBox::Continue,
        MessageBox::None,
        checkbox);
    if (result != MessageBox::Continue) {
        return false;
    }
    config()->set(Config::Whisper_ConsentGiven, dontAsk);
    return true;
}

void ShareEntryWhisperDialog::setupHeader()
{
    auto font = m_ui->headerTitleLabel->font();
    font.setPointSize(font.pointSize() + 3);
    font.setBold(true);
    m_ui->headerTitleLabel->setFont(font);
    setMuted(m_ui->headerSubtitleLabel);

    const auto title = m_entry->title().isEmpty() ? tr("Share Entry") : tr("Share \"%1\"").arg(m_entry->title());
    setHeader(Icons::entryIconPixmap(m_entry, IconSize::Large),
              title,
              tr("Encrypted on this device. Only people with the link can read it."));
}

void ShareEntryWhisperDialog::setupFields()
{
    const auto hidden = QString("●").repeated(6);
    const auto username = m_entry->resolveMultiplePlaceholders(m_entry->username());
    const auto password = m_entry->resolveMultiplePlaceholders(m_entry->password());
    const auto url = m_entry->resolveMultiplePlaceholders(m_entry->url());
    const auto notes = m_entry->resolveMultiplePlaceholders(m_entry->notes());
    const bool hideUsername = config()->get(Config::GUI_HideUsernames).toBool();
    const bool hideNotes = config()->get(Config::Security_HideNotes).toBool();

    setupField(m_ui->usernameCheckBox, m_ui->usernameValueLabel, username, hideUsername ? hidden : username);
    setupField(m_ui->passwordCheckBox, m_ui->passwordValueLabel, password, hidden);
    setupField(m_ui->urlCheckBox, m_ui->urlValueLabel, url, url);
    setupField(
        m_ui->notesCheckBox, m_ui->notesValueLabel, notes, hideNotes ? hidden : QString(notes).replace('\n', ' '));
}

void ShareEntryWhisperDialog::setupField(QCheckBox* checkBox,
                                         QLabel* preview,
                                         const QString& value,
                                         const QString& shownValue)
{
    setMuted(preview);
    if (value.isEmpty()) {
        checkBox->setChecked(false);
        checkBox->setEnabled(false);
        preview->setEnabled(false);
        preview->setText(tr("(empty)"));
        return;
    }
    preview->setText(preview->fontMetrics().elidedText(shownValue, Qt::ElideRight, 280));
}

void ShareEntryWhisperDialog::setupControls()
{
    m_ui->expirationComboBox->addItem(tr("1 hour"), 3600);
    m_ui->expirationComboBox->addItem(tr("6 hours"), 6 * 3600);
    m_ui->expirationComboBox->addItem(tr("24 hours"), 24 * 3600);
    m_ui->expirationComboBox->addItem(tr("7 days"), 7 * 24 * 3600);

    const auto serverUrl = config()->get(Config::Whisper_ServerUrl).toString();
    m_ui->serverIconLabel->setPixmap(icons()->icon("lock").pixmap(16));
    m_ui->serverLabel->setText(QUrl(serverUrl).host());
    m_ui->serverLabel->setToolTip(serverUrl);
    setMuted(m_ui->serverLabel);

    m_shareButton = m_ui->configButtonBox->addButton(tr("Share"), QDialogButtonBox::AcceptRole);
    m_shareButton->setIcon(icons()->icon("share-link"));
    m_shareButton->setDefault(true);
    m_ui->copyButton->setIcon(icons()->icon("url-copy"));
    m_ui->expirationIconLabel->setPixmap(icons()->icon("entry-expire").pixmap(16));
    setMuted(m_ui->expirationInfoLabel);
}

void ShareEntryWhisperDialog::connectSignals()
{
    for (auto checkBox : fieldCheckBoxes()) {
        connect(checkBox, &QCheckBox::toggled, this, &ShareEntryWhisperDialog::updateShareButtonState);
    }
    // clicked, not toggled, so the sync in updateShareButtonState() does not loop back here
    connect(m_ui->selectAllCheckBox, &QCheckBox::clicked, this, [this](bool checked) {
        for (auto checkBox : fieldCheckBoxes()) {
            checkBox->setChecked(checked && checkBox->isEnabled());
        }
    });

    auto openButton = m_ui->resultButtonBox->addButton(tr("Open in Browser"), QDialogButtonBox::ActionRole);
    openButton->setIcon(icons()->icon("web"));
    connect(openButton, &QPushButton::clicked, this, [this] { QDesktopServices::openUrl(m_shareUrl); });

    connect(m_ui->configButtonBox, &QDialogButtonBox::accepted, this, &ShareEntryWhisperDialog::onShareClicked);
    connect(m_ui->configButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_ui->resultButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_ui->copyButton, &QPushButton::clicked, this, &ShareEntryWhisperDialog::onCopyClicked);
    connect(m_client, &WhisperClient::shareSucceeded, this, &ShareEntryWhisperDialog::onShareSucceeded);
    connect(m_client, &WhisperClient::shareFailed, this, &ShareEntryWhisperDialog::onShareFailed);
}

void ShareEntryWhisperDialog::setHeader(const QPixmap& icon, const QString& title, const QString& subtitle)
{
    m_ui->headerIconLabel->setPixmap(icon);
    m_ui->headerTitleLabel->setText(title);
    m_ui->headerSubtitleLabel->setText(subtitle);
}

QList<QCheckBox*> ShareEntryWhisperDialog::fieldCheckBoxes() const
{
    return {m_ui->usernameCheckBox, m_ui->passwordCheckBox, m_ui->urlCheckBox, m_ui->notesCheckBox};
}

void ShareEntryWhisperDialog::updateShareButtonState()
{
    bool any = false;
    bool all = true;
    for (auto checkBox : fieldCheckBoxes()) {
        if (checkBox->isEnabled()) {
            any = any || checkBox->isChecked();
            all = all && checkBox->isChecked();
        }
    }
    m_shareButton->setEnabled(any);
    m_ui->selectAllCheckBox->setChecked(any && all);
}

void ShareEntryWhisperDialog::onShareClicked()
{
    WhisperClient::Fields fields;
    fields.username = m_ui->usernameCheckBox->isChecked();
    fields.password = m_ui->passwordCheckBox->isChecked();
    fields.url = m_ui->urlCheckBox->isChecked();
    fields.notes = m_ui->notesCheckBox->isChecked();

    m_ui->messageWidget->hideMessage();
    m_shareButton->setEnabled(false);
    m_shareButton->setText(tr("Sharing…"));
    m_client->shareSecret(config()->get(Config::Whisper_ServerUrl).toString(),
                          WhisperClient::buildPayload(m_entry, fields),
                          m_ui->expirationComboBox->currentData().toInt(),
                          m_ui->selfDestructCheckBox->isChecked());
}

void ShareEntryWhisperDialog::onShareSucceeded(const QUrl& shareLink)
{
    m_shareUrl = shareLink;
    setHeader(icons()->icon("dialog-ok").pixmap(32),
              tr("Your link is ready"),
              tr("Anyone with this link can read the shared data. Send it over a channel you trust."));
    m_ui->resultUrlLineEdit->setText(shareLink.toString());
    m_ui->resultUrlLineEdit->setCursorPosition(0);

    const auto expiration = m_ui->expirationComboBox->currentText();
    m_ui->expirationInfoLabel->setText(m_ui->selfDestructCheckBox->isChecked()
                                           ? tr("Expires in %1 · Deleted after it is opened once").arg(expiration)
                                           : tr("Expires in %1").arg(expiration));

    m_ui->configPage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_ui->resultPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_ui->stackedWidget->setCurrentWidget(m_ui->resultPage);
    m_shareButton->setDefault(false);
    m_ui->copyButton->setDefault(true);
    m_ui->copyButton->setFocus();
    adjustSize();
}

void ShareEntryWhisperDialog::onShareFailed(const QString& errorMessage)
{
    m_ui->messageWidget->showMessage(errorMessage, MessageWidget::Error, MessageWidget::DisableAutoHide);
    m_shareButton->setText(tr("Share"));
    updateShareButtonState();
}

void ShareEntryWhisperDialog::onCopyClicked()
{
    clipboard()->setText(m_shareUrl.toString());
    m_ui->messageWidget->showMessage(tr("Link copied to clipboard."), MessageWidget::Positive);
}
