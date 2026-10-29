#include <stdio.h>
#include <wchar.h>
#include "preflight.h"
int wmain(int argc,wchar_t **argv) {
    if(argc!=3 || wcscmp(argv[1],L"--game")) {
        fputs("Usage: es_preflight.exe --game <Mewgenics.exe>\nRead-only: no process launch, hooks or save access.\n",stderr);
        return 64;
    }
    uint8_t digest[32];int result=es_check_executable(argv[2],digest);
    if(result<0){fputs("Cannot verify executable. No activation.\n",stderr);return 2;}
    printf("SHA256=");for(unsigned i=0;i<32;i++)printf("%02x",digest[i]);puts("");
    if(!result){puts("UNSUPPORTED EXECUTABLE. No activation.");return 1;}
    puts("Exact target hash matched: Steam build 25143593 / game 1.1.21239.");
    puts(es_runtime_status());return 0;
}
