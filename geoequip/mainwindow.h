#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QColor>
#include <QVector>
#include "model/GeoObject.h"
#include <QWebChannel>
#include <QObject>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct CorrelationRow {
    QString name;
    double equipment;
    double accessibility;
    double cost;
    double suitability;
};

class QtBridge : public QObject {
    Q_OBJECT
public:
    explicit QtBridge(QObject* parent = nullptr) : QObject(parent) {}
signals:
    void enterpriseClicked(const QString& name, double lat, double lon);
public slots:
    void onEnterpriseClicked(const QString& name, double lat, double lon) {
        emit enterpriseClicked(name, lat, lon);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void onSearchEquipmentClicked();
    void onRefreshDbClicked();
    void onSaveSettingsClicked();
    void onAccessibilityChanged(int value);
    void onCostChanged(int value);
    void updateLocalCalculations();
    void onSimilarReplyFinished(QNetworkReply* reply, double centerLat, double centerLon);
    void requestSimilarEnterprises(const QString& name, double lat, double lon);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;

    QVector<GeoObject> objects;
    QJsonArray m_lastResults;

    QString m_backendUrl = "http://127.0.0.1:8000";
    QString m_neo4jPort = "7687";

    void setupNavigation();
    void applyDarkTheme();
    void applyLightTheme();
    void onSearchReplyFinished(QNetworkReply *reply, double desiredAccessibility, double desiredCost);
    void refreshUiElements();

    double calculateMembership(const GeoObject& geo, double similarity, double desiredAcc, double desiredCost);
    QColor suitabilityToColor(double suitability);
    double correlation(const QVector<double>& x, const QVector<double>& y);
};

#endif // MAINWINDOW_H
