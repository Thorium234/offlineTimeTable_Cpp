#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QProcess>
#include <QDir>
#include <QThread>
#include <QFileInfo>
#include <QDebug>
#include <cstdlib>
#include <iostream>

#include "gui/MainWindow.h"
#include "utils/PathUtil.h"
#include "services/DataManager.h"
#include "services/TimetableEngine.h"
#include "timetable/Timetable.h"

static QProcess *flaskProcess = nullptr;
static int chosenPort = -1;

static QString findProjectRoot()
{
    QString dir = QCoreApplication::applicationDirPath();
    while (true) {
        if (QFileInfo::exists(dir + "/web/app.py"))
            return dir;
        QDir up(dir);
        up.cdUp();
        if (up.absolutePath() == dir)
            break;
        dir = up.absolutePath();
    }
    return QDir::currentPath();
}

static void killPort(int port)
{
    QProcess k;
    QString cmd = QString(
        "PID=$(lsof -ti:%1 2>/dev/null) && "
        "if [ -n \"$PID\" ]; then "
        "  CMD=$(ps -p $PID -o comm= 2>/dev/null); "
        "  if echo \"$CMD\" | grep -qiE 'python|flask'; then "
        "    kill $PID 2>/dev/null; "
        "  fi; "
        "fi; true"
    ).arg(port);
    k.start("bash", {"-c", cmd});
    k.waitForFinished(2000);
}

static int startFlaskWebServer(const QString &projectRoot)
{
    killPort(5000);

    for (int port = 5000; port <= 5009; ++port) {
        killPort(port);
        QThread::msleep(200);

        QProcess *flask = new QProcess();
        flask->setProcessChannelMode(QProcess::ForwardedChannels);
        flask->setWorkingDirectory(projectRoot);

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("FLASK_PORT", QString::number(port));
        env.insert("FLASK_SILENT_LOGS", "1");
        flask->setProcessEnvironment(env);

        flask->start("python3", {"web/app.py"});

        if (!flask->waitForStarted(5000)) {
            qWarning() << "[WebServer] Failed to start Flask on port" << port << ":" << flask->errorString();
            delete flask;
            continue;
        }

        QThread::msleep(1500);

        bool ready = false;
        for (int attempt = 0; attempt < 10; ++attempt) {
            if (flask->state() != QProcess::Running)
                break;
            QProcess probe;
            probe.start("curl", {"-sf", "--max-time", "1",
                QString("http://127.0.0.1:%1/").arg(port)});
            probe.waitForFinished(2000);
            if (probe.exitCode() == 0) {
                ready = true;
                break;
            }
            QThread::msleep(300);
        }

        if (ready) {
            flaskProcess = flask;
            chosenPort = port;
            qDebug().noquote() << QString("[WebServer] Flask ready on http://127.0.0.1:%1").arg(port);
            return port;
        }

        qWarning() << "[WebServer] Flask on port" << port << "did not become ready, trying next...";
        flask->terminate();
        flask->waitForFinished(2000);
        delete flask;
    }

    qWarning() << "[WebServer] Could not start Flask on any port (5000-5009).";
    return -1;
}

static void stopFlaskWebServer()
{
    if (!flaskProcess)
        return;
    qDebug() << "[WebServer] Shutting down Flask...";
    flaskProcess->terminate();
    if (!flaskProcess->waitForFinished(5000)) {
        flaskProcess->kill();
        flaskProcess->waitForFinished(3000);
    }
    delete flaskProcess;
    flaskProcess = nullptr;
    chosenPort = -1;
}

static int solveFromDb(const QString &projectRoot)
{
    Q_UNUSED(projectRoot);
    DataManager dm;
    if (!dm.loadFromDB()) {
        std::cerr << "{\"error\":\"Failed to open database\"}\n";
        return 1;
    }

    if (dm.lessons.empty()) {
        std::cout << "{\"score\":0,\"slots\":0,\"unscheduled\":0,\"message\":\"No lessons in database\"}\n";
        return 0;
    }

    auto old_buf = std::cout.rdbuf();
    std::cout.rdbuf(std::cerr.rdbuf());

    TimetableEngine engine;
    Timetable result = engine.generate(dm);

    std::cout.rdbuf(old_buf);

    dm.sql.clearTimetableSlots();

    int slotsWritten = 0;
    for (auto it = result.schedules.begin(); it != result.schedules.end(); ++it) {
        int classId = it->first;
        const auto &schedule = it->second;
        for (int dayIdx = 0; dayIdx < (int)schedule.size(); ++dayIdx) {
            const auto &dayRow = schedule[dayIdx];
            for (int perIdx = 0; perIdx < (int)dayRow.size(); ++perIdx) {
                const TimetableCell &cell = dayRow[perIdx];
                if (!cell.isEmpty()) {
                    dm.sql.addTimetableSlot(classId, dayIdx + 1, perIdx + 1,
                                            cell.subjectId, cell.teacherId, cell.roomId);
                    slotsWritten++;
                }
            }
        }
    }

    std::cout << "{\"score\":" << result.score
              << ",\"slots\":" << slotsWritten
              << ",\"unscheduled\":" << result.unscheduledLessons.size()
              << "}\n";
    return 0;
}

static void printWebUrl()
{
    if (chosenPort > 0) {
        std::cout << "\n  Web interface: http://127.0.0.1:" << chosenPort << "\n";
    }
}

int main(int argc, char *argv[])
{
    bool solveDbMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--solve-from-db") {
            solveDbMode = true;
        }
    }

    // Headless solver mode (invoked by Flask /api/solve)
    if (solveDbMode) {
        QCoreApplication app(argc, argv);
        QString projectRoot = findProjectRoot();
        return solveFromDb(projectRoot);
    }

    // GUI mode: start Flask then launch the Qt application window
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    QString projectRoot = findProjectRoot();
    qDebug().noquote() << "[Project Root]" << projectRoot;

    if (startFlaskWebServer(projectRoot) > 0) {
        printWebUrl();
    } else {
        qWarning() << "[WebServer] Could not start web server. GUI works standalone.";
    }

    QPixmap splashPixmap(PathUtil::resolvePath("gui/splash.png"));
    QSplashScreen splash(splashPixmap);
    splash.show();
    app.processEvents();

    MainWindow w;
    w.show();
    splash.finish(&w);

    int ret = app.exec();
    stopFlaskWebServer();
    return ret;
}
