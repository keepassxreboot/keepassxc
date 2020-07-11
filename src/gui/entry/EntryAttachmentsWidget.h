#ifndef ENTRYATTACHMENTSWIDGET_H
#define ENTRYATTACHMENTSWIDGET_H

#include <QPointer>
#include <QWidget>

namespace Ui
{
    class EntryAttachmentsWidget;
}

class QByteArray;
class EntryAttachments;
class EntryAttachmentsModel;

class EntryAttachmentsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool isButtonsVisible READ isButtonsVisible WRITE setButtonsVisible NOTIFY buttonsVisibleChanged)
public:
    explicit EntryAttachmentsWidget(QWidget* parent = nullptr);
    ~EntryAttachmentsWidget();

    const EntryAttachments* entryAttachments() const;
    bool isReadOnly() const;
    bool isButtonsVisible() const;

    QByteArray getAttachment(const QString& name);
    void setAttachment(const QString& name, const QByteArray& value);
    void removeAttachment(const QString& name);

public slots:
    void setEntryAttachments(const EntryAttachments* attachments);
    void clearAttachments();
    void setReadOnly(bool readOnly);
    void setButtonsVisible(bool isButtonsVisible);

signals:
    void errorOccurred(const QString& error);
    void readOnlyChanged(bool readOnly);
    void buttonsVisibleChanged(bool isButtonsVisible);
    void widgetUpdated();

private slots:
    void insertAttachments();
    void removeSelectedAttachments();
    void renameSelectedAttachments();
    void saveSelectedAttachments();
    void openAttachment(const QModelIndex& index);
    void openSelectedAttachments();
    void updateButtonsVisible();
    void updateButtonsEnabled();

private:
    bool insertAttachments(const QStringList& fileNames, QString& errorMessage);
    bool openAttachment(const QModelIndex& index, QString& errorMessage);

    QStringList confirmLargeAttachments(const QStringList& filenames);

    bool eventFilter(QObject* watched, QEvent* event) override;

    QScopedPointer<Ui::EntryAttachmentsWidget> m_ui;
    QPointer<EntryAttachments> m_entryAttachments;
    QPointer<EntryAttachmentsModel> m_attachmentsModel;
    bool m_readOnly;
    bool m_buttonsVisible;
};

#endif // ENTRYATTACHMENTSWIDGET_H
