#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <numeric>
#include <QDebug>
#include <QTableWidgetItem>
#include <QWebEngineSettings>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QWebChannel>
#include <QObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);

    // Мост Qt ↔ JavaScript
    auto* bridge = new QtBridge(this);
    auto* channel = new QWebChannel(this);
    channel->registerObject("qt", bridge);
    ui->mapView->page()->setWebChannel(channel);

    // При клике на предприятие — запрашиваем похожие
    connect(bridge, &QtBridge::enterpriseClicked, this,
            [this](const QString& name, double lat, double lon) {
                requestSimilarEnterprises(name, lat, lon);
            });

    // Заполнение комбобокса тем на 5-й вкладке
    ui->themeComboBox->clear();
    ui->themeComboBox->addItems({"Тёмная", "Светлая"});

    applyDarkTheme();
    setupNavigation();

    ui->mapView->settings()->setAttribute(
        QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    // 1-я вкладка: Поиск по нажатию Enter в queryInput
    connect(ui->queryInput, &QLineEdit::returnPressed, this, &MainWindow::onAnalyzeClicked);

    // Слайдеры и их метки
    connect(ui->accessibilitySlider, &QSlider::valueChanged, this, &MainWindow::onAccessibilityChanged);
    connect(ui->costSlider, &QSlider::valueChanged, this, &MainWindow::onCostChanged);

    // Локальный пересчет без лишних сетевых запросов
    connect(ui->accessibilitySlider, &QSlider::sliderReleased, this, &MainWindow::updateLocalCalculations);
    connect(ui->costSlider, &QSlider::sliderReleased, this, &MainWindow::updateLocalCalculations);

    // 2-я вкладка: Поиск оборудования по кнопке и по Enter
    connect(ui->btnSearchEquipment, &QPushButton::clicked, this, &MainWindow::onSearchEquipmentClicked);
    connect(ui->equipmentQueryInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchEquipmentClicked);

    // 4-я вкладка: Кнопка обновления БД
    connect(ui->btnRefreshDb, &QPushButton::clicked, this, &MainWindow::onRefreshDbClicked);

    // 5-я вкладка: Сохранение настроек и переключение темы
    connect(ui->btnSaveSettings, &QPushButton::clicked, this, &MainWindow::onSaveSettingsClicked);
    connect(ui->themeComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index == 0) applyDarkTheme();
        else applyLightTheme();
    });

    // Настройка растягивания таблиц
    ui->statsTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->statsTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->hintLabel->setText("Введите запрос выше и нажмите Enter или кнопку «Найти»");
    ui->hintLabel->setVisible(true);

    ui->mapView->setUrl(
        QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/map.html")
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::requestSimilarEnterprises(const QString& name, double lat, double lon)
{
    // Убираем старые линии
    ui->mapView->page()->runJavaScript("clearConnections();");

    QString encodedName = QUrl::toPercentEncoding(name);
    QUrl url(m_backendUrl + "/similar/" + encodedName);
    QNetworkRequest request(url);

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, lat, lon]() {
                onSimilarReplyFinished(reply, lat, lon);
            });
}

void MainWindow::onSimilarReplyFinished(QNetworkReply* reply, double centerLat, double centerLon)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Ошибка при запросе похожих предприятий:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    if (!doc.isArray()) {
        qDebug() << "Бэкенд вернул не массив!";
        return;
    }

    QJsonArray results = doc.array();

    // Формируем строгий, валидный JSON через QJsonDocument
    QJsonDocument outDoc(results);
    QString jsonString = QString::fromUtf8(outDoc.toJson(QJsonDocument::Compact));

    // Вызываем JS-функцию
    QString js = QString("showConnections(%1, %2, %3);")
                     .arg(centerLat, 0, 'f', 6)
                     .arg(centerLon, 0, 'f', 6)
                     .arg(jsonString);

    ui->mapView->page()->runJavaScript(js);
}

void MainWindow::setupNavigation()
{
    auto* navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);

    // Синхронизация с порядком в ui:
    // 0 -> pageMap, 1 -> pageAnalytics, 2 -> pageSearch, 3 -> pageDB, 4 -> pageSettings
    navGroup->addButton(ui->btnMap, 0);
    navGroup->addButton(ui->btnSearch, 1);
    navGroup->addButton(ui->btnAnalytics, 2);
    navGroup->addButton(ui->btnDatabase, 3);
    navGroup->addButton(ui->btnSettings, 4);

    ui->btnMap->setCheckable(true);
    ui->btnSearch->setCheckable(true);
    ui->btnAnalytics->setCheckable(true);
    ui->btnDatabase->setCheckable(true);
    ui->btnSettings->setCheckable(true);

    ui->btnMap->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);

    connect(navGroup, &QButtonGroup::idClicked, this, [this](int id) {
        if (id >= 0 && id < ui->stackedWidget->count()) {
            ui->stackedWidget->setCurrentIndex(id);
        }
    });
}



