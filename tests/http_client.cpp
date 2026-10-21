#include "http.h"
#include <cstdio>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if(argc!=3) return 2;
    if(!QSslSocket::supportsSsl()) { fprintf(stderr,"Qt TLS backend unavailable\n"); return 3; }
    const QString base=QString::fromUtf8(argv[1]);
    QByteArray body;
    if(httpLocal(base+"/ok",L"GET",{},{},body)!=200 || body!="hello") return 4;
    if(httpLocal(base+"/error",L"POST","Content-Type: text/xml","test",body)!=500 || body!="soap-error") return 5;
    if(httpLocal(base+"/large",L"GET",{},{},body)!=0) return 6;
    if(httpGet((base+"/redirect").toStdWString().c_str(),body)!=200 || body!="hello") return 7;
    QString tls=QString::fromUtf8(argv[2]);
    if(httpGet(tls.toStdWString().c_str(),body)!=0) return 8; // untrusted local certificate must be rejected
    auto config=QSslConfiguration::defaultConfiguration();
    auto certs=QSslCertificate::fromPath(qEnvironmentVariable("T6_TEST_CA"));
    if(certs.isEmpty()) return 9;
    auto ca=config.caCertificates(); ca.append(certs); config.setCaCertificates(ca); QSslConfiguration::setDefaultConfiguration(config);
    if(httpGet(tls.toStdWString().c_str(),body)!=200 || body!="hello") return 10;
    int workerStatus=0;
    std::thread worker([&] { QByteArray reply; workerStatus=httpLocal(base+"/ok",L"GET",{},{},reply); if(reply!="hello") workerStatus=0; });
    worker.join(); if(workerStatus!=200) return 11;
    puts("HTTP GET/POST/errors/redirect/body cap, worker thread and TLS trust/rejection: PASS");
}
