/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/binder_ibinder.h>
#include <android/binder_ibinder_platform.h>
#include <android/binder_stability.h>
#include <android/binder_status.h>
#include <binder/Functional.h>
#include <binder/IPCThreadState.h>
#include <binder/IResultReceiver.h>
#include <binder/Trace.h>
#if __has_include(<private/android_filesystem_config.h>)
#include <private/android_filesystem_config.h>
#endif

#include "../BuildFlags.h"
#include "ibinder_internal.h"
#include "parcel_internal.h"
#include "status_internal.h"

using DeathRecipient = ::android::IBinder::DeathRecipient;

using ::android::IBinder;
using ::android::IResultReceiver;
using ::android::Parcel;
using ::android::sp;
using ::android::status_t;
using ::android::statusToString;
using ::android::String16;
using ::android::String8;
using ::android::wp;
using ::android::binder::impl::make_scope_guard;
using ::android::binder::impl::scope_guard;
using ::android::binder::os::get_trace_enabled_tags;
using ::android::binder::os::trace_begin;
using ::android::binder::os::trace_end;

// transaction codes for getInterfaceHash and getInterfaceVersion are defined
// in file : system/tools/aidl/aidl.cpp
static constexpr int kGetInterfaceVersionId = 0x00fffffe;
static const char* kInterfaceVersion = "getInterfaceVersion";
static constexpr int kGetInterfaceHashId = 0x00fffffd;
static const char* kInterfaceHash = "getInterfaceHash";
static const char* kNdkTrace = "AIDL::ndk::";
static const char* kServerTrace = "::server";
static const char* kClientTrace = "::client";
static const char* kSeparator = "::";
static const char* kUnknownCode = "Unknown_Transaction_Code:";

namespace ABBinderTag {

static const void* kId = "ABBinder";
static void* kValue = static_cast<void*>(new bool{true});
void clean(const void* /*id*/, void* /*obj*/, void* /*cookie*/) {
    /* do nothing */
}

static void attach(const sp<IBinder>& binder) {
    auto alreadyAttached = binder->attachObject(kId, kValue, nullptr /*cookie*/, clean);
    LOG_ALWAYS_FATAL_IF(alreadyAttached != nullptr, "can only attach once");
}
static bool has(const sp<IBinder>& binder) {
    return binder != nullptr && binder->findObject(kId) == kValue;
}

}  // namespace ABBinderTag

namespace ABpBinderTag {

static const void* kId = "ABpBinder";
struct Value {
    wp<ABpBinder> binder;
};
void clean(const void* id, void* obj, void* cookie) {
    // be weary of leaks!
    // ALOGI("Deleting an ABpBinder");

    LOG_ALWAYS_FATAL_IF(id != kId, "%p %p %p", id, obj, cookie);

    delete static_cast<Value*>(obj);
}

}  // namespace ABpBinderTag

AIBinder::AIBinder(const AIBinder_Class* clazz) : mClazz(clazz) {}
AIBinder::~AIBinder() {}

// b/175635923 libcxx causes "implicit-conversion" with a string with invalid char
static std::string SanitizeString(const String16& str) {
    std::string sanitized{String8(str)};
    for (auto& c : sanitized) {
        if (!isprint(c)) {
            c = '?';
        }
    }
    return sanitized;
}

const std::string getMethodName(const AIBinder_Class* clazz, transaction_code_t code) {
    // TODO(b/150155678) - Move getInterfaceHash and getInterfaceVersion to libbinder and remove
    // hardcoded cases.
    if (code <= clazz->getTransactionCodeToFunctionLength() && code >= FIRST_CALL_TRANSACTION) {
        // Codes have FIRST_CALL_TRANSACTION as added offset. Subtract to access function name
        return clazz->getFunctionName(code);
    } else if (code == kGetInterfaceVersionId) {
        return kInterfaceVersion;
    } else if (code == kGetInterfaceHashId) {
        return kInterfaceHash;
    }
    return kUnknownCode + std::to_string(code);
}

const std::string getTraceSectionName(const AIBinder_Class* clazz, transaction_code_t code,
                                      bool isServer) {
    if (clazz == nullptr) {
        ALOGE("class associated with binder is null. Class is needed to add trace with interface "
              "name and function name");
        return kNdkTrace;
    }

    const std::string descriptor = clazz->getInterfaceDescriptorUtf8();
    const std::string methodName = getMethodName(clazz, code);

    size_t traceSize =
            strlen(kNdkTrace) + descriptor.size() + strlen(kSeparator) + methodName.size();
    traceSize += isServer ? strlen(kServerTrace) : strlen(kClientTrace);

    std::string trace;
    // reserve to avoid repeated allocations
    trace.reserve(traceSize);

    trace += kNdkTrace;
    trace += clazz->getInterfaceDescriptorUtf8();
    trace += kSeparator;
    trace += methodName;
    trace += isServer ? kServerTrace : kClientTrace;

    LOG_ALWAYS_FATAL_IF(trace.size() != traceSize, "Trace size mismatch. Expected %zu, got %zu",
                        traceSize, trace.size());

    return trace;
}

