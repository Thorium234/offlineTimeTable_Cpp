#include <QCoreApplication>
#include <QApplication>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QPixmap>
#include "gui/MainWindow.h"
#include "utils/Menu.h"
#include "utils/PathUtil.h"
#include <cstdlib>
#include <string>

int main(int argc, char *argv[])
{
    // First, check if CLI mode is requested via environment or arguments
    bool cliMode = false;
    bool helpMode = false;
    
    // Check arguments manually to avoid instantiating QApplication when display is missing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--cli" || arg == "-c") {
            cliMode = true;
        } else if (arg == "--help" || arg == "-h" || arg == "-?") {
            helpMode = true;
        }
    }
    
    // Also check if DISPLAY and WAYLAND_DISPLAY are empty (headless environment)
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if (!display && !wayland) {
        cliMode = true;
    }

    if (cliMode || helpMode) {
        // Set up a QCoreApplication (headless, doesn't require GUI display server)
        QCoreApplication app(argc, argv);
        
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addOption({{"c", "cli"}, "Run in pure‑CLI mode (no GUI)."});
        parser.process(app); // Automatically handles --help / -h and exits if requested

        if (helpMode) {
            return 0;
        }

        Menu menu;
        menu.run();
        return 0;
    }

    // Otherwise, start the GUI application (requires display server)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    // Show a splash screen while constructing the main window
    QPixmap splashPixmap(PathUtil::resolvePath("gui/splash.png"));
    QSplashScreen splash(splashPixmap);
    splash.show();
    app.processEvents();

    MainWindow w;
    w.show();
    splash.finish(&w);
    
    return app.exec();
}