void MainWindow::onAnalyzeClicked()
{
    QString query = ui->queryInput->text().trimmed();
    double acc = ui->accessibilitySlider->value() / 100.0;
    double cost = ui->costSlider->value() / 100.0;

    if (query.isEmpty()) return;

    QUrl url(m_backendUrl + "/search");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonBody;
    jsonBody["query"] = query;
    jsonBody["accessibility"] = acc;
    jsonBody["cost"] = cost;

    QNetworkReply* reply = networkManager->post(request, QJsonDocument(jsonBody).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, acc, cost]() {
        onSearchReplyFinished(reply, acc, cost);
    });
}

void MainWindow::onSearchReplyFinished(QNetworkReply* reply, double desiredAccessibility, double desiredCost)
{
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    m_lastResults = doc.object()["results"].toArray();

    objects.clear();

    for (const QJsonValue& val : m_lastResults) {
        QJsonObject obj = val.toObject();

        GeoObject geo;
        geo.name          = obj["enterprise"].toString();
        geo.latitude      = obj["lat"].toDouble();
        geo.longitude     = obj["lon"].toDouble();
        geo.accessibility = obj["accessibility"].toDouble();
        geo.cost          = obj["cost"].toDouble();

        double similarityScore = obj["score"].toDouble() * 100.0;
        geo.suitability = calculateMembership(geo, similarityScore, desiredAccessibility, desiredCost);

        objects.push_back(geo);
    }

    refreshUiElements();
}

void MainWindow::updateLocalCalculations()
{
    if (m_lastResults.isEmpty()) return;

    double desiredAcc = ui->accessibilitySlider->value() / 100.0;
    double desiredCost = ui->costSlider->value() / 100.0;

    for (int i = 0; i < objects.size(); ++i) {
        double similarityScore = m_lastResults[i].toObject()["score"].toDouble() * 100.0;
        objects[i].suitability = calculateMembership(objects[i], similarityScore, desiredAcc, desiredCost);
    }

    refreshUiElements();
}

