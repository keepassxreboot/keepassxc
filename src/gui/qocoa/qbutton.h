#ifndef QBUTTON_H
#define QBUTTON_H

#include <QWidget>
#include <QPointer>

class QButtonPrivate;
class QButton : public QWidget
{
    Q_OBJECT
public:
    // Matches NSBezelStyle
    enum BezelStyle {
       Rounded           = 1,
       RegularSquare     = 2,
       Disclosure        = 5,
       ShadowlessSquare  = 6,
       Circular          = 7,
       TexturedSquare    = 8,
       HelpButton        = 9,
       SmallSquare       = 10,
       TexturedRounded   = 11,
       RoundRect         = 12,
       Recessed          = 13,
       RoundedDisclosure = 14,
#ifdef __MAC_10_7
       Inline            = 15
#endif
    };

    explicit QButton(QWidget *parent, BezelStyle bezelStyle = Rounded);

public Q_SLOTS:
    void setText(const QString &text);
    void setImage(const QPixmap &image);
    void setChecked(bool checked);

public:
    void setCheckable(bool checkable);
    bool isChecked();

Q_SIGNALS:
    void clicked(bool checked = false);

private:
    friend class QButtonPrivate;
    QPointer<QButtonPrivate> pimpl;
};
#endif // QBUTTON_H
