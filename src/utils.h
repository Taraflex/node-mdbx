#pragma once

#include <string>
#include <vector>
#include <iterator>

#include <napi.h>
#include "mdbx.h"

typedef std::vector<char> buffer_t;

static void ExtractBuffer(const Napi::Value &from, buffer_t &to) {
    if (from.IsString()) {
        std::string s = from.As<Napi::String>().Utf8Value();
        to.clear();
        to.reserve(s.size());
        std::copy(s.begin(), s.end(), std::back_inserter(to));
    } else if (from.IsBuffer()) {
        auto b = from.As<Napi::Buffer<char>>();
        size_t size = b.Length();
        const char *data = b.Data();
        to.clear();
        to.reserve(size);
        std::copy(data, data + size, std::back_inserter(to));
    } else {
        const Napi::Env env = from.Env();
        throw Napi::Error::New(env, "Bad input. Should be a string or a buffer.");
    };
}

static MDBX_val CreateMdbxVal(const buffer_t &buffer) {
    MDBX_val val;
    val.iov_base = (void *) buffer.data();
    val.iov_len = buffer.size();
    return val;
}


// Throwing other than Napi::Error from C++ code to calling JS code leads to program termination
template<typename F>
static auto wrapException(Napi::Env env, const F &f) {
    try {
        return f();
    } catch(Napi::Error &) {
        throw;
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    };
}