bool AIBinder::associateClass(const AIBinder_Class* clazz) {
    if (clazz == nullptr) return false;

    // If mClazz is non-null, this must have been called and cached
    // already. So, we can safely call this first. Due to the implementation
    // of getInterfaceDescriptor (at time of writing), two simultaneous calls
    // may lead to extra binder transactions, but this is expected to be
    // exceedingly rare. Once we have a binder, when we get it again later,
    // we won't make another binder transaction here.
    const String16& descriptor = getBinder()->getInterfaceDescriptor();
    const String16& newDescriptor = clazz->getInterfaceDescriptor();

    std::lock_guard<std::mutex> lock(mClazzMutex);
    if (mClazz == clazz) return true;

    // If this is an ABpBinder, the first class object becomes the canonical one. The implication
    // of this is that no API can require a proxy information to get information on how to behave.
    // from the class itself - which should only store the interface descriptor. The functionality
    // should be implemented by adding AIBinder_* APIs to set values on binders themselves, by
    // setting things on AIBinder_Class which get transferred along with the binder, so that they
    // can be read along with the BpBinder, or by modifying APIs directly (e.g. an option in
    // onTransact).
    //
    // While this check is required to support linkernamespaces, one downside of it is that
    // you may parcel code to communicate between things in the same process. However, comms
    // between linkernamespaces like this already happen for cross-language calls like Java<->C++
    // or Rust<->Java, and there are good stability guarantees here. This interacts with
    // binder Stability checks exactly like any other in-process call. The stability is known
    // to the IBinder object, so that it doesn't matter if a class object comes from
    // a different stability level.
    if (mClazz != nullptr && !asABpBinder()) {
        const String16& currentDescriptor = mClazz->getInterfaceDescriptor();
        if (newDescriptor == currentDescriptor) {
            ALOGE("Class descriptors '%s' match during associateClass, but they are different class"
                  " objects (%p vs %p). Class descriptor collision?",
                  String8(currentDescriptor).c_str(), clazz, mClazz);
        } else {
            ALOGE("%s: Class cannot be associated on object which already has a class. "
                  "Trying to associate to '%s' but already set to '%s'.",
                  __func__, String8(newDescriptor).c_str(), String8(currentDescriptor).c_str());
        }

        // always a failure because we know mClazz != clazz
        return false;
    }

    // This will always be an O(n) comparison, but it's expected to be extremely rare.
    // since it's an error condition. Do the comparison after we take the lock and
    // check the pointer equality fast path. By always taking the lock, it's also
    // more flake-proof. However, the check is not dependent on the lock.
    if (descriptor != newDescriptor && !(asABpBinder() && asABpBinder()->isServiceFuzzing())) {
        if (getBinder()->isBinderAlive()) {
            ALOGE("%s: Expecting binder to have class '%s' but descriptor is actually '%s'.",
                  __func__, String8(newDescriptor).c_str(), SanitizeString(descriptor).c_str());
        } else {
            // b/155793159
            ALOGE("%s: Cannot associate class '%s' to dead binder with cached descriptor '%s'.",
                  __func__, String8(newDescriptor).c_str(), SanitizeString(descriptor).c_str());
        }
        return false;
    }

    // A local binder being set for the first time OR
    // ignoring a proxy binder which is set multiple time, by considering the first
    // associated class as the canonical one.
    if (mClazz == nullptr) {
        mClazz = clazz;
    }

    return true;
}

ABBinder::ABBinder(const AIBinder_Class* clazz, void* userData)
    : AIBinder(clazz), BBinder(), mUserData(userData) {
    LOG_ALWAYS_FATAL_IF(clazz == nullptr, "clazz == nullptr");
}
ABBinder::~ABBinder() {
    getClass()->onDestroy(mUserData);
}

const String16& ABBinder::getInterfaceDescriptor() const {
    return getClass()->getInterfaceDescriptor();
}

