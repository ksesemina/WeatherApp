#include <QtWidgets>
#include <QtCharts>
#include <QtNetwork>
#include <QtCharts/QDateTimeAxis>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <numeric>

/**
 * @file main.cpp
 * @brief Графическое приложение для отображения и анализа погодных данных через API.
 */

/**
 * @struct WeatherPoint
 * @brief Хранит данные о погоде в конкретный момент времени.
 */
struct WeatherPoint {
  QDateTime datetime;  ///< Дата и время измерения
  double temperature;  ///< Температура в градусах Цельсия
  double pressure;     ///< Атмосферное давление в hPa
  double humidity;     ///< Относительная влажность в процентах
  double windspeed;    ///< Скорость ветра в метрах в секунду
};

/**
 * @brief Определяет цвет фона в зависимости от температуры.
 *
 * @param temp_c Температура в градусах Цельсия.
 * @return QColor Цвет для подложки.
 */
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

/**
 * @brief Парсит JSON с погодой от VisualCrossing.
 *
 * @param doc   JSON-документ с данными API.
 * @param freq  Частота данных: "1h", "3h", "6h", "12h", "1d".
 * @return QList<WeatherPoint> Список точек погоды по расписанию.
 */
QList<WeatherPoint> parseVisualCrossingJson(const QJsonDocument& doc,
                                           const QString& freq) {
  QList<WeatherPoint> result;
  int interval = 1;
  if (freq == "1h")
    interval = 1;
  else if (freq == "3h")
    interval = 3;
  else if (freq == "6h")
    interval = 6;
  else if (freq == "12h")
    interval = 12;
  else if (freq == "1d")
    interval = 24;

  QJsonObject obj = doc.object();
  QJsonArray days = obj["days"].toArray();
  for (const auto& dayV : days) {
    QJsonObject day = dayV.toObject();
    QString date = day["datetime"].toString();
    if (!day.contains("hours")) continue;
    QJsonArray hours = day["hours"].toArray();
    for (int i = 0; i < hours.size(); i += interval) {
      QJsonObject hour = hours[i].toObject();
      WeatherPoint p;
      QString time = hour["datetime"].toString();
      p.datetime = QDateTime::fromString(date + " " + time, "yyyy-MM-dd HH:mm:ss");
      p.temperature = hour["temp"].toDouble();
      p.pressure = hour["pressure"].toDouble();
      p.humidity = hour["humidity"].toDouble();
      p.windspeed = hour["windspeed"].toDouble();
      result.append(p);
    }
  }
  return result;
}

/**
 * @class WeatherChart
 * @brief Виджет для отображения погодных графиков с помощью QtCharts.
 */
class WeatherChart : public QChartView {
  Q_OBJECT
 public:
  /**
   * @brief Конструктор WeatherChart.
   * @param parent Родительский виджет.
   */
  explicit WeatherChart(QWidget* parent = nullptr) : QChartView(parent) {
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(250);
  }

  /**
   * @brief Строит бар-график температур с окраской каждого столбца.
   * @param times Массив подписей X.
   * @param temps Вектор температур.
   * @param colors Цвета для каждого столбца.
   */
  void plotBarColored(const QStringList& times, const QVector<double>& temps,
                      const QVector<QColor>& colors) {
    auto* chart = new QChart();
    auto* series = new QBarSeries();
    auto* barSet = new QBarSet("Температура");
    for (double t : temps) *barSet << t;
    for (int i = 0; i < colors.size(); ++i) {
      barSet->setColor(colors[i]);
    }
    series->append(barSet);
    chart->addSeries(series);

    chart->setTitle("Температура в ближайшие 24 часа");
    auto* axisX = new QBarCategoryAxis();
    axisX->append(times);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    axisY->setRange(*std::min_element(temps.begin(), temps.end()) - 5,
        *std::max_element(temps.begin(), temps.end()) + 5);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    setChart(chart);
  }

