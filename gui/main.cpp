#include <QApplication>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QPixmap>
#include "MainWindow.h"
#include "../utils/PathUtil.h"

int main(int argc, char *argv[])
{
    // Enable high‑DPI support (optional but looks great)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    // ------------------------------------------------------------------
    // Command‑line parsing – "--cli" forces the plain text menu.
    // ------------------------------------------------------------------
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({{"c", "cli"}, "Run in pure‑CLI mode (no GUI)."});
    parser.process(app);

    if (parser.isSet("cli")) {
        // ----------------------------------------------------------------
        // CLI mode – reuse the original console menu implementation.
        // ----------------------------------------------------------------
        // NOTE: The original CLI entry point resides in "main.cpp" (root).
        // We simply include that file's logic here to avoid a second binary.
        extern int runConsoleMenu(); // forward declaration (define in utils/Menu.cpp)
        return runConsoleMenu();
    }

    // ------------------------------------------------------------------
    // GUI mode – optional splash screen while the application starts.
    // ------------------------------------------------------------------
    QPixmap splashPixmap(PathUtil::resolvePath("gui/splash.png")); // place your image here.
    QSplashScreen splash(splashPixmap);
    splash.show();
    // Process events so the splash actually appears before we build the UI.
    app.processEvents();

    MainWindow w;
    w.show();
    // Fade out splash when the main window is ready.
    splash.finish(&w);
    return app.exec();
}