status_t ABBinder::dump(int fd, const ::android::Vector<String16>& args) {
    AIBinder_onDump onDump = getClass()->onDump;

    if (onDump == nullptr) {
        return STATUS_OK;
    }

    // technically UINT32_MAX would be okay here, but INT32_MAX is expected since this may be
    // null in Java
    if (args.size() > INT32_MAX) {
        ALOGE("ABBinder::dump received too many arguments: %zu", args.size());
        return STATUS_BAD_VALUE;
    }

    std::vector<String8> utf8Args;  // owns memory of utf8s
    utf8Args.reserve(args.size());
    std::vector<const char*> utf8Pointers;  // what can be passed over NDK API
    utf8Pointers.reserve(args.size());

    for (size_t i = 0; i < args.size(); i++) {
        utf8Args.push_back(String8(args[i]));
        utf8Pointers.push_back(utf8Args[i].c_str());
    }

    return onDump(this, fd, utf8Pointers.data(), utf8Pointers.size());
}

status_t ABBinder::onTransact(transaction_code_t code, const Parcel& data, Parcel* reply,
                              binder_flags_t flags) {
    std::string sectionName;
    bool tracingEnabled = get_trace_enabled_tags() & ATRACE_TAG_AIDL;
    if (tracingEnabled) {
        sectionName = getTraceSectionName(getClass(), code, true /*isServer*/);
        trace_begin(ATRACE_TAG_AIDL, sectionName.c_str());
    }

    scope_guard guard = make_scope_guard([&]() {
        if (tracingEnabled) trace_end(ATRACE_TAG_AIDL);
    });

    if (isUserCommand(code)) {
        if (getClass()->writeHeader && !data.checkInterface(this)) {
            return STATUS_BAD_TYPE;
        }

        const AParcel in = AParcel::readOnly(this, &data);
        AParcel out = AParcel(this, reply, false /*owns*/);

        binder_status_t status = getClass()->onTransact(this, code, &in, &out);
        return PruneStatusT(status);
    } else if (code == SHELL_COMMAND_TRANSACTION && getClass()->handleShellCommand != nullptr) {
        if constexpr (!android::kEnableKernelIpc) {
            // Non-IPC builds do not have getCallingUid(),
            // so we have no way of authenticating the caller
            return STATUS_PERMISSION_DENIED;
        }

        int in = data.readFileDescriptor();
        int out = data.readFileDescriptor();
        int err = data.readFileDescriptor();

        int argc = data.readInt32();
        std::vector<String8> utf8Args;          // owns memory of utf8s
        std::vector<const char*> utf8Pointers;  // what can be passed over NDK API
        for (int i = 0; i < argc && data.dataAvail() > 0; i++) {
            utf8Args.push_back(String8(data.readString16()));
            utf8Pointers.push_back(utf8Args[i].c_str());
        }

        data.readStrongBinder();  // skip over the IShellCallback
        sp<IResultReceiver> resultReceiver = IResultReceiver::asInterface(data.readStrongBinder());

        // Shell commands should only be callable by ADB.
        uid_t uid = AIBinder_getCallingUid();
        if (uid != 0 /* root */
#ifdef AID_SHELL
            && uid != AID_SHELL
#endif
        ) {
            if (resultReceiver != nullptr) {
                resultReceiver->send(-1);
            }
            return STATUS_PERMISSION_DENIED;
        }

        // Check that the file descriptors are valid.
        if (in == STATUS_BAD_TYPE || out == STATUS_BAD_TYPE || err == STATUS_BAD_TYPE) {
            if (resultReceiver != nullptr) {
                resultReceiver->send(-1);
            }
            return STATUS_BAD_VALUE;
        }

        binder_status_t status = getClass()->handleShellCommand(
                this, in, out, err, utf8Pointers.data(), utf8Pointers.size());
        if (resultReceiver != nullptr) {
            resultReceiver->send(status);
        }
        return status;
    } else {
        return BBinder::onTransact(code, data, reply, flags);
    }
}

void ABBinder::addDeathRecipient(const ::android::sp<AIBinder_DeathRecipient>& /* recipient */,
                                 void* /* cookie */) {
    LOG_ALWAYS_FATAL("Should not reach this. Can't linkToDeath local binders.");
}

ABpBinder::ABpBinder(const ::android::sp<::android::IBinder>& binder)
    : AIBinder(nullptr /*clazz*/), mRemote(binder) {
    LOG_ALWAYS_FATAL_IF(binder == nullptr, "binder == nullptr");
}

ABpBinder::~ABpBinder() {
    for (auto& recip : mDeathRecipients) {
        sp<AIBinder_DeathRecipient> strongRecip = recip.recipient.promote();
        if (strongRecip) {
            strongRecip->pruneThisTransferEntry(getBinder(), recip.cookie);
        }
    }
}

