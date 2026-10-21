#include "processes.h"
#include <cstdio>
#include <unistd.h>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    QProcess child;
    prepareLinuxChild(&child);
    child.setProgram("/bin/sleep"); child.setArguments({"30"}); child.start();
    if(!child.waitForStarted(3000)) return 1;
    printf("%lld\n",static_cast<long long>(child.processId())); fflush(stdout);
    for(;;) pause();
}
