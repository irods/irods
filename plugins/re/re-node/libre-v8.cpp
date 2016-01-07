#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/libplatform/libplatform.h"
#include "include/v8.h"
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
#include <boost/any.hpp>
#include <irods/irods_re_plugin.hpp>
#include <boost/range/irange.hpp>

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  virtual void* Allocate(size_t length) {
    void* data = AllocateUninitialized(length);
    return data == NULL ? data : memset(data, 0, length);
  }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) { free(data); }
};

v8::Platform* platform2;
v8::Isolate* isolate;
ArrayBufferAllocator allocator;
v8::Isolate::CreateParams create_params;
v8::Persistent<v8::Context> context_handle;

template<typename T>
v8::Handle<T> v8_cast(const v8::Handle<v8::Value> &v) {
    return v8::Handle<T>::Cast(v);
}

v8::Handle<v8::Value> any_to_v8(boost::any &itr) {
  return v8::String::NewFromUtf8(isolate, irods::any_extract<std::string>(itr, [](){return std::string();}).c_str());
}

void v8_to_any_update(boost::any &itr, const v8::Handle<v8::Value> &obj) {
  irods::any_update(itr, std::string(*v8::String::Utf8Value(v8_cast<v8::String>(obj))) );
}

void do_app(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto args = new v8::Handle<v8::Value>[info.Length()+1];
  auto data = v8_cast<v8::Array>(info.Data());
  auto func = v8_cast<v8::Function>(data->Get(0));
  args[0] = data->Get(1);
  for(int i = 0; i<info.Length(); i++) {
      args[i+1] = info[i];
  }
  info.GetReturnValue().Set(func->Call(info.This(),info.Length()+1, args));
  delete[] args;
}

using wrapped_call_type = v8::Handle<v8::Function>;
using wrapped_call_object_type = v8::Handle<v8::Object>;
using wrapped_callback_type = v8::Handle<v8::External>;

template<typename T>
wrapped_call_type app( wrapped_call_type func,  T&& arg) {

  v8::Handle<v8::Array> arr = v8::Array::New(isolate, 2);
  arr->Set(0, func);
  arr->Set(1, arg);
  auto cb2 = v8::FunctionTemplate::New(isolate, do_app, arr);

  return cb2->GetFunction();
}

void cbSet(v8::Local<v8::String> name, v8::Local<v8::Value> v, const v8::PropertyCallbackInfo<v8::Value>& info) {
    info.GetIsolate()->ThrowException(v8::String::NewFromUtf8(isolate, "Cannot set property"));
}

void cbGet(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
    info.GetReturnValue().Set(app(v8_cast<v8::Function>(info.This()->GetInternalField(0)), name));
}

wrapped_call_object_type objectify(wrapped_call_type func) {
  auto obj_temp = v8::ObjectTemplate::New(isolate);
  obj_temp->SetInternalFieldCount(1);
  obj_temp->SetNamedPropertyHandler(cbGet, cbSet);
  auto obj = obj_temp->NewInstance();
  obj->SetInternalField(0, func);
  return obj;
}


wrapped_callback_type wrap_callback(irods::callback &cb) {
    return v8::External::New(isolate, &cb);
}


void call_cb_fn(const v8::FunctionCallbackInfo<v8::Value>& info){
  std::vector<std::string> ps0(info.Length() - 2);
  std::list<boost::any> ps;

  irods::callback *cb = static_cast<irods::callback*>(v8_cast<v8::External>(info[0])->Value());
  auto fn = std::string(*v8::String::Utf8Value(v8_cast<v8::String>(info[1])));

  irods::foreach2(ps0, boost::irange(2,info.Length()), [&](auto&& a, auto&& b) {
    ps.emplace_back(&a);
    v8_to_any_update(ps.back(), info[b]);
  });

  irods::error err = (*cb)(fn, irods::unpack(ps));

  if(!err.ok()) {
    info.GetIsolate()->ThrowException(v8::Integer::New(isolate, err.code()));
  } else {
    v8::Handle<v8::Array> arr = v8::Array::New(info.GetIsolate(),ps.size());
    irods::foreach2(ps, boost::irange(0,info.Length()-2), [&](auto&& a, auto&& b) {
        arr->Set(b,any_to_v8(a));
    });
    info.GetReturnValue().Set(arr);
  }

}

wrapped_call_type eval() {
  return v8::FunctionTemplate::New(isolate, call_cb_fn)->GetFunction();
}


v8::Local<v8::Object> get_global(){
    return v8::Local<v8::Context>::New(isolate, context_handle)->Global();
}

extern "C" {
  irods::error start(irods::default_re_ctx&) {

    v8::V8::InitializeICU();
    platform2 = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform2);
    v8::V8::Initialize();

    create_params.array_buffer_allocator = &allocator;
    isolate = v8::Isolate::New(create_params);
    isolate->Enter();

    std::ifstream ifs("/etc/irods/core.js");
    std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context = v8::Context::New(isolate);

    context_handle.Reset(isolate, context);

    context->Enter();

    v8::Script::Compile(v8::String::NewFromUtf8(isolate, code.c_str()))->Run();

    return SUCCESS();
  }
  irods::error stop(irods::default_re_ctx&) {
    {
      v8::HandleScope handle_scope(isolate);

      v8::Local<v8::Context>::New(isolate, context_handle)->Exit();

    }
    context_handle.Reset();
    isolate->Exit();
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete platform2;
    return SUCCESS();
  }
  irods::error rule_exists(irods::default_re_ctx&, std::string fn, bool& b) {

    v8::HandleScope handle_scope(isolate);
    b = get_global()->Has(v8::String::NewFromUtf8(isolate, fn.c_str()));
    return SUCCESS();
  }
  irods::error exec_rule(irods::default_re_ctx&, std::string fn, std::list<boost::any>& ps, irods::callback cb) {

    v8::HandleScope handle_scope(isolate);
    v8::TryCatch trycatch(isolate);

    auto global = get_global();
    auto func = v8_cast<v8::Function>(global->Get(v8::String::NewFromUtf8(isolate, fn.c_str())));

    std::vector<v8::Handle<v8::Value> > args;

    irods::foreach2 (ps, boost::irange(0, static_cast<int>(ps.size())), [&](auto&& a, auto&& b) {
        args.push_back(any_to_v8(a));
    });

    args.push_back(objectify(app(eval(), wrap_callback(cb))));

    auto js_res = func->Call(global, args.size(), &args[0]);

    if(trycatch.HasCaught()) {
        auto exception = trycatch.Exception();
        v8::String::Utf8Value exception_str(exception);
        rodsLog(LOG_ERROR, *exception_str);
        return ERROR(-1, *exception_str);
    } else {
      if (js_res->IsArray()) {
          auto arr = v8_cast<v8::Array>(js_res);

          irods::foreach2 (ps, boost::irange(0, static_cast<int>(arr->Length())), [&](auto&&a ,auto&& b) {
              v8_to_any_update(a, arr->Get(b));
          });

      }

      return SUCCESS();


    }

  }
  irods::pluggable_rule_engine<irods::default_re_ctx>* plugin_factory(const std::string& _inst_name,
                                                                    const std::string& _context) {
    return new irods::pluggable_rule_engine<irods::default_re_ctx>( _inst_name , _context);
  }

}
