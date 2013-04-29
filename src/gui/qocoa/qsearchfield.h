#ifndef QSEARCHFIELD_H
#define QSEARCHFIELD_H

#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtGui/QMenu>

class QSearchFieldPrivate;
class QSearchField : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true);
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText);

public:
    explicit QSearchField(QWidget *parent);

    QString text() const;
    QString placeholderText() const;
    void setFocus(Qt::FocusReason);
    void setMenu(QMenu *menu);

public Q_SLOTS:
    void setText(const QString &text);
    void setPlaceholderText(const QString &text);
    void clear();
    void selectAll();
    void setFocus();

Q_SIGNALS:
    void textChanged(const QString &text);
    void editingFinished();
    void returnPressed();

private Q_SLOTS:
    void popupMenu();

protected:
    void changeEvent(QEvent*);
    void resizeEvent(QResizeEvent*);

private:
    friend class QSearchFieldPrivate;
    QPointer <QSearchFieldPrivate> pimpl;
};

#endif // QSEARCHFIELD_H
