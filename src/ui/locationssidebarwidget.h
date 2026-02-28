#ifndef LOCATIONSSIDEBARWIDGET_H
#define LOCATIONSSIDEBARWIDGET_H

#include <QWidget>
#include <QList>

class QVBoxLayout;
class QPushButton;
class QFrame;

class LocationsSidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LocationsSidebarWidget(QWidget *parent = nullptr);
    ~LocationsSidebarWidget();

signals:
    void locationSelected(const QString &path);

private slots:
    void onLocationClicked(const QString &path);

private:
    void setupUI();
    QPushButton *createLocationButton(const QString &label, const QString &path);

    QVBoxLayout *m_layout = nullptr;
    QFrame *m_locationsFrame = nullptr;
    QList<QPushButton *> m_locationButtons;
};

#endif // LOCATIONSSIDEBARWIDGET_H
