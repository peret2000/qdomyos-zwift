#include "zwiftworkout.h"
#include <QXmlStreamReader>

QList<trainrow> zwiftworkout::load(const QString &filename) {
    QSettings settings;
    // QList<trainrow> list; //NOTE: clazy-unuzed-non-trivial-variable
    QFile input(filename);
    input.open(QIODevice::ReadOnly);
    return load(input.readAll());
}

bool zwiftworkout::durationAsDistance(QString sportType, QString durationType) {
    // watch out to the compatibility with HomeFitnessBuddy
    if (!sportType.toLower().contains(QStringLiteral("run")))
        return false;
    else if (sportType.toLower().contains(QStringLiteral("run")) &&
             durationType.toLower().contains(QStringLiteral("time")))
        return false;
    else
        return true;
}

double zwiftworkout::speedFromPace(double Pace) {
    QSettings settings;
    double speed = 0;
    QString pace_default = settings.value(QStringLiteral("pace_default"), QStringLiteral("Half Marathon")).toString();
    if (Pace == 0) {
        speed = settings.value(QStringLiteral("pace_1mile"), QStringLiteral("Half Marathon")).toInt();
    } else if (Pace == 1) {
        speed = settings.value(QStringLiteral("pace_5km"), QStringLiteral("Half Marathon")).toInt();
    } else if (Pace == 2) {
        speed = settings.value(QStringLiteral("pace_10km"), QStringLiteral("Half Marathon")).toInt();
    } else if (Pace == 3) {
        speed = settings.value(QStringLiteral("pace_halfmarathon"), QStringLiteral("Half Marathon")).toInt();
    } else if (Pace == 4) {
        speed = settings.value(QStringLiteral("pace_marathon"), QStringLiteral("Half Marathon")).toInt();
    } else {
        if (!pace_default.compare(QStringLiteral("1 mile")))
            speed = settings.value(QStringLiteral("pace_1mile"), QStringLiteral("Half Marathon")).toInt();
        else if (!pace_default.compare(QStringLiteral("5 km")))
            speed = settings.value(QStringLiteral("pace_5km"), QStringLiteral("Half Marathon")).toInt();
        else if (!pace_default.compare(QStringLiteral("10 km")))
            speed = settings.value(QStringLiteral("pace_10km"), QStringLiteral("Half Marathon")).toInt();
        else if (!pace_default.compare(QStringLiteral("Half Marathon")))
            speed = settings.value(QStringLiteral("pace_halfmarathon"), QStringLiteral("Half Marathon")).toInt();
        else
            speed = settings.value(QStringLiteral("pace_marathon"), QStringLiteral("Half Marathon")).toInt();
    }

    return speed;
}