sp<AIBinder> ABpBinder::lookupOrCreateFromBinder(const ::android::sp<::android::IBinder>& binder) {
    if (binder == nullptr) {
        return nullptr;
    }
    if (ABBinderTag::has(binder)) {
        return static_cast<ABBinder*>(binder.get());
    }

    // The following code ensures that for a given binder object (remote or local), if it is not an
    // ABBinder then at most one ABpBinder object exists in a given process representing it.

    auto* value = static_cast<ABpBinderTag::Value*>(binder->findObject(ABpBinderTag::kId));
    if (value == nullptr) {
        value = new ABpBinderTag::Value;
        auto oldValue = static_cast<ABpBinderTag::Value*>(
                binder->attachObject(ABpBinderTag::kId, static_cast<void*>(value),
                                     nullptr /*cookie*/, ABpBinderTag::clean));

        // allocated by another thread
        if (oldValue) {
            delete value;
            value = oldValue;
        }
    }

    sp<ABpBinder> ret;
    binder->withLock([&]() {
        ret = value->binder.promote();
        if (ret == nullptr) {
            ret = sp<ABpBinder>::make(binder);
            value->binder = ret;
        }
    });

    return ret;
}

void ABpBinder::addDeathRecipient(const ::android::sp<AIBinder_DeathRecipient>& recipient,
                                  void* cookie) {
    std::lock_guard<std::mutex> l(mDeathRecipientsMutex);
    mDeathRecipients.emplace_back(recipient, cookie);
}

struct AIBinder_Weak {
    wp<AIBinder> binder;
};
AIBinder_Weak* AIBinder_Weak_new(AIBinder* binder) {
    if (binder == nullptr) {
        return nullptr;
    }

    return new AIBinder_Weak{wp<AIBinder>(binder)};
}
void AIBinder_Weak_delete(AIBinder_Weak* weakBinder) {
    delete weakBinder;
}
AIBinder* AIBinder_Weak_promote(AIBinder_Weak* weakBinder) {
    if (weakBinder == nullptr) {
        return nullptr;
    }

    sp<AIBinder> binder = weakBinder->binder.promote();
    AIBinder_incStrong(binder.get());
    return binder.get();
}

AIBinder_Weak* AIBinder_Weak_clone(const AIBinder_Weak* weak) {
    if (weak == nullptr) {
        return nullptr;
    }

    return new AIBinder_Weak{weak->binder};
}

bool AIBinder_lt(const AIBinder* lhs, const AIBinder* rhs) {
    if (lhs == nullptr || rhs == nullptr) return lhs < rhs;

    return const_cast<AIBinder*>(lhs)->getBinder() < const_cast<AIBinder*>(rhs)->getBinder();
}

bool AIBinder_Weak_lt(const AIBinder_Weak* lhs, const AIBinder_Weak* rhs) {
    if (lhs == nullptr || rhs == nullptr) return lhs < rhs;

    return lhs->binder < rhs->binder;
}

// WARNING: When multiple classes exist with the same interface descriptor in different
// linkernamespaces, the first one to be associated with mClazz becomes the canonical one
// and the only requirement on this is that the interface descriptors match. If this
// is an ABpBinder, no other state can be referenced from mClazz.
AIBinder_Class::AIBinder_Class(const char* interfaceDescriptor, AIBinder_Class_onCreate onCreate,
                               AIBinder_Class_onDestroy onDestroy,
                               AIBinder_Class_onTransact onTransact)
    : onCreate(onCreate),
      onDestroy(onDestroy),
      onTransact(onTransact),
      mInterfaceDescriptor(interfaceDescriptor),
      mWideInterfaceDescriptor(interfaceDescriptor) {}

bool AIBinder_Class::setTransactionCodeMap(const char** transactionCodeMap, size_t length) {
    if (mTransactionCodeToFunction != nullptr) {
        ALOGE("mTransactionCodeToFunction is already set!");
        return false;
    }
    mTransactionCodeToFunction = transactionCodeMap;
    mTransactionCodeToFunctionLength = length;
    return true;
}

const char* AIBinder_Class::getFunctionName(transaction_code_t code) const {
    if (mTransactionCodeToFunction == nullptr) {
        ALOGE("mTransactionCodeToFunction is not set!");
        return nullptr;
    }

    if (code < FIRST_CALL_TRANSACTION ||
        code - FIRST_CALL_TRANSACTION >= mTransactionCodeToFunctionLength) {
        ALOGE("Function name for requested code not found!");
        return nullptr;
    }

    return mTransactionCodeToFunction[code - FIRST_CALL_TRANSACTION];
}

AIBinder_Class* AIBinder_Class_define(const char* interfaceDescriptor,
                                      AIBinder_Class_onCreate onCreate,
                                      AIBinder_Class_onDestroy onDestroy,
                                      AIBinder_Class_onTransact onTransact) {
    if (interfaceDescriptor == nullptr || onCreate == nullptr || onDestroy == nullptr ||
        onTransact == nullptr) {
        return nullptr;
    }

    return new AIBinder_Class(interfaceDescriptor, onCreate, onDestroy, onTransact);
}

