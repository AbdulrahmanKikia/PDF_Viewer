#include "locationssidebarwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QStandardPaths>
#include <QScrollArea>

LocationsSidebarWidget::LocationsSidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("LocationsSidebar"));
    setupUI();
}

LocationsSidebarWidget::~LocationsSidebarWidget()
{
}

void LocationsSidebarWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 12, 8, 8);
    mainLayout->setSpacing(4);

    m_locationsFrame = new QFrame(this);
    m_locationsFrame->setObjectName(QStringLiteral("LocationsSidebarFrame"));

    QVBoxLayout *locationsLayout = new QVBoxLayout(m_locationsFrame);
    locationsLayout->setContentsMargins(0, 0, 0, 0);
    locationsLayout->setSpacing(2);

    // Standard locations (placeholders - paths from QStandardPaths)
    const QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QPushButton *recentBtn = createLocationButton(tr("Recent"), homePath);
    recentBtn->setChecked(true);  // Default view is home
    locationsLayout->addWidget(recentBtn);
    locationsLayout->addWidget(createLocationButton(tr("Downloads"), downloadsPath.isEmpty() ? homePath : downloadsPath));
    locationsLayout->addWidget(createLocationButton(tr("Documents"), documentsPath.isEmpty() ? homePath : documentsPath));
    locationsLayout->addWidget(createLocationButton(tr("Desktop"), desktopPath.isEmpty() ? homePath : desktopPath));

    locationsLayout->addSpacing(12);

    QPushButton *addLocationBtn = new QPushButton(tr("+ Add Location"), this);
    addLocationBtn->setObjectName(QStringLiteral("AddLocationButton"));
    addLocationBtn->setFlat(true);
    connect(addLocationBtn, &QPushButton::clicked, this, [this]() {
        // Placeholder: will open folder picker in later phase
        emit locationSelected(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    });
    locationsLayout->addWidget(addLocationBtn);

    locationsLayout->addStretch();

    mainLayout->addWidget(m_locationsFrame);
}

QPushButton *LocationsSidebarWidget::createLocationButton(const QString &label, const QString &path)
{
    QPushButton *btn = new QPushButton(label, this);
    btn->setObjectName(QStringLiteral("LocationButton"));
    btn->setFlat(true);
    btn->setCheckable(true);
    btn->setProperty("locationPath", path);
    m_locationButtons.append(btn);
    connect(btn, &QPushButton::clicked, this, [this, path, btn]() {
        for (QPushButton *b : m_locationButtons) {
            b->setChecked(b == btn);
        }
        onLocationClicked(path);
    });
    return btn;
}

void LocationsSidebarWidget::onLocationClicked(const QString &path)
{
    emit locationSelected(path);
}
