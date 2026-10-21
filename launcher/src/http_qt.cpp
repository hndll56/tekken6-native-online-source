#include "http.h"
static int request(const QString& url, const QByteArray& verb, const QString& headers,
                   const QByteArray& body, QByteArray& out, bool local,
                   const std::function<void(qint64,qint64)>& progress = {}) {
    out.clear();
    const QUrl u(url);
    if (!u.isValid() || (u.scheme() != "http" && u.scheme() != "https")) return 0;
    QNetworkAccessManager manager;
    if (local) manager.setProxy(QNetworkProxy::NoProxy);
    QNetworkRequest req(u);
    req.setRawHeader("User-Agent", "Tekken6NativeOnlineLauncher");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, local ? QNetworkRequest::ManualRedirectPolicy : QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(local ? 3000 : 30000);
    for (const auto& line : headers.split("\r\n", Qt::SkipEmptyParts)) {
        int colon = line.indexOf(':');
        if (colon > 0) req.setRawHeader(line.left(colon).toLatin1(), line.mid(colon+1).trimmed().toLatin1());
    }
    auto* reply = manager.sendCustomRequest(req, verb, body);
    reply->setReadBufferSize(local ? (1 << 20) + 1 : 4 << 20);
    QEventLoop loop;
    bool overflow = false;
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&] {
        out += reply->readAll();
        if (local && out.size() > (1 << 20)) { overflow = true; reply->abort(); }
    });
    if (progress) QObject::connect(reply, &QNetworkReply::downloadProgress, &loop, progress);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    out += reply->readAll();
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // HTTP errors still carry their status/body for UPnP SOAP error handling.
    if (overflow || !code || (reply->error() != QNetworkReply::NoError && code < 400)) code = 0;
    delete reply;
    return code;
}
int httpGet(const wchar_t* url, QByteArray& out, const std::function<void(qint64,qint64)>& progress) {
    return request(QString::fromWCharArray(url), "GET", "Accept: application/vnd.github+json, application/octet-stream", {}, out, false, progress);
}
int httpLocal(const QString& url, const wchar_t* verb, const QString& headers, const QByteArray& body, QByteArray& out) {
    return request(url, QString::fromWCharArray(verb).toLatin1(), headers, body, out, true);
}