void AIBinder_Class_setOnDump(AIBinder_Class* clazz, AIBinder_onDump onDump) {
    LOG_ALWAYS_FATAL_IF(clazz == nullptr, "setOnDump requires non-null clazz");

    // this is required to be called before instances are instantiated
    clazz->onDump = onDump;
}

void AIBinder_Class_setTransactionCodeToFunctionNameMap(AIBinder_Class* clazz,
                                                        const char** transactionCodeToFunction,
                                                        size_t length) {
    LOG_ALWAYS_FATAL_IF(clazz == nullptr || transactionCodeToFunction == nullptr,
                        "Valid clazz and transactionCodeToFunction are needed to set code to "
                        "function mapping.");
    LOG_ALWAYS_FATAL_IF(!clazz->setTransactionCodeMap(transactionCodeToFunction, length),
                        "Failed to set transactionCodeToFunction to clazz! Is "
                        "transactionCodeToFunction already set?");
}

const char* AIBinder_Class_getFunctionName(AIBinder_Class* clazz, transaction_code_t code) {
    LOG_ALWAYS_FATAL_IF(
            clazz == nullptr,
            "Valid clazz is needed to get function name for requested transaction code");
    return clazz->getFunctionName(code);
}

void AIBinder_Class_disableInterfaceTokenHeader(AIBinder_Class* clazz) {
    LOG_ALWAYS_FATAL_IF(clazz == nullptr, "disableInterfaceTokenHeader requires non-null clazz");

    clazz->writeHeader = false;
}

void AIBinder_Class_setHandleShellCommand(AIBinder_Class* clazz,
                                          AIBinder_handleShellCommand handleShellCommand) {
    LOG_ALWAYS_FATAL_IF(clazz == nullptr, "setHandleShellCommand requires non-null clazz");

    clazz->handleShellCommand = handleShellCommand;
}

const char* AIBinder_Class_getDescriptor(const AIBinder_Class* clazz) {
    LOG_ALWAYS_FATAL_IF(clazz == nullptr, "getDescriptor requires non-null clazz");

    return clazz->getInterfaceDescriptorUtf8();
}

AIBinder_DeathRecipient::TransferDeathRecipient::~TransferDeathRecipient() {
    if (mOnUnlinked != nullptr) {
        mOnUnlinked(mCookie);
    }
}

void AIBinder_DeathRecipient::TransferDeathRecipient::binderDied(const wp<IBinder>& who) {
    LOG_ALWAYS_FATAL_IF(who != mWho, "%p (%p) vs %p (%p)", who.unsafe_get(), who.get_refs(),
                        mWho.unsafe_get(), mWho.get_refs());

    mOnDied(mCookie);

    sp<AIBinder_DeathRecipient> recipient = mParentRecipient.promote();
    sp<IBinder> strongWho = who.promote();

    // otherwise this will be cleaned up later with pruneDeadTransferEntriesLocked
    if (recipient != nullptr && strongWho != nullptr) {
        status_t result = recipient->unlinkToDeath(strongWho, mCookie);
        if (result != ::android::DEAD_OBJECT) {
            ALOGW("Unlinking to dead binder resulted in: %d", result);
        }
    }

    mWho = nullptr;
}

AIBinder_DeathRecipient::AIBinder_DeathRecipient(AIBinder_DeathRecipient_onBinderDied onDied)
    : mOnDied(onDied), mOnUnlinked(nullptr) {
    LOG_ALWAYS_FATAL_IF(onDied == nullptr, "onDied == nullptr");
}

void AIBinder_DeathRecipient::pruneThisTransferEntry(const sp<IBinder>& who, void* cookie) {
    std::lock_guard<std::mutex> l(mDeathRecipientsMutex);
    mDeathRecipients.erase(std::remove_if(mDeathRecipients.begin(), mDeathRecipients.end(),
                                          [&](const sp<TransferDeathRecipient>& tdr) {
                                              auto tdrWho = tdr->getWho();
                                              return tdrWho != nullptr && tdrWho.promote() == who &&
                                                     cookie == tdr->getCookie();
                                          }),
                           mDeathRecipients.end());
}

void AIBinder_DeathRecipient::pruneDeadTransferEntriesLocked() {
    mDeathRecipients.erase(std::remove_if(mDeathRecipients.begin(), mDeathRecipients.end(),
                                          [](const sp<TransferDeathRecipient>& tdr) {
                                              return tdr->getWho() == nullptr;
                                          }),
                           mDeathRecipients.end());
}