void MainWindow::refreshUiElements()
{
    ui->hintLabel->setVisible(false);
    QVector<CorrelationRow> matrix;
    for (const auto& geo : objects) {
        double similarityScore = 0;
        for (const auto& item : m_lastResults) {
            if (item.toObject()["enterprise"].toString() == geo.name) {
                similarityScore = item.toObject()["score"].toDouble() * 100.0;
                break;
            }
        }
        matrix.push_back({geo.name, similarityScore / 100.0, geo.accessibility / 100.0, geo.cost / 100.0, geo.suitability});
    }

    // 1. Заполнение таблицы 1-й страницы
    ui->tableWidget->setRowCount(matrix.size());
    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setHorizontalHeaderLabels({"Объект", "Сходство", "Доступность", "Стоимость", "Пригодность"});

    for (int i = 0; i < matrix.size(); ++i) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(matrix[i].name));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(matrix[i].equipment * 100.0, 'f', 1) + "%"));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(matrix[i].accessibility * 100.0)));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(matrix[i].cost * 100.0)));

        auto* item = new QTableWidgetItem(QString::number(matrix[i].suitability, 'f', 2));
        item->setBackground(suitabilityToColor(matrix[i].suitability));
        item->setForeground(Qt::black);
        ui->tableWidget->setItem(i, 4, item);
    }

    // 2. Обновление карты
    ui->mapView->page()->runJavaScript("clearMap();");
    for (const GeoObject& geo : objects) {
        QColor color = suitabilityToColor(geo.suitability);

        QString balloonHtml = QString("%1<br>Сходство: %2%<br>Пригодность: %3")
                                  .arg(geo.name)
                                  .arg(QString::number(geo.accessibility))
                                  .arg(geo.suitability, 0, 'f', 2);

        // Передаем balloonHtml для балуна и geo.name для вызова backend
        QString balloonText = QString("%1<br>Сходство: %2%<br>Пригодность: %3")
                                  .arg(geo.name)
                                  .arg(QString::number(geo.accessibility))
                                  .arg(geo.suitability, 0, 'f', 2);
        QString safeName = geo.name;
        safeName.replace("'", "\\'");
        QString js = QString("addPoint(%1, %2, '%3', '%4', '%5');")
                         .arg(geo.latitude,  0, 'f', 6)
                         .arg(geo.longitude, 0, 'f', 6)
                         .arg(color.name())
                         .arg(balloonText)
                         .arg(safeName);

        ui->mapView->page()->runJavaScript(js);
    }

    // 3. Обновление матрицы корреляции в correlationTable (на странице pageAnalytics)
    QVector<double> sim, acc, cost, suit;
    for (const auto& row : matrix) {
        sim.push_back(row.equipment);
        acc.push_back(row.accessibility);
        cost.push_back(row.cost);
        suit.push_back(row.suitability);
    }

    QStringList headers = {"Similarity", "Accessibility", "Cost", "Suitability"};
    ui->correlationTable->setRowCount(4);
    ui->correlationTable->setColumnCount(4);
    ui->correlationTable->setHorizontalHeaderLabels(headers);
    ui->correlationTable->setVerticalHeaderLabels(headers);

    QVector<QVector<double>> values = {
        {correlation(sim,sim),  correlation(sim,acc),  correlation(sim,cost),  correlation(sim,suit)},
        {correlation(acc,sim),  correlation(acc,acc),  correlation(acc,cost),  correlation(acc,suit)},
        {correlation(cost,sim), correlation(cost,acc), correlation(cost,cost), correlation(cost,suit)},
        {correlation(suit,sim), correlation(suit,acc), correlation(suit,cost), correlation(suit,suit)}
    };

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            ui->correlationTable->setItem(i, j, new QTableWidgetItem(QString::number(values[i][j], 'f', 2)));
        }
    }
    ui->correlationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->correlationTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 4. Заполнение большой таблицы детальной статистики (statsTableWidget) на вкладке Аналитика
    ui->statsTableWidget->clearContents();
    ui->statsTableWidget->setRowCount(matrix.size());
    ui->statsTableWidget->setColumnCount(5);
    ui->statsTableWidget->setHorizontalHeaderLabels({"Предприятие", "Сходство", "Доступность", "Стоимость", "Пригодность"});

    for (int i = 0; i < matrix.size(); ++i) {
        ui->statsTableWidget->setItem(i, 0, new QTableWidgetItem(matrix[i].name));
        ui->statsTableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(matrix[i].equipment * 100.0, 'f', 1) + "%"));
        ui->statsTableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(matrix[i].accessibility * 100.0, 'f', 1) + "%"));
        ui->statsTableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(matrix[i].cost * 100.0, 'f', 1) + "%"));

        auto* item = new QTableWidgetItem(QString::number(matrix[i].suitability, 'f', 2));
        item->setBackground(suitabilityToColor(matrix[i].suitability));
        item->setForeground(Qt::black);
        ui->statsTableWidget->setItem(i, 4, item);
    }

    ui->statsTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

// Поиск оборудования на 2-й странице (pageSearch)
void MainWindow::onSearchEquipmentClicked()
{
    QString query = ui->equipmentQueryInput->text().trimmed();
    if (query.isEmpty()) return;

    QUrl url(m_backendUrl + "/search/equipment?query=" + QUrl::toPercentEncoding(query));
    QNetworkRequest request(url);

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray results = doc.array();

            ui->equipmentTableWidget->clearContents();
            ui->equipmentTableWidget->setRowCount(0);
            ui->equipmentTableWidget->setColumnCount(4);
            ui->equipmentTableWidget->setHorizontalHeaderLabels({"Оборудование", "Предприятие", "Описание", "Сходство"});

            int row = 0;
            for (const QJsonValue& val : results) {
                QJsonObject item = val.toObject();
                ui->equipmentTableWidget->insertRow(row);
                ui->equipmentTableWidget->setItem(row, 0, new QTableWidgetItem(item["name"].toString()));
                ui->equipmentTableWidget->setItem(row, 1, new QTableWidgetItem(item["enterprise"].toString()));
                ui->equipmentTableWidget->setItem(row, 2, new QTableWidgetItem(item["description"].toString()));
                ui->equipmentTableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(item["score"].toDouble(), 'f', 3)));
                row++;
            }
            ui->equipmentTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
        reply->deleteLater();
    });
}

// Обновление всей базы Neo4j на 4-й странице (pageDB)
void MainWindow::onRefreshDbClicked()
{
    QUrl url(m_backendUrl + "/api/database/all");
    QNetworkRequest request(url);

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray data = doc.array();

            ui->dbTableWidget->setRowCount(data.size());
            ui->dbTableWidget->setColumnCount(5);
            ui->dbTableWidget->setHorizontalHeaderLabels({"ID / Название", "Тип", "Широта", "Долгота", "Доступность"});

            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                ui->dbTableWidget->setItem(i, 0, new QTableWidgetItem(obj["name"].toString()));
                ui->dbTableWidget->setItem(i, 1, new QTableWidgetItem(obj["type"].toString()));
                ui->dbTableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(obj["lat"].toDouble())));
                ui->dbTableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(obj["lon"].toDouble())));
                ui->dbTableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(obj["accessibility"].toDouble())));
            }
            ui->dbTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
        reply->deleteLater();
    });
}

