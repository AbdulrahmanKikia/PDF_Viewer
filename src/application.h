#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class PDFViewerApp : public QApplication
{
    Q_OBJECT
public:
    PDFViewerApp(int &argc, char **argv);
    ~PDFViewerApp();

    static PDFViewerApp *instance();

    void setTheme(const QString &themeName);
    QString currentTheme() const;

signals:
    void themeChanged(const QString &themeName);

private:
    static PDFViewerApp *s_instance;
    QString m_currentTheme;
};

#endif // APPLICATION_H
