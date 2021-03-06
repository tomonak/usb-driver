#include "usb_driver.h"
#include "nan.h"

using namespace v8;

namespace {
    static Local<Object>
    USBDrive_to_Object(struct usb_driver::USBDrive *usb_drive)
    {
	Local<Object> obj = NanNew<Object>();

#define OBJ_ATTR(name, val) \
	do { \
	    Local<String> _name = NanNew<v8::String>(name); \
	    if (val.size() > 0) { \
		obj->Set(_name, NanNew<v8::String>(val)); \
	    } \
	    else { \
		obj->Set(_name, NanNull()); \
	    } \
	} \
	while (0)

	OBJ_ATTR("id", usb_drive->uid);
	OBJ_ATTR("productCode", usb_drive->product_id);
	OBJ_ATTR("vendorCode", usb_drive->vendor_id);
	OBJ_ATTR("product", usb_drive->product_str);
	OBJ_ATTR("serialNumber", usb_drive->serial_str);
	OBJ_ATTR("manufacturer", usb_drive->vendor_str);
	OBJ_ATTR("mount", usb_drive->mount);

#undef OBJ_ATTR
	return obj;
    }

    NAN_METHOD(Unmount)
    {
	NanScope();

	String::Utf8Value utf8_string(Local<String>::Cast(args[0]));
	if (usb_driver::Unmount(*utf8_string)) {
	    NanReturnValue(NanTrue());
	}
	else {
	    NanReturnValue(NanFalse());
	}
    }

    class NodeUSBWatcher : public usb_driver::USBWatcher {
	Persistent<Object> js_watcher;

	public:
	NodeUSBWatcher(Local<Object> obj) {
	    NanAssignPersistent(js_watcher, obj);
	}

	virtual ~NodeUSBWatcher() {
	    NanDisposePersistent(js_watcher);
	}

	virtual void
	attached(struct usb_driver::USBDrive *usb_info) {
	    emit("attach", usb_info);
	}

	virtual void
	detached(struct usb_driver::USBDrive *usb_info) {
	    emit("detach", usb_info);
	}

	virtual void
	mount(struct usb_driver::USBDrive *usb_info) {
	    emit("mount", usb_info);
	}

	virtual void
	unmount(struct usb_driver::USBDrive *usb_info) {
	    emit("unmount", usb_info);
	}

	private:
	void
	emit(const char *msg, struct usb_driver::USBDrive *usb_info) {
	    assert(usb_info != NULL);

	    Local<Object> rcv = NanNew<Object>(js_watcher);
	    Handle<Value> argv[1] = { USBDrive_to_Object(usb_info) };
	    NanMakeCallback(rcv, NanNew<v8::String>(msg), 1, argv);
	}
    };

    NAN_METHOD(RegisterWatcher)
    {
	NanScope();
	Local<Object> js_watcher(Local<Object>::Cast(args[0]));
	NodeUSBWatcher *watcher = new NodeUSBWatcher(js_watcher);
	usb_driver::RegisterWatcher(watcher);
	NanReturnNull();
    }

    NAN_METHOD(WaitForEvents)
    {
	NanScope();
	usb_driver::WaitForEvents();
	NanReturnNull();
    }

    NAN_METHOD(GetDevice)
    {
	NanScope();

	String::Utf8Value utf8_string(Local<String>::Cast(args[0]));
	struct usb_driver::USBDrive *usb_drive =
	    usb_driver::GetDevice(*utf8_string);
	if (usb_drive == NULL) {
	    NanReturnNull();
	}
	else {
	    NanReturnValue(USBDrive_to_Object(usb_drive));
	}
    }

    NAN_METHOD(GetDevices)
    {
	NanScope();

	std::vector<struct usb_driver::USBDrive *> devices =
	    usb_driver::GetDevices();
	Handle<Array> ary = NanNew<Array>(devices.size());
	for (unsigned int i = 0; i < devices.size(); i++) {
	    ary->Set((int)i, USBDrive_to_Object(devices[i]));
	}
	NanReturnValue(ary);
    }

    void
    Init(Handle<Object> exports)
    {
	NODE_SET_METHOD(exports, "unmount", Unmount);
	NODE_SET_METHOD(exports, "getDevice", GetDevice);
	NODE_SET_METHOD(exports, "getDevices", GetDevices);
	NODE_SET_METHOD(exports, "registerWatcher", RegisterWatcher);
	NODE_SET_METHOD(exports, "waitForEvents", WaitForEvents);
    }
}  // namespace

NODE_MODULE(usb_driver, Init)