// Сохранение настроек на 5-й странице (pageSettings)
void MainWindow::onSaveSettingsClicked()
{
    m_backendUrl = ui->backendUrlInput->text().trimmed();
    m_neo4jPort = ui->neo4jPortInput->text().trimmed();
    qDebug() << "Настройки сохранены:" << m_backendUrl << ":" << m_neo4jPort;
}

void MainWindow::onAccessibilityChanged(int value)
{
    ui->accessibilityValueLabel->setText(QString::number(value));
}

void MainWindow::onCostChanged(int value)
{
    ui->costValueLabel->setText(QString::number(value));
}

double MainWindow::calculateMembership(const GeoObject& geo, double similarity, double desiredAcc, double desiredCost)
{
    double accScore = 1.0 - std::abs((geo.accessibility / 100.0) - desiredAcc);
    double costScore = 1.0 - std::abs((geo.cost / 100.0) - desiredCost);
    return (similarity / 100.0 * 0.4) + (accScore * 0.3) + (costScore * 0.3);
}

QColor MainWindow::suitabilityToColor(double suitability)
{
    if (suitability >= 0.75) return QColor("#22c55e");
    if (suitability >= 0.50) return QColor("#eab308");
    if (suitability >= 0.25) return QColor("#f97316");
    return QColor("#ef4444");
}

double MainWindow::correlation(const QVector<double>& x, const QVector<double>& y)
{
    if (x.size() != y.size() || x.isEmpty()) return 0.0;

    double sumX = std::accumulate(x.begin(), x.end(), 0.0);
    double sumY = std::accumulate(y.begin(), y.end(), 0.0);
    double meanX = sumX / x.size();
    double meanY = sumY / y.size();

    double numerator = 0.0;
    double denomX = 0.0;
    double denomY = 0.0;

    for (int i = 0; i < x.size(); ++i) {
        double diffX = x[i] - meanX;
        double diffY = y[i] - meanY;
        numerator += diffX * diffY;
        denomX += diffX * diffX;
        denomY += diffY * diffY;
    }

    if (denomX == 0.0 || denomY == 0.0) return 0.0;
    return numerator / std::sqrt(denomX * denomY);
}

void MainWindow::applyDarkTheme()
{
    this->setStyleSheet(R"(
        QMainWindow { background-color: #0f1117; }
        #sidebarFrame { background-color: #090b0f; border-right: 1px solid #1e2130; }
        QPushButton { background-color: #1a1d2e; color: #9ca3af; border: 1px solid #2a2d3a; border-radius: 8px; padding: 8px 12px; font-weight: 500; }
        QPushButton:hover { background-color: #252a40; color: #e2e4ef; }
        QPushButton:checked { background-color: #2563eb; color: #ffffff; border-color: #3b82f6; }
        QLineEdit { background-color: #1a1d2e; border: 1px solid #2a2d3a; border-radius: 8px; color: #e2e4ef; padding: 8px; }
        QComboBox { background-color: #1a1d2e; color: #e2e4ef; border: 1px solid #2a2d3a; border-radius: 6px; padding: 4px; }
        QTableWidget { background-color: #090b0f; color: #e2e4ef; gridline-color: #1e2130; border: 1px solid #1e2130; border-radius: 8px; }
        QHeaderView::section { background-color: #1a1d2e; color: #9ca3af; border: 1px solid #1e2130; padding: 4px; }
        QLabel { color: #e2e4ef; }
    )");
}

void MainWindow::applyLightTheme()
{
    this->setStyleSheet(R"(
        QMainWindow { background-color: #ffffff; }
        #sidebarFrame { background-color: #f3f4f6; border-right: 1px solid #e5e7eb; }
        QPushButton { background-color: #ffffff; color: #1f2937; border: 1px solid #d1d5db; border-radius: 8px; padding: 8px 12px; }
        QPushButton:checked { background-color: #2563eb; color: #ffffff; }
        QLineEdit { background-color: #ffffff; border: 1px solid #d1d5db; color: #1f2937; padding: 8px; }
        QComboBox { background-color: #ffffff; color: #1f2937; border: 1px solid #d1d5db; border-radius: 6px; padding: 4px; }
        QTableWidget { background-color: #ffffff; color: #1f2937; gridline-color: #e5e7eb; border: 1px solid #e5e7eb; }
        QHeaderView::section { background-color: #f3f4f6; color: #374151; border: 1px solid #e5e7eb; }
        QLabel { color: #1f2937; }
    )");
}