binder_status_t AIBinder_DeathRecipient::linkToDeath(const sp<IBinder>& binder, void* cookie) {
    LOG_ALWAYS_FATAL_IF(binder == nullptr, "binder == nullptr");

    std::lock_guard<std::mutex> l(mDeathRecipientsMutex);

    if (mOnUnlinked && cookie &&
        std::find_if(mDeathRecipients.begin(), mDeathRecipients.end(),
                     [&cookie](android::sp<TransferDeathRecipient> recipient) {
                         return recipient->getCookie() == cookie;
                     }) != mDeathRecipients.end()) {
        ALOGE("Attempting to AIBinder_linkToDeath with the same cookie with an onUnlink callback. "
              "This will cause the onUnlinked callback to be called multiple times with the same "
              "cookie, which is usually not intended.");
    }
    if (!mOnUnlinked && cookie) {
        ALOGW("AIBinder_linkToDeath is being called with a non-null cookie and no onUnlink "
              "callback set. This might not be intended. AIBinder_DeathRecipient_setOnUnlinked "
              "should be called first.");
    }

    sp<TransferDeathRecipient> recipient =
            new TransferDeathRecipient(binder, cookie, this, mOnDied, mOnUnlinked);

    status_t status = binder->linkToDeath(recipient, cookie, 0 /*flags*/);
    if (status != STATUS_OK) {
        // When we failed to link, the destructor of TransferDeathRecipient runs here, which
        // ensures that mOnUnlinked is called before we return with an error from this method.
        return PruneStatusT(status);
    }

    mDeathRecipients.push_back(recipient);

    pruneDeadTransferEntriesLocked();
    return STATUS_OK;
}

binder_status_t AIBinder_DeathRecipient::unlinkToDeath(const sp<IBinder>& binder, void* cookie) {
    LOG_ALWAYS_FATAL_IF(binder == nullptr, "binder == nullptr");

    std::lock_guard<std::mutex> l(mDeathRecipientsMutex);

    for (auto it = mDeathRecipients.rbegin(); it != mDeathRecipients.rend(); ++it) {
        sp<TransferDeathRecipient> recipient = *it;

        if (recipient->getCookie() == cookie && recipient->getWho() == binder) {
            mDeathRecipients.erase(it.base() - 1);

            status_t status = binder->unlinkToDeath(recipient, cookie, 0 /*flags*/);
            if (status != ::android::OK) {
                ALOGE("%s: removed reference to death recipient but unlink failed: %s", __func__,
                      statusToString(status).c_str());
            }
            return PruneStatusT(status);
        }
    }

    return STATUS_NAME_NOT_FOUND;
}

void AIBinder_DeathRecipient::setOnUnlinked(AIBinder_DeathRecipient_onBinderUnlinked onUnlinked) {
    mOnUnlinked = onUnlinked;
}

// start of C-API methods

AIBinder* AIBinder_new(const AIBinder_Class* clazz, void* args) {
    if (clazz == nullptr) {
        ALOGE("%s: Must provide class to construct local binder.", __func__);
        return nullptr;
    }

    void* userData = clazz->onCreate(args);

    sp<AIBinder> ret = new ABBinder(clazz, userData);
    ABBinderTag::attach(ret->getBinder());

    AIBinder_incStrong(ret.get());
    return ret.get();
}

bool AIBinder_isRemote(const AIBinder* binder) {
    if (binder == nullptr) {
        return false;
    }

    return binder->isRemote();
}

bool AIBinder_isAlive(const AIBinder* binder) {
    if (binder == nullptr) {
        return false;
    }

    return const_cast<AIBinder*>(binder)->getBinder()->isBinderAlive();
}

binder_status_t AIBinder_ping(AIBinder* binder) {
    if (binder == nullptr) {
        return STATUS_UNEXPECTED_NULL;
    }

    return PruneStatusT(binder->getBinder()->pingBinder());
}

binder_status_t AIBinder_dump(AIBinder* binder, int fd, const char** args, uint32_t numArgs) {
    if (binder == nullptr) {
        return STATUS_UNEXPECTED_NULL;
    }

    ABBinder* bBinder = binder->asABBinder();
    if (bBinder != nullptr) {
        AIBinder_onDump onDump = binder->getClass()->onDump;
        if (onDump == nullptr) {
            return STATUS_OK;
        }
        return PruneStatusT(onDump(bBinder, fd, args, numArgs));
    }

    ::android::Vector<String16> utf16Args;
    utf16Args.setCapacity(numArgs);
    for (uint32_t i = 0; i < numArgs; i++) {
        utf16Args.push(String16(String8(args[i])));
    }

    status_t status = binder->getBinder()->dump(fd, utf16Args);
    return PruneStatusT(status);
}