  /**
   * @brief Строит линейный график с отображением средних и отклонений.
   *
   * @param xys            Массив точек (ось X — время, Y — значение).
   * @param name           Имя серии/графика.
   * @param color          Цвет основной линии.
   * @param showDeviation  Отображать ли линии +σ и -σ (СКО).
   * @param freq           Частота данных для форматирования оси X.
   */
  void plotLineStat(const QVector<QPointF>& xys, const QString& name, QColor color,
                    bool showDeviation, const QString& freq) {
    QChart* chart = new QChart();
    auto* series = new QLineSeries();
    QVector<QDateTime> datetimes;
    QVector<double> values;

    for (const auto& pt : xys) {
      qint64 ms = static_cast<qint64>(pt.x());
      double val = pt.y();
      series->append(ms, val);
      datetimes.append(QDateTime::fromMSecsSinceEpoch(ms));
      values.append(val);
    }
    series->setName(name);
    series->setColor(color);
    chart->addSeries(series);

    double mean =
        std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double stddev = std::sqrt(std::accumulate(
        values.begin(), values.end(), 0.0,
        [mean](double sum, double v) { return sum + (v - mean) * (v - mean); }) /
        values.size());

    auto* meanL = new QLineSeries();
    meanL->setColor(Qt::red);
    meanL->setName("Среднее");
    for (int i = 0; i < datetimes.size(); ++i) {
      meanL->append(datetimes[i].toMSecsSinceEpoch(), mean);
    }
    chart->addSeries(meanL);

    if (showDeviation && stddev > 0) {
      auto* upper = new QLineSeries();
      auto* lower = new QLineSeries();
      upper->setName("+σ");
      upper->setColor(Qt::gray);
      lower->setName("-σ");
      lower->setColor(Qt::gray);
      for (int i = 0; i < datetimes.size(); ++i) {
        qint64 ms = datetimes[i].toMSecsSinceEpoch();
        upper->append(ms, values[i] + stddev);
        lower->append(ms, values[i] - stddev);
      }
      chart->addSeries(upper);
      chart->addSeries(lower);
    }

    auto* axisX = new QDateTimeAxis();

    QString format;
    if (freq == "1h" || freq == "3h") {
      format = "HH:mm\ndd.MM";
    } else if (freq == "6h" || freq == "12h") {
      format = "HH:mm\ndd.MM";
    } else {
      format = "dd.MM.yyyy";
    }

    axisX->setFormat(format);
    axisX->setTitleText("Время");
    if (!datetimes.isEmpty()) {
      axisX->setRange(datetimes.first(), datetimes.last());
      axisX->setTickCount(qMin(10, datetimes.size()));
    }
    chart->addAxis(axisX, Qt::AlignBottom);
    for (auto* s : chart->series()) s->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);
    for (auto* s : chart->series()) s->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->setTitle(name);
    setChart(chart);
  }

  /**
   * @brief Сохраняет текущий график как изображение (PNG/JPG).
   * @param parent Родительское окно (для диалога сохранения).
   */
  void saveChart(QWidget* parent) {
    QString fileName = QFileDialog::getSaveFileName(
        parent, "Сохранить график", "", "PNG Files (*.png);;JPEG Files (*.jpg)");
    if (!fileName.isEmpty()) {
      QPixmap pixmap = this->grab();
      pixmap.save(fileName);
    }
  }
};

