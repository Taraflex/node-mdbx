#pragma once

#include <napi.h>
#include "mdbx.h"
#include "db_env.h"
#include "utils.h"

class CppDbi : public Napi::ObjectWrap<CppDbi>
{
public:
    CppDbi(const Napi::CallbackInfo & info);

    static Napi::Function GetClass(Napi::Env env);

    void Init(const DbEnvPtr &dbEnvPtr, MDBX_dbi dbDbi);

    Napi::Value Put(const Napi::CallbackInfo& info);
    Napi::Value Get(const Napi::CallbackInfo& info);
    Napi::Value Del(const Napi::CallbackInfo& info);
    Napi::Value Has(const Napi::CallbackInfo& info);

    Napi::Value FirstKey(const Napi::CallbackInfo& info);
    Napi::Value LastKey(const Napi::CallbackInfo& info);
    Napi::Value NextKey(const Napi::CallbackInfo& info);
    Napi::Value PrevKey(const Napi::CallbackInfo& info);
    Napi::Value LowerBoundKey(const Napi::CallbackInfo& info);

private:
    void _check(Napi::Env &env);
    Napi::Value _outKey(Napi::Buffer<char> &buffer);

    DbEnvPtr _dbEnvPtr;
    MDBX_dbi _dbDbi = 0;
    buffer_t _keyBuffer;
    buffer_t _valueBuffer;
};
