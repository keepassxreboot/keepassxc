#include <QCheckBox>
#include <QMouseEvent>
#include <QPainter>

class SafariWebExtensionCheckbox : public QCheckBox
{
    Q_OBJECT

public:
    explicit SafariWebExtensionCheckbox(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *e) override;

private slots:
    void onApplicationStateChanged(Qt::ApplicationState state);
};