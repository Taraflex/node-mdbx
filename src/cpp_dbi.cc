#include "cpp_dbi.h"
#include "utils.h"

CppDbi::CppDbi(const Napi::CallbackInfo & info): Napi::ObjectWrap<CppDbi>(info) {};

Napi::Function CppDbi::GetClass(Napi::Env env) {
    return DefineClass(env, "CppDbi", {
        CppDbi::InstanceMethod("put", &CppDbi::Put),
        CppDbi::InstanceMethod("get", &CppDbi::Get),
        CppDbi::InstanceMethod("del", &CppDbi::Del),
        CppDbi::InstanceMethod("has", &CppDbi::Has),

        CppDbi::InstanceMethod("first", &CppDbi::FirstKey),
        CppDbi::InstanceMethod("last", &CppDbi::LastKey),
        CppDbi::InstanceMethod("next", &CppDbi::NextKey),
        CppDbi::InstanceMethod("prev", &CppDbi::PrevKey),
        CppDbi::InstanceMethod("lowerBound", &CppDbi::LowerBoundKey),
    });
}

void CppDbi::Init(const DbEnvPtr &dbEnvPtr, MDBX_dbi dbDbi) {
    _dbEnvPtr = dbEnvPtr;
    _dbDbi = dbDbi;
}

Napi::Value CppDbi::Put(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);

    ExtractBuffer(info[0], _keyBuffer);
    ExtractBuffer(info[1], _valueBuffer);

    return wrapException(env, [&] () {
        MDBX_val key = CreateMdbxVal(_keyBuffer);
        MDBX_val value = CreateMdbxVal(_valueBuffer);

        const int rc = mdbx_put(_dbEnvPtr->GetTransaction(), _dbDbi, &key, &value, MDBX_UPSERT);
        CheckMdbxResult(rc);

        return env.Undefined();
    });
}

Napi::Value CppDbi::Get(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);

    ExtractBuffer(info[0], _keyBuffer);

    return wrapException(env, [&] () {
        MDBX_val key = CreateMdbxVal(_keyBuffer);
        MDBX_val value;

        const int rc = mdbx_get(_dbEnvPtr->GetTransaction(), _dbDbi, &key, &value);
        if (rc == MDBX_NOTFOUND)
            return env.Undefined();
        CheckMdbxResult(rc);

        return (Napi::Value) Napi::Buffer<char>::Copy(env, (const char *)value.iov_base, value.iov_len);
    });
}

Napi::Value CppDbi::Del(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);

    ExtractBuffer(info[0], _keyBuffer);

    return wrapException(env, [&] () {
        MDBX_val key = CreateMdbxVal(_keyBuffer);
        const int rc = mdbx_del(_dbEnvPtr->GetTransaction(), _dbDbi, &key, NULL);
        CheckMdbxResult(rc);

        return env.Undefined();
    });
}

Napi::Value CppDbi::Has(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);

    ExtractBuffer(info[0], _keyBuffer);

    return wrapException(env, [&] () {
        MDBX_val key = CreateMdbxVal(_keyBuffer);
        MDBX_val value;

        const int rc = mdbx_get(_dbEnvPtr->GetTransaction(), _dbDbi, &key, &value);
        if (rc == MDBX_NOTFOUND)
            return Napi::Value::From(env, false);
        CheckMdbxResult(rc);

        return Napi::Value::From(env, true);
    });
}

Napi::Value CppDbi::FirstKey(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);

    return wrapException(env, [&] () {
        MDBX_cursor *dbCur = NULL;
        MDBX_val key;

        int rc = MDBX_SUCCESS;
        try {
            rc = mdbx_cursor_open(_dbEnvPtr->GetTransaction(), _dbDbi, &dbCur);
            CheckMdbxResult(rc);

            rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_FIRST);
            if (rc == MDBX_NOTFOUND)
                return env.Undefined();
            CheckMdbxResult(rc);

            Napi::Value result = Napi::Buffer<char>::Copy(env, (const char *)key.iov_base, key.iov_len);

            mdbx_cursor_close(dbCur);
            return result;
        } catch(...) {
            if (dbCur != NULL)
                mdbx_cursor_close(dbCur);
            throw;
        };

        return env.Undefined();
    });
}

Napi::Value CppDbi::LastKey(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);

    return wrapException(env, [&] () {
        MDBX_cursor *dbCur = NULL;
        MDBX_val key;

        int rc = MDBX_SUCCESS;
        try {
            rc = mdbx_cursor_open(_dbEnvPtr->GetTransaction(), _dbDbi, &dbCur);
            CheckMdbxResult(rc);

            rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_LAST);
            if (rc == MDBX_NOTFOUND)
                return env.Undefined();
            CheckMdbxResult(rc);

            Napi::Value result = Napi::Buffer<char>::Copy(env, (const char *)key.iov_base, key.iov_len);

            mdbx_cursor_close(dbCur);
            return result;
        } catch(...) {
            if (dbCur != NULL)
                mdbx_cursor_close(dbCur);
            throw;
        };

        return env.Undefined();
    });
}

