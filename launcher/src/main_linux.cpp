#include "launcher.h"
#include <cstdio>
int main(int argc, char** argv) {
    for(int i=1;i<argc;++i) if(strcmp(argv[i],"--version")==0) { puts(kVersion); return 0; }
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Tekken6NativeOnline");
    QCoreApplication::setApplicationName("Tekken6NativeOnline");
    const auto args = app.arguments();
    g_updateTest=args.contains("--update-test");
    if (args.contains("--version")) { puts(kVersion); return 0; }
    if (int i = args.indexOf("--instance"); i >= 0 && i + 1 < args.size()) g_instance = std::max(1, args[i+1].toInt());
    const QString locks = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString identity = QString::fromLatin1(QCryptographicHash::hash(root().toUtf8(), QCryptographicHash::Sha256).toHex().left(16));
    QLockFile lock(QDir(locks).filePath(QString("t6-%1-%2.lock").arg(identity).arg(g_instance)));
    if (!lock.tryLock(10000)) { QMessageBox::information(nullptr, "Tekken 6 Online", "Tekken 6 Online is already running."); return 0; }
    for (const char* f : {":/fonts/BaiJamjuree-Regular.ttf", ":/fonts/BaiJamjuree-Medium.ttf", ":/fonts/BaiJamjuree-SemiBold.ttf", ":/fonts/BaiJamjuree-Bold.ttf"}) {
        int id = QFontDatabase::addApplicationFont(f);
        auto families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty()) g_body = g_display = families.first();
    }
    app.setFont(QFont(g_body, 10));
    app.setStyleSheet(QString(kStyleTemplate).replace("%BODY%", g_body).replace("%DISPLAY%", g_display) + (QFile::exists(":/art/ui/field.png") ? kArtStyle : ""));
    try {
        Launcher w;
        app.setWindowIcon(QIcon(":/app.ico"));
        if(g_updateTest) { w.setAttribute(Qt::WA_DontShowOnScreen); QTimer::singleShot(60000,&app,&QCoreApplication::quit); }
        w.show();
        if (int i = args.indexOf("--smoke-test-ms"); i >= 0 && i + 1 < args.size())
            QTimer::singleShot(args[i+1].toInt(), &app, &QCoreApplication::quit);
        return app.exec();
    } catch (const std::exception& e) { fprintf(stderr, "Launcher exception: %s\n", e.what()); return 1; }
}
