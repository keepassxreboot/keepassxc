#include "QuickStartWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/Entry.h"
#include "core/EntryAttributes.h"
#include "gui/Clipboard.h"
#include "gui/DatabaseWidget.h"
#include "gui/Icons.h"

QuickStartWindow::QuickStartWindow(Entry* entry, DatabaseWidget* dbWidget, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::WindowStaysOnTopHint)
    , m_entry(entry)
    , m_dbWidget(dbWidget)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("KeePassXL-QuickStart");
    setMinimumWidth(fontMetrics().horizontalAdvance(windowTitle()) + 130);

    auto* layout = new QVBoxLayout(this);

    // Datenbankname
    auto* dbLabel = new QLabel(dbWidget->displayName(), this);
    dbLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(dbLabel);

    // Kopfzeile: Icon + Titel
    auto* header = new QHBoxLayout();
    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(Icons::entryIconPixmap(entry));
    auto* titleLabel = new QLabel(entry->title(), this);
    header->addWidget(iconLabel);
    header->addWidget(titleLabel, 1);
    layout->addLayout(header);

    // Button: URL -> Browser
    if (!entry->url().isEmpty()) {
        auto* btn = new QPushButton(tr("URL / Program"), this);
        connect(btn, &QPushButton::clicked, this, [this]() {
            if (m_entry && m_dbWidget) {
                m_dbWidget->openUrlForEntry(m_entry);
            }
        });
        layout->addWidget(btn);
    }

    // Hilfsfunktion: Button, der einen Wert in die Zwischenablage kopiert
    auto addCopyButton = [this, layout](const QString& label, std::function<QString()> getValue, bool nonEmpty) {
        if (!nonEmpty) {
            return;
        }
        auto* btn = new QPushButton(label, this);
        connect(btn, &QPushButton::clicked, this, [this, getValue]() {
            if (m_entry) {
                clipboard()->setText(getValue(), true);
            }
        });
        layout->addWidget(btn);
    };

    addCopyButton(
        tr("User"),
        [this]() { return m_entry->resolveMultiplePlaceholders(m_entry->username()); },
        !entry->username().isEmpty());
    addCopyButton(
        tr("Password"),
        [this]() { return m_entry->resolveMultiplePlaceholders(m_entry->password()); },
        !entry->password().isEmpty());

    // Alle benutzerdefinierten Felder
    for (const QString& key : entry->attributes()->customKeys()) {
        if (entry->attributes()->value(key).isEmpty()) {
            continue;
        }
        addCopyButton(
            key,
            [this, key]() { return m_entry->resolveMultiplePlaceholders(m_entry->attributes()->value(key)); },
            true);
    }
}