binder_status_t AIBinder_linkToDeath(AIBinder* binder, AIBinder_DeathRecipient* recipient,
                                     void* cookie) {
    if (binder == nullptr || recipient == nullptr) {
        ALOGE("%s: Must provide binder (%p) and recipient (%p)", __func__, binder, recipient);
        return STATUS_UNEXPECTED_NULL;
    }

    binder_status_t ret = recipient->linkToDeath(binder->getBinder(), cookie);
    if (ret == STATUS_OK) {
        binder->addDeathRecipient(recipient, cookie);
    }
    return ret;
}

binder_status_t AIBinder_unlinkToDeath(AIBinder* binder, AIBinder_DeathRecipient* recipient,
                                       void* cookie) {
    if (binder == nullptr || recipient == nullptr) {
        ALOGE("%s: Must provide binder (%p) and recipient (%p)", __func__, binder, recipient);
        return STATUS_UNEXPECTED_NULL;
    }

    // returns binder_status_t
    return recipient->unlinkToDeath(binder->getBinder(), cookie);
}

#ifdef BINDER_WITH_KERNEL_IPC
uid_t AIBinder_getCallingUid() {
    return ::android::IPCThreadState::self()->getCallingUid();
}

pid_t AIBinder_getCallingPid() {
    return ::android::IPCThreadState::self()->getCallingPid();
}

bool AIBinder_isHandlingTransaction() {
    return ::android::IPCThreadState::self()->getServingStackPointer() != nullptr;
}
#endif

void AIBinder_incStrong(AIBinder* binder) {
    if (binder == nullptr) {
        return;
    }

    binder->incStrong(nullptr);
}
void AIBinder_decStrong(AIBinder* binder) {
    if (binder == nullptr) {
        ALOGE("%s: on null binder", __func__);
        return;
    }

    binder->decStrong(nullptr);
}
int32_t AIBinder_debugGetRefCount(AIBinder* binder) {
    if (binder == nullptr) {
        ALOGE("%s: on null binder", __func__);
        return -1;
    }

    return binder->getStrongCount();
}

bool AIBinder_associateClass(AIBinder* binder, const AIBinder_Class* clazz) {
    if (binder == nullptr) {
        return false;
    }

    return binder->associateClass(clazz);
}

const AIBinder_Class* AIBinder_getClass(AIBinder* binder) {
    if (binder == nullptr) {
        return nullptr;
    }

    return binder->getClass();
}

void* AIBinder_getUserData(AIBinder* binder) {
    if (binder == nullptr) {
        return nullptr;
    }

    ABBinder* bBinder = binder->asABBinder();
    if (bBinder == nullptr) {
        return nullptr;
    }

    return bBinder->getUserData();
}

binder_status_t AIBinder_prepareTransaction(AIBinder* binder, AParcel** in) {
    if (binder == nullptr || in == nullptr) {
        ALOGE("%s: requires non-null parameters binder (%p) and in (%p).", __func__, binder, in);
        return STATUS_UNEXPECTED_NULL;
    }
    const AIBinder_Class* clazz = binder->getClass();
    if (clazz == nullptr) {
        ALOGE("%s: Class must be defined for a remote binder transaction. See "
              "AIBinder_associateClass.",
              __func__);
        return STATUS_INVALID_OPERATION;
    }

    *in = new AParcel(binder);
    (*in)->get()->markForBinder(binder->getBinder());

    status_t status = android::OK;

    // note - this is the only read of a value in clazz, and it comes with a warning
    // on the API itself. Do not copy this design. Instead, attach data in a new
    // version of the prepareTransaction function.
    if (clazz->writeHeader) {
        status = (*in)->get()->writeInterfaceToken(clazz->getInterfaceDescriptor());
    }
    binder_status_t ret = PruneStatusT(status);

    if (ret != STATUS_OK) {
        delete *in;
        *in = nullptr;
    }

    return ret;
}

static void DestroyParcel(AParcel** parcel) {
    delete *parcel;
    *parcel = nullptr;
}