QList<trainrow> zwiftworkout::load(const QByteArray &input) {
    QSettings settings;
    QList<trainrow> list;
    QXmlStreamReader stream(input);
    double thresholdSecPerKm = 0;
    QString sportType = QStringLiteral("");
    QString durationType = QStringLiteral("");
    while (!stream.atEnd()) {
        stream.readNext();
        QString name = stream.name().toString();
        QString text = stream.text().toString();
        QXmlStreamAttributes atts = stream.attributes();
        if (name.toLower().contains(QStringLiteral("thresholdsecperkm")) && thresholdSecPerKm == 0) {
            stream.readNext();
            thresholdSecPerKm = stream.text().toDouble();
        } else if (name.toLower().contains(QStringLiteral("sporttype")) && sportType.length() == 0) {
            stream.readNext();
            sportType = stream.text().toString();
        } else if (name.toLower().contains(QStringLiteral("durationtype")) && durationType.length() == 0) {
            stream.readNext();
            durationType = stream.text().toString();
        } else if (!atts.isEmpty()) {
            if (stream.name().contains(QStringLiteral("IntervalsT"))) {
                uint32_t repeat = 1;
                uint32_t OnDuration = 1;
                uint32_t OffDuration = 1;
                double OnPower = 1;
                double OffPower = 1;
                double Pace = -1;
                if (atts.hasAttribute(QStringLiteral("Repeat"))) {
                    repeat = atts.value(QStringLiteral("Repeat")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("OnDuration"))) {
                    OnDuration = atts.value(QStringLiteral("OnDuration")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("OffDuration"))) {
                    OffDuration = atts.value(QStringLiteral("OffDuration")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("OnPower"))) {
                    OnPower = atts.value(QStringLiteral("OnPower")).toDouble();
                }
                if (atts.hasAttribute(QStringLiteral("OffPower"))) {
                    OffPower = atts.value(QStringLiteral("OffPower")).toDouble();
                }
                if (atts.hasAttribute(QStringLiteral("pace"))) {
                    Pace = atts.value(QStringLiteral("pace")).toUInt();
                }

                for (uint32_t i = 0; i < repeat; i++) {
                    trainrow row;
                    if (!durationAsDistance(sportType, durationType))
                        row.duration = QTime(OnDuration / 3600, OnDuration / 60, OnDuration % 60, 0);
                    else
                        row.distance = OnDuration / 1000.0;
                    if (sportType.toLower().contains(QStringLiteral("run"))) {
                        row.forcespeed = 1;
                        double speed = speedFromPace(Pace);
                        row.speed = ((60.0 / speed) * 60.0) * OnPower;
                    } else {
                        row.power = OnPower * settings.value(QStringLiteral("ftp"), 200.0).toDouble();
                    }
                    list.append(row);
                    if (!durationAsDistance(sportType, durationType))
                        row.duration = QTime(OffDuration / 3600, OffDuration / 60, OffDuration % 60, 0);
                    else
                        row.distance = OffDuration / 1000.0;
                    if (sportType.toLower().contains(QStringLiteral("run"))) {
                        row.forcespeed = 1;
                        double speed = speedFromPace(Pace);
                        row.speed = ((60.0 / speed) * 60.0) * OffPower;
                    } else {
                        row.power = OffPower * settings.value(QStringLiteral("ftp"), 200.0).toDouble();
                    }
                    list.append(row);
                }
            } else if (stream.name().contains(QStringLiteral("FreeRide"))) {
                uint32_t Duration = 1;
                // double FlatRoad = 1;
                if (atts.hasAttribute(QStringLiteral("Duration"))) {
                    Duration = atts.value(QStringLiteral("Duration")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("FlatRoad"))) {
                    // NOTE: Value stored to FlatRoad is never read clang-analyzer-deadcode.DeadStores
                    // FlatRoad = atts.value(QStringLiteral("FlatRoad")).toDouble();
                }

                trainrow row;
                if (!durationAsDistance(sportType, durationType))
                    row.duration = QTime(Duration / 3600, Duration / 60, Duration % 60, 0);
                else
                    row.distance = ((double)Duration) / 1000.0;
                list.append(row);
            } else if (stream.name().contains(QStringLiteral("Ramp")) ||
                       stream.name().contains(QStringLiteral("Warmup"), Qt::CaseInsensitive) ||
                       stream.name().contains(QStringLiteral("Cooldown"))) {
                uint32_t Duration = 1;
                double PowerLow = 1;
                double PowerHigh = 1;
                double Pace = -1;
                if (atts.hasAttribute(QStringLiteral("Duration"))) {
                    Duration = atts.value(QStringLiteral("Duration")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("PowerLow"))) {
                    PowerLow = atts.value(QStringLiteral("PowerLow")).toDouble();
                }
                if (atts.hasAttribute(QStringLiteral("PowerHigh"))) {
                    PowerHigh = atts.value(QStringLiteral("PowerHigh")).toDouble();
                }
                if (atts.hasAttribute(QStringLiteral("pace"))) {
                    Pace = atts.value(QStringLiteral("pace")).toUInt();
                }

                for (uint32_t i = 0; i < Duration; i++) {
                    trainrow row;
                    if (!durationAsDistance(sportType, durationType))
                        row.duration = QTime(0, 0, 1, 0);
                    else
                        row.distance = 0.001;
                    if (PowerHigh > PowerLow) {
                        if (sportType.toLower().contains(QStringLiteral("run"))) {
                            row.forcespeed = 1;
                            double speed = speedFromPace(Pace);
                            row.speed =
                                ((60.0 / speed) * 60.0) * (PowerLow + (((PowerHigh - PowerLow) / Duration) * i));
                        } else {
                            row.power = (PowerLow + (((PowerHigh - PowerLow) / Duration) * i)) *
                                        settings.value(QStringLiteral("ftp"), 200.0).toDouble();
                        }
                    } else {
                        if (sportType.toLower().contains(QStringLiteral("run"))) {
                            row.forcespeed = 1;
                            double speed = speedFromPace(Pace);
                            row.speed =
                                ((60.0 / speed) * 60.0) * (PowerLow + (((PowerHigh - PowerLow) / Duration) * i));
                        } else {
                            row.power = (PowerLow - (((PowerLow - PowerHigh) / Duration) * i)) *
                                        settings.value(QStringLiteral("ftp"), 200.0).toDouble();
                        }
                    }
                    list.append(row);
                }
            } else if (stream.name().contains(QStringLiteral("SteadyState"))) {
                uint32_t Duration = 1;
                double Power = 1;
                double Pace = -1;
                trainrow row;

                if (atts.hasAttribute(QStringLiteral("Duration"))) {
                    Duration = atts.value(QStringLiteral("Duration")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("pace"))) {
                    Pace = atts.value(QStringLiteral("pace")).toUInt();
                }
                if (atts.hasAttribute(QStringLiteral("Power"))) {
                    if (sportType.toLower().contains(QStringLiteral("run")) && Duration != 1) {
                        if (thresholdSecPerKm != 0) {
                            row.forcespeed = 1;
                            row.speed =
                                (60.0 / (thresholdSecPerKm / atts.value(QStringLiteral("Power")).toDouble())) * 60.0;
                        } else {
                            double speed = speedFromPace(Pace);
                            row.forcespeed = 1;
                            row.speed = ((60.0 / speed) * 60.0) * atts.value(QStringLiteral("Power")).toDouble();
                        }
                    } else {
                        Power = atts.value(QStringLiteral("Power")).toDouble();
                    }
                }

                row.power = Power * settings.value(QStringLiteral("ftp"), 200.0).toDouble();
                if (!durationAsDistance(sportType, durationType))
                    row.duration = QTime(Duration / 3600, Duration / 60, Duration % 60, 0);
                else
                    row.distance = Duration / 1000.0;
                list.append(row);
            }
        }
    }
    return list;
}
