#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#include "preflight.h"

static const uint8_t target[32]={
    0x41,0x27,0xcd,0x6a,0x79,0x2a,0xe5,0x28,0xbc,0xa6,0xf6,0x5a,0x88,0x73,0xdd,0x61,
    0x78,0x95,0x91,0x93,0x7d,0x87,0x65,0x6c,0x2b,0x58,0x6a,0x5e,0x30,0xeb,0x77,0xea};
int es_check_executable(const wchar_t *path,uint8_t actual[32]) {
    if(!path || !actual)return -1;
    memset(actual,0,32);
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return -1;
    BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;
    int result=-1;DWORD received=0;uint8_t buffer[65536],digest[32];
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)goto done;
    for(;;) {
        if(!ReadFile(file,buffer,sizeof(buffer),&received,NULL))goto done;
        if(!received)break;
        if(BCryptHashData(hash,buffer,received,0)<0)goto done;
    }
    if(BCryptFinishHash(hash,digest,32,0)<0)goto done;
    memcpy(actual,digest,32);result=memcmp(target,digest,32)==0;
done:
    if(hash)BCryptDestroyHash(hash);
    if(alg)BCryptCloseAlgorithmProvider(alg,0);
    CloseHandle(file);return result;
}
const char *es_runtime_status(void) {
    return "Supported by Expanded Storage 0.2.0 (Storage capacity patch).";
}