/**
 * @class MainWindow
 * @brief Главное окно приложения с вкладками и виджетами для работы с погодными данными.
 *
 * Предоставляет интерфейс с тремя вкладками:
 * 1. Текущая погода - показывает актуальные данные и прогноз на 24 часа
 * 2. Статистика (СКО) - отображает графики с стандартным отклонением
 * 3. Статистика (Средние) - показывает графики средних значений
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  /**
   * @brief Конструктор главного окна. Создаёт все вкладки и элементы управления.
   */
  MainWindow() {
    setWindowTitle("Приложение Погода");
    resize(1200, 720);

    tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    setupCurrentWeatherTab();
    setupStdDevTab();
    setupAvgTab();

    chartsDialog = nullptr;
  }

 private slots:
  /**
   * @brief Получает и отображает текущие погодные данные и прогноз.
   * @details Срабатывает при нажатии кнопки "Найти" на вкладке 1.
   */
  void getWeather() {
    QString city = cityEdit->text().trimmed();
    if (city.isEmpty()) {
      QMessageBox::critical(this, "Ошибка", "Введите название города!");
      return;
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QString apiKey = "e8cbc7a2196cd7ab375020c19a6ecbf3";
    QString url = QString("https://api.openweathermap.org/data/2.5/weather?q=%1&appid=%2")
                      .arg(city, apiKey);

    QNetworkReply* reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, [=]() {
      if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Ошибка", "Город не найден.");
        return;
      }

      QByteArray data = reply->readAll();
      QJsonDocument doc = QJsonDocument::fromJson(data);
      auto mainObj = doc.object()["main"].toObject();
      double temp = mainObj["temp"].toDouble() - 273.15;
      double feels = mainObj["feels_like"].toDouble() - 273.15;
      int humidity = mainObj["humidity"].toInt();
      int pressure = mainObj["pressure"].toInt();
      double wind = doc.object()["wind"].toObject()["speed"].toDouble();

      currLabel->setText(
          QString("%1°C, ощущается как %2°C  |  Влажность: %3%%  |  Давление: %4 hPa  |  Ветер: %5 м/с")
              .arg(int(temp))
              .arg(int(feels))
              .arg(humidity)
              .arg(pressure)
              .arg(wind));
      tabs->widget(0)->setStyleSheet(
          QString("background-color: %1;").arg(fon(temp).name()));

      // Получение прогноза на 24 часа (8 точек данных с интервалом 3 часа)
      QString fUrl = QString(
                         "https://api.openweathermap.org/data/2.5/forecast?q=%1&appid=%2&cnt=9")
                         .arg(city, apiKey);
      QNetworkReply* fReply = manager->get(QNetworkRequest(fUrl));
      connect(fReply, &QNetworkReply::finished, [=]() {
        if (fReply->error() != QNetworkReply::NoError) return;

        QByteArray fdata = fReply->readAll();
        QJsonDocument fdoc = QJsonDocument::fromJson(fdata);
        QJsonArray arr = fdoc.object()["list"].toArray();
        QStringList times;
        QVector<double> temps;
        QVector<QColor> colors;

        // Сохраняем данные для использования в showFourChartsCurrent
        forecastData.clear();
        for (const auto& v : arr) {
          QJsonObject obj = v.toObject();
          QDateTime dt =
              QDateTime::fromString(obj["dt_txt"].toString(), "yyyy-MM-dd HH:mm:ss");
          double temp = obj["main"].toObject()["temp"].toDouble() - 273.15;
          double pressure = obj["main"].toObject()["pressure"].toDouble();
          double humidity = obj["main"].toObject()["humidity"].toDouble();
          double wind = obj["wind"].toObject()["speed"].toDouble();

          forecastData.append({dt, temp, pressure, humidity, wind});

          // Для столбчатой диаграммы берем только время (часы)
          times << dt.toString("HH:mm");
          temps << temp;
          colors << fon(temp);
        }
        currChart->plotBarColored(times, temps, colors);
      });
    });
  }

  /**
   * @brief Показывает окно с четырьмя графиками (температура, давление, влажность, ветер) за последние сутки.
   */
  void showFourChartsCurrent() {
    if (forecastData.isEmpty()) {
      QMessageBox::critical(this, "Ошибка",
                           "Сначала введите город и нажмите 'Найти'");
      return;
    }

    if (chartsDialog) delete chartsDialog;
    chartsDialog = new QDialog(this);
    chartsDialog->setWindowTitle("Погодные графики за сутки: " + cityEdit->text());
    chartsDialog->resize(1200, 700);
    QVBoxLayout* vlay = new QVBoxLayout(chartsDialog);
    QGridLayout* grid = new QGridLayout();
    WeatherChart* charts[4];
    QVector<QPointF> tempXY, presXY, humXY, windXY;

    for (const auto& pt : forecastData) {
      qint64 ms = pt.datetime.toMSecsSinceEpoch();
      tempXY << QPointF(ms, pt.temperature);
      presXY << QPointF(ms, pt.pressure);
      humXY << QPointF(ms, pt.humidity);
      windXY << QPointF(ms, pt.windspeed);
    }

    QVector<QPointF> xys[4] = {tempXY, presXY, humXY, windXY};
    QString names[4] = {"Температура", "Давление", "Влажность", "Скорость ветра"};
    QColor cols[4] = {Qt::blue, Qt::darkGreen, Qt::darkYellow, Qt::magenta};

    for (int j = 0; j < 4; ++j) {
      charts[j] = new WeatherChart;
      charts[j]->plotLineStat(xys[j], names[j], cols[j], false, "3h");
      QChart* chart = charts[j]->chart();
      QDateTimeAxis* axisX = qobject_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal).first());
      if (axisX) {
          axisX->setFormat("HH:mm"); 
      }
                
      grid->addWidget(charts[j], j / 2, j % 2);
    }

    vlay->addLayout(grid);
    QPushButton* saveBtn = new QPushButton("Сохранить все графики");
    vlay->addWidget(saveBtn);

    connect(saveBtn, &QPushButton::clicked, chartsDialog, [=]() {
      for (int j = 0; j < 4; ++j) charts[j]->saveChart(chartsDialog);
    });

    chartsDialog->exec();
    for (int j = 0; j < 4; ++j) delete charts[j];
  }

  /**
   * @brief Получает и строит графики (СКО или средние) по заданному городу и периоду.
   * @param showDev   true — строить графики со СКО, false — только средние.
   * @param city      Название города.
   * @param start     Дата начала.
   * @param end       Дата конца.
   * @param freq      Частота: "1h", "3h" и т.д.
   */
  void getPlotStat(bool showDev, QString city, QDate start, QDate end,
                   QString freq) {
    if (city.isEmpty()) {
      QMessageBox::critical(this, "Ошибка", "Введите город!");
      return;
    }

    QString apiKey = "Y8M5RVB7XEMBPVBVMWENAFBWN";
    QString url = QString(
                     "https://weather.visualcrossing.com/VisualCrossingWebServices/"
                     "rest/services/timeline/%1/%2/%3?unitGroup=metric&key=%4")
                     .arg(city)
                     .arg(start.toString("yyyy-MM-dd"))
                     .arg(end.toString("yyyy-MM-dd"))
                     .arg(apiKey);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkReply* reply = manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, [=]() {
      if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Ошибка получения данных.",
                             "Ошибка получения данных.");
        return;
      }

      QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
      auto list = parseVisualCrossingJson(doc, freq);
      if (list.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Нет данных для отображения.");
        return;
      }

      QVector<QPointF> tempXY, presXY, humXY, windXY;
      for (const auto& pt : list) {
        qint64 ms = pt.datetime.toMSecsSinceEpoch();
        tempXY << QPointF(ms, pt.temperature);
        presXY << QPointF(ms, pt.pressure);
        humXY << QPointF(ms, pt.humidity);
        windXY << QPointF(ms, pt.windspeed);
      }

      QVector<QPointF> xys[4] = {tempXY, presXY, humXY, windXY};
      QString names[4] = {"Температура", "Давление", "Влажность", "Скорость ветра"};
      QColor cols[4] = {Qt::blue, Qt::darkGreen, Qt::darkYellow, Qt::magenta};

      for (int j = 0; j < 4; ++j) {
        if (showDev)
          statCharts[j]->plotLineStat(xys[j], names[j], cols[j], true, freq);
        else
          avgCharts[j]->plotLineStat(xys[j], names[j], cols[j], false, freq);
      }
    });
  }

 private:
  /**
   * @brief Настраивает вкладку с текущей погодой.
   */
  void setupCurrentWeatherTab() {
    QWidget* tab1 = new QWidget;
    QVBoxLayout* v1 = new QVBoxLayout(tab1);
    QHBoxLayout* h1 = new QHBoxLayout();

    cityEdit = new QLineEdit;
    QPushButton* findBtn = new QPushButton("Найти");
    h1->addWidget(new QLabel("Введите город:"));
    h1->addWidget(cityEdit);
    h1->addWidget(findBtn);
    v1->addLayout(h1);

    currLabel = new QLabel("Текущие данные будут тут.");
    v1->addWidget(currLabel);

    currChart = new WeatherChart;
    v1->addWidget(currChart);

    QPushButton* saveBtn = new QPushButton("Сохранить график");
    v1->addWidget(saveBtn);

    QPushButton* show4Btn = new QPushButton("Показать 4 графика за сутки");
    v1->addWidget(show4Btn);

    connect(findBtn, &QPushButton::clicked, this, &MainWindow::getWeather);
    connect(saveBtn, &QPushButton::clicked, this,
            [=]() { currChart->saveChart(this); });
    connect(show4Btn, &QPushButton::clicked, this,
            &MainWindow::showFourChartsCurrent);

    tabs->addTab(tab1, "Текущая погода");
  }

  /**
   * @brief Настраивает вкладку со статистикой (СКО).
   */
  void setupStdDevTab() {
    QWidget* tab2 = new QWidget;
    QVBoxLayout* v2 = new QVBoxLayout(tab2);
    QHBoxLayout* h2 = new QHBoxLayout();

    cityEditStd = new QLineEdit;
    QDateEdit* startDateStd = new QDateEdit(QDate::currentDate().addDays(-7));
    QDateEdit* endDateStd = new QDateEdit(QDate::currentDate());
    freqBoxStd = new QComboBox;
    freqBoxStd->addItems({"1h", "3h", "6h", "12h", "1d"});
    QPushButton* plotBtnStd = new QPushButton("Построить графики");

    h2->addWidget(new QLabel("Город:"));
    h2->addWidget(cityEditStd);
    h2->addWidget(new QLabel("Дата начала:"));
    h2->addWidget(startDateStd);
    h2->addWidget(new QLabel("Дата конца:"));
    h2->addWidget(endDateStd);
    h2->addWidget(new QLabel("Частота:"));
    h2->addWidget(freqBoxStd);
    h2->addWidget(plotBtnStd);
    v2->addLayout(h2);

    QGridLayout* grid2 = new QGridLayout();
    for (int i = 0; i < 4; ++i) {
      statCharts[i] = new WeatherChart;
      grid2->addWidget(statCharts[i], i / 2, i % 2);
    }
    v2->addLayout(grid2);

    QPushButton* saveBtnStd =
        new QPushButton("Сохранить графики (по отдельности)");
    v2->addWidget(saveBtnStd);

    connect(plotBtnStd, &QPushButton::clicked, this, [=]() {
      getPlotStat(true, cityEditStd->text(), startDateStd->date(),
                  endDateStd->date(), freqBoxStd->currentText());
    });
    connect(saveBtnStd, &QPushButton::clicked, this, [=]() {
      for (int i = 0; i < 4; ++i) statCharts[i]->saveChart(this);
    });

    tabs->addTab(tab2, "Статистика (СКО)");
  }

  /**
   * @brief Настраивает вкладку со статистикой (средние значения).
   */
  void setupAvgTab() {
    QWidget* tab3 = new QWidget;
    QVBoxLayout* v3 = new QVBoxLayout(tab3);
    QHBoxLayout* h3 = new QHBoxLayout();

    cityEditAvg = new QLineEdit;
    QDateEdit* startDateAvg = new QDateEdit(QDate::currentDate().addDays(-7));
    QDateEdit* endDateAvg = new QDateEdit(QDate::currentDate());
    freqBoxAvg = new QComboBox;
    freqBoxAvg->addItems({"1h", "3h", "6h", "12h", "1d"});
    QPushButton* plotBtnAvg = new QPushButton("Построить графики");

    h3->addWidget(new QLabel("Город:"));
    h3->addWidget(cityEditAvg);
    h3->addWidget(new QLabel("Дата начала:"));
    h3->addWidget(startDateAvg);
    h3->addWidget(new QLabel("Дата конца:"));
    h3->addWidget(endDateAvg);
    h3->addWidget(new QLabel("Частота:"));
    h3->addWidget(freqBoxAvg);
    h3->addWidget(plotBtnAvg);
    v3->addLayout(h3);

    QGridLayout* grid3 = new QGridLayout();
    for (int i = 0; i < 4; ++i) {
      avgCharts[i] = new WeatherChart;
      grid3->addWidget(avgCharts[i], i / 2, i % 2);
    }
    v3->addLayout(grid3);

    QPushButton* saveBtnAvg =
        new QPushButton("Сохранить графики (по отдельности)");
    v3->addWidget(saveBtnAvg);

    connect(plotBtnAvg, &QPushButton::clicked, this, [=]() {
      getPlotStat(false, cityEditAvg->text(), startDateAvg->date(),
                  endDateAvg->date(), freqBoxAvg->currentText());
    });
    connect(saveBtnAvg, &QPushButton::clicked, this, [=]() {
      for (int i = 0; i < 4; ++i) avgCharts[i]->saveChart(this);
    });

    tabs->addTab(tab3, "Статистика (Средние)");
  }

  QTabWidget* tabs;
  QLineEdit *cityEdit, *cityEditStd, *cityEditAvg;
  QComboBox *freqBoxStd, *freqBoxAvg;
  QLabel* currLabel;
  WeatherChart* currChart;
  WeatherChart* statCharts[4];
  WeatherChart* avgCharts[4];
  QDialog* chartsDialog;
  QList<WeatherPoint> forecastData;  ///< Данные прогноза для текущей погоды
};

#include "main.moc"

/**
 * @brief Точка входа в программу.
 * @param argc Количество аргументов командной строки.
 * @param argv Массив аргументов командной строки.
 * @return Код завершения (0 — успешно).
 */
int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
