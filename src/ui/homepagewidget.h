#ifndef HOMEPAGEWIDGET_H
#define HOMEPAGEWIDGET_H

#include <QWidget>

class LocationsSidebarWidget;
class HomeContentWidget;
class QSplitter;

class HomePageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomePageWidget(QWidget *parent = nullptr);
    ~HomePageWidget();

    HomeContentWidget* contentWidget() const { return m_content; }
    void focusSearchBar();

signals:
    void openFileRequested(const QString &path);

private slots:
    void onLocationSelected(const QString &path);

private:
    void setupUI();

    QSplitter *m_splitter = nullptr;
    LocationsSidebarWidget *m_sidebar = nullptr;
    HomeContentWidget *m_content = nullptr;
};

#endif // HOMEPAGEWIDGET_H