Napi::Value CppDbi::NextKey(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);
    
    if (info[0].IsNull() || info[0].IsUndefined())
        return env.Undefined();

    ExtractBuffer(info[0], _keyBuffer);
    MDBX_val inKey = CreateMdbxVal(_keyBuffer);

    return wrapException(env, [&] () {
        MDBX_cursor *dbCur = NULL;
        MDBX_val key = inKey;

        int rc = MDBX_SUCCESS;
        try {
            rc = mdbx_cursor_open(_dbEnvPtr->GetTransaction(), _dbDbi, &dbCur);
            CheckMdbxResult(rc);

            rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_SET_RANGE);
            if (rc == MDBX_NOTFOUND)
                return env.Undefined();
            CheckMdbxResult(rc);

            const int cmpResult = mdbx_cmp(_dbEnvPtr->GetTransaction(), _dbDbi, &inKey, &key);
            if (cmpResult == 0) {
                rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_NEXT);
                if (rc == MDBX_NOTFOUND)
                    return env.Undefined();
                CheckMdbxResult(rc);
            };

            Napi::Value result = Napi::Buffer<char>::Copy(env, (const char *)key.iov_base, key.iov_len);

            mdbx_cursor_close(dbCur);
            return result;
        } catch(...) {
            if (dbCur != NULL)
                mdbx_cursor_close(dbCur);
            throw;
        };

        return env.Undefined();
    });
}

Napi::Value CppDbi::PrevKey(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    _check(env);
    
    if (info[0].IsNull() || info[0].IsUndefined())
        return env.Undefined();

    ExtractBuffer(info[0], _keyBuffer);
    MDBX_val inKey = CreateMdbxVal(_keyBuffer);

    return wrapException(env, [&] () {
        MDBX_cursor *dbCur = NULL;
        MDBX_val key = inKey;

        int rc = MDBX_SUCCESS;
        try {
            rc = mdbx_cursor_open(_dbEnvPtr->GetTransaction(), _dbDbi, &dbCur);
            CheckMdbxResult(rc);

            rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_SET_RANGE);
            if (rc == MDBX_NOTFOUND) {
                rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_LAST);
                if (rc == MDBX_NOTFOUND)
                    return env.Undefined();
            };
            CheckMdbxResult(rc);

            const int cmpResult = mdbx_cmp(_dbEnvPtr->GetTransaction(), _dbDbi, &inKey, &key);
            if (cmpResult <= 0) {
                rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_PREV);
                if (rc == MDBX_NOTFOUND)
                    return env.Undefined();
                CheckMdbxResult(rc);
            };

            Napi::Value result = Napi::Buffer<char>::Copy(env, (const char *)key.iov_base, key.iov_len);

            mdbx_cursor_close(dbCur);
            return result;
        } catch(...) {
            if (dbCur != NULL)
                mdbx_cursor_close(dbCur);
            throw;
        };

        return env.Undefined();
    });
}

Napi::Value CppDbi::LowerBoundKey(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info[0].IsNull() || info[0].IsUndefined())
        return FirstKey(info);

    _check(env);

    ExtractBuffer(info[0], _keyBuffer);
    MDBX_val inKey = CreateMdbxVal(_keyBuffer);

    return wrapException(env, [&] () {
        MDBX_cursor *dbCur = NULL;
        MDBX_val key = inKey;

        int rc = MDBX_SUCCESS;
        try {
            rc = mdbx_cursor_open(_dbEnvPtr->GetTransaction(), _dbDbi, &dbCur);
            CheckMdbxResult(rc);

            rc = mdbx_cursor_get(dbCur, &key, NULL, MDBX_SET_RANGE);
            if (rc == MDBX_NOTFOUND)
                return env.Undefined();
            CheckMdbxResult(rc);

            Napi::Value result = Napi::Buffer<char>::Copy(env, (const char *)key.iov_base, key.iov_len);

            mdbx_cursor_close(dbCur);
            return result;
        } catch(...) {
            if (dbCur != NULL)
                mdbx_cursor_close(dbCur);
            throw;
        };

        return env.Undefined();
    });
}

void CppDbi::_check(Napi::Env env) {
    if (!_dbEnvPtr || !_dbEnvPtr->IsOpened())
        throw Napi::Error::New(env, "Closed.");
}