binder_status_t AIBinder_transact(AIBinder* binder, transaction_code_t code, AParcel** in,
                                  AParcel** out, binder_flags_t flags) {
    const AIBinder_Class* clazz = binder ? binder->getClass() : nullptr;

    std::string sectionName;
    bool tracingEnabled = get_trace_enabled_tags() & ATRACE_TAG_AIDL;
    if (tracingEnabled) {
        sectionName = getTraceSectionName(clazz, code, false /*isServer*/);
        trace_begin(ATRACE_TAG_AIDL, sectionName.c_str());
    }

    scope_guard guard = make_scope_guard([&]() {
        if (tracingEnabled) trace_end(ATRACE_TAG_AIDL);
    });

    if (in == nullptr) {
        ALOGE("%s: requires non-null in parameter", __func__);
        return STATUS_UNEXPECTED_NULL;
    }

    using AutoParcelDestroyer = std::unique_ptr<AParcel*, void (*)(AParcel**)>;
    // This object is the input to the transaction. This function takes ownership of it and deletes
    // it.
    AutoParcelDestroyer forIn(in, DestroyParcel);

    if (!isUserCommand(code)) {
        ALOGE("%s: Only user-defined transactions can be made from the NDK, but requested: %d",
              __func__, code);
        return STATUS_UNKNOWN_TRANSACTION;
    }

    constexpr binder_flags_t kAllFlags = FLAG_PRIVATE_VENDOR | FLAG_ONEWAY | FLAG_CLEAR_BUF;
    if ((flags & ~kAllFlags) != 0) {
        ALOGE("%s: Unrecognized flags sent: %d", __func__, flags);
        return STATUS_BAD_VALUE;
    }

    if (binder == nullptr || *in == nullptr || out == nullptr) {
        ALOGE("%s: requires non-null parameters binder (%p), in (%p), and out (%p).", __func__,
              binder, in, out);
        return STATUS_UNEXPECTED_NULL;
    }

    if ((*in)->getBinder() != binder) {
        ALOGE("%s: parcel is associated with binder object %p but called with %p", __func__, binder,
              (*in)->getBinder());
        return STATUS_BAD_VALUE;
    }

    *out = new AParcel(binder);

    status_t status = binder->getBinder()->transact(code, *(*in)->get(), (*out)->get(), flags);
    binder_status_t ret = PruneStatusT(status);

    if (ret != STATUS_OK) {
        delete *out;
        *out = nullptr;
    }

    return ret;
}

AIBinder_DeathRecipient* AIBinder_DeathRecipient_new(
        AIBinder_DeathRecipient_onBinderDied onBinderDied) {
    if (onBinderDied == nullptr) {
        ALOGE("%s: requires non-null onBinderDied parameter.", __func__);
        return nullptr;
    }
    auto ret = new AIBinder_DeathRecipient(onBinderDied);
    ret->incStrong(nullptr);
    return ret;
}

void AIBinder_DeathRecipient_setOnUnlinked(AIBinder_DeathRecipient* recipient,
                                           AIBinder_DeathRecipient_onBinderUnlinked onUnlinked) {
    if (recipient == nullptr) {
        return;
    }

    recipient->setOnUnlinked(onUnlinked);
}

void AIBinder_DeathRecipient_delete(AIBinder_DeathRecipient* recipient) {
    if (recipient == nullptr) {
        return;
    }

    recipient->decStrong(nullptr);
}

binder_status_t AIBinder_getExtension(AIBinder* binder, AIBinder** outExt) {
    if (binder == nullptr || outExt == nullptr) {
        if (outExt != nullptr) {
            *outExt = nullptr;
        }
        return STATUS_UNEXPECTED_NULL;
    }

    sp<IBinder> ext;
    status_t res = binder->getBinder()->getExtension(&ext);

    if (res != android::OK) {
        *outExt = nullptr;
        return PruneStatusT(res);
    }

    sp<AIBinder> ret = ABpBinder::lookupOrCreateFromBinder(ext);
    if (ret != nullptr) ret->incStrong(binder);

    *outExt = ret.get();
    return STATUS_OK;
}

binder_status_t AIBinder_setExtension(AIBinder* binder, AIBinder* ext) {
    if (binder == nullptr || ext == nullptr) {
        return STATUS_UNEXPECTED_NULL;
    }

    ABBinder* rawBinder = binder->asABBinder();
    if (rawBinder == nullptr) {
        return STATUS_INVALID_OPERATION;
    }

    rawBinder->setExtension(ext->getBinder());
    return STATUS_OK;
}

// platform methods follow

void AIBinder_setRequestingSid(AIBinder* binder, bool requestingSid) {
    ABBinder* localBinder = binder->asABBinder();
    LOG_ALWAYS_FATAL_IF(localBinder == nullptr,
                        "AIBinder_setRequestingSid must be called on a local binder");

    localBinder->setRequestingSid(requestingSid);
}

#ifdef BINDER_WITH_KERNEL_IPC
const char* AIBinder_getCallingSid() {
    return ::android::IPCThreadState::self()->getCallingSid();
}
#endif

void AIBinder_setMinSchedulerPolicy(AIBinder* binder, int policy, int priority) {
    binder->asABBinder()->setMinSchedulerPolicy(policy, priority);
}

void AIBinder_setInheritRt(AIBinder* binder, bool inheritRt) {
    ABBinder* localBinder = binder->asABBinder();
    LOG_ALWAYS_FATAL_IF(localBinder == nullptr,
                        "AIBinder_setInheritRt must be called on a local binder");

    localBinder->setInheritRt(inheritRt);
}