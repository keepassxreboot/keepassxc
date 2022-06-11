/*
  MIT License

  Copyright (c) 2022 KeePassXC Team <team@keepassxc.org>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#pragma once

#include <QTextEdit>

class Entry;
class QListWidget;
class QListWidgetItem;

struct EntryPlaceholder
{
    EntryPlaceholder(const QString& placeholder);

    QString getString() const
    {
        return m_fullText;
    }
    QString getStyledString(const bool emptyTargetValue) const;
    QString getKey() const
    {
        return m_key;
    }
    QString getValue() const
    {
        return m_value;
    }
    QString getRefField() const
    {
        return m_crossReferenceField;
    }
    QString getRefSearchIn() const
    {
        return m_crossReferenceSearchIn;
    }

private:
    QString m_fullText;
    QString m_key;
    QString m_value;
    QString m_crossReferenceField;
    QString m_crossReferenceSearchIn;
};

class EntryPlaceholderCompletionPopup : public QWidget
{
    Q_OBJECT

public:
    explicit EntryPlaceholderCompletionPopup(QWidget* parent);

    int count() const;
    int getCurrentItemIndex() const;
    QString getCurrentItemText() const;

    void setCurrentIndex(const int idx) const;
    void populateCompletionChoices(const QString& filter);

    bool matchAutoComplete(const QString& placeholder) const;

signals:
    void autoCompletionChoiceSelected(QString choice);

private slots:
    void itemActivated(QListWidgetItem* item);
    void itemClicked(QListWidgetItem* item);

private:
    QListWidget* m_autoCompletionList;
    QStringList m_autoCompletionPlaceholdersList;

    Q_DISABLE_COPY(EntryPlaceholderCompletionPopup)
};

class EntryPlaceholderTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit EntryPlaceholderTextEdit(QWidget* parent);

    void setEntry(Entry* e, const QString& text);
    QString text() const
    {
        return toPlainText();
    }

signals:
    void textChanged(QString newContent);
    void accepted();
    void canceled();

protected:
    virtual void keyPressEvent(QKeyEvent* e) override;

private slots:
    void trackCursorPosition();
    void applyAutoCompletion(QString placeholderName);

private:
    Entry* m_entry;
    EntryPlaceholderCompletionPopup* m_autoCompletionPopup;

    void redrawPlaceHolders();
    QString getPlaceholderBeforeCursor() const;
    QString getPlaceholderAfterCursor() const;

    Q_DISABLE_COPY(EntryPlaceholderTextEdit)
};
