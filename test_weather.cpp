#include <QtTest/QtTest>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTabWidget>
#include <QMainWindow>

struct WeatherPoint {
    QDateTime datetime;
    double temperature;
    double pressure;
    double humidity;
    double windspeed;
};

QColor fon(double temp_c);
QList<WeatherPoint> parseVisualCrossingJson(const QJsonDocument& doc, const QString& freq);

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
        QTabWidget* tabs = new QTabWidget(this);
        setCentralWidget(tabs);
    }
};


QColor fon(double temp_c) {
    if (temp_c < -20) return QColor("#191970");
    else if (temp_c <= -10) return QColor("#4682B4");
    else if (temp_c <= -5) return QColor("#B0E0E6");
    else if (temp_c <= 0) return QColor("#E0FFFF");
    else if (temp_c <= 10) return QColor("#FFE4B5");
    else if (temp_c <= 15) return QColor("#DEB887");
    else if (temp_c < 21) return QColor("#DAA520");
    else if (temp_c < 26) return QColor("#FF8C00");
    else return QColor("#B22222");
}

QList<WeatherPoint> parseVisualCrossingJson(const QJsonDocument& doc, const QString& freq) {
    QList<WeatherPoint> result;
    if (!doc.isObject()) return result;
    
    QJsonObject obj = doc.object();
    QJsonArray days = obj["days"].toArray();
    
    for (const auto& dayV : days) {
        QJsonObject day = dayV.toObject();
        QString date = day["datetime"].toString();
        if (!day.contains("hours")) continue;
        
        QJsonArray hours = day["hours"].toArray();
        for (const auto& hourV : hours) {
            QJsonObject hour = hourV.toObject();
            WeatherPoint p;
            p.datetime = QDateTime::fromString(date + " " + hour["datetime"].toString(), "yyyy-MM-dd HH:mm:ss");
            p.temperature = hour["temp"].toDouble();
            p.pressure = hour["pressure"].toDouble();
            p.humidity = hour["humidity"].toDouble();
            p.windspeed = hour["windspeed"].toDouble();
            result.append(p);
        }
    }
    return result;
}

class TestWeatherApp : public QObject
{
    Q_OBJECT

private slots:

    void testFonColors()
    {
        QCOMPARE(fon(-25.0), QColor("#191970"));   // Очень холодно
        QCOMPARE(fon(-15.0), QColor("#4682B4"));  // Холодно
        QCOMPARE(fon(5.0), QColor("#FFE4B5"));     // Прохладно
        QCOMPARE(fon(20.0), QColor("#DAA520"));    // Тепло
        QCOMPARE(fon(30.0), QColor("#B22222"));    // Жарко
    }

    void testJsonParser()
    {
        QString json = R"({
            "days": [{
                "datetime": "2023-01-01",
                "hours": [
                    {"datetime": "00:00:00", "temp": 10, "pressure": 1010, "humidity": 80, "windspeed": 1.0},
                    {"datetime": "01:00:00", "temp": 11, "pressure": 1011, "humidity": 81, "windspeed": 1.1}
                ]
            }]
        })";

        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        auto points = parseVisualCrossingJson(doc, "1h");

        QCOMPARE(points.size(), 2);
        QCOMPARE(points[0].temperature, 10.0);
        QCOMPARE(points[1].humidity, 81.0);
        QVERIFY(points[0].datetime == QDateTime::fromString("2023-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss"));
    }

    void testEmptyJson()
    {
        QJsonDocument doc;
        auto points = parseVisualCrossingJson(doc, "1h");
        QVERIFY(points.isEmpty());
    }

    void testWeatherPointStruct()
    {
        WeatherPoint wp;
        wp.datetime = QDateTime::currentDateTime();
        wp.temperature = 15.5;
        wp.pressure = 1013.2;
        wp.humidity = 60.0;
        wp.windspeed = 3.1;

        QCOMPARE(wp.temperature, 15.5);
        QVERIFY(wp.datetime.isValid());
    }

    
    void testMainWindowCreation()
    {
        MainWindow w;
        QVERIFY(w.findChild<QTabWidget*>() != nullptr);
    }
};


#include "test_weather.moc"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestWeatherApp test;
    return QTest::qExec(&test, argc, argv);
}
