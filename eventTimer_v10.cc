#include <string>
#include <node.h>	/* 必須 */
#include <uv.h>		/* 非同期コールバックの為 */
#include <thread>	/* for std::this_thread::sleep_for() */

using namespace v8;	/* 必須 */
using namespace node;

static Persistent<String> emit_symbol;
void _execute(uv_work_t* req);
void _complete(uv_work_t* req, int);

class MyObject : public ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object>& target)
	{
		Local<FunctionTemplate> clazz = FunctionTemplate::New(MyObject::New);
		clazz->SetClassName(String::NewSymbol("MyObject"));
		clazz->InstanceTemplate()->SetInternalFieldCount(1);
		clazz->PrototypeTemplate()->Set(
			String::NewSymbol("start"),
			FunctionTemplate::New(MyObject::Start)->GetFunction()
			);
		target->Set(String::NewSymbol("MyObject"), clazz->GetFunction());
	};
	void Emit(const std::string& msg)
	{
		HandleScope scope;

		Local<Value> emit_v = handle_->Get(emit_symbol);
		assert(emit_v->IsFunction());
		Local<Function> emit = emit_v.As<Function>();

		const size_t argc = 2;
		Handle<Value> argv[argc] = {
			String::New(msg.c_str()),
			String::New(name_.c_str())
		};

		TryCatch tc;
		emit->Call(this->handle_, argc, argv);
		if (tc.HasCaught()) {
			FatalException(tc);
		}
	}
	int delay = 0;
	bool stop = false;

private:
	MyObject(const std::string& name)
		: ObjectWrap(), name_(name) {};

	~MyObject() {};

	// JavaScript の世界で new されたら呼ばれる
	static v8::Handle<v8::Value> New(const v8::Arguments& args)
	{
		HandleScope scope;

		v8::String::Utf8Value name(args[0]);
		MyObject* obj = new MyObject(*name);
		obj->Wrap(args.This());

		return args.This();
	};

	// JavaScript の世界で MyObject.start したら呼ばれる
	static v8::Handle<v8::Value> Start(const v8::Arguments& args)
	{
		HandleScope scope;

		MyObject* obj = ObjectWrap::Unwrap<MyObject>(args.This());
		obj->delay = args[0]->Int32Value();
		uv_work_t *req = new uv_work_t;
		req->data = obj;
		uv_queue_work(uv_default_loop(), req, _execute, _complete);
		return scope.Close(Undefined());
	};


	// クラス毎に扱う変数とか
	const std::string name_;
};

void _execute(uv_work_t* req) {

	MyObject* data = static_cast<MyObject*>(req->data);
	std::this_thread::sleep_for(std::chrono::milliseconds(data->delay));
}
void _complete(uv_work_t* req, int) {

	MyObject* data = static_cast<MyObject*>(req->data);
	data->Emit("interval");
	if (data->stop){
		delete req;
	}
	else{
		uv_queue_work(uv_default_loop(), req, _execute, _complete);
	}
}



/* ここに外部から呼ばれる関数を書いていく */
/* 外部から呼ばれる名前と内部の関数名のひも付け */
void init(Handle<Object> target) {
	emit_symbol = NODE_PSYMBOL("emit");
	MyObject::Init(target);
}

/* モジュールがrequireされる時に呼ばれる */
NODE_MODULE(eventTimer_v10, init)