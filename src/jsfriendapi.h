/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jsfriendapi_h
#define jsfriendapi_h

#include "mozilla/Atomics.h"
#include "mozilla/Casting.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"

#include "jspubtd.h"

#include "js/CallArgs.h"
#include "js/CallNonGenericMethod.h"
#include "js/CharacterEncoding.h"
#include "js/Class.h"
#include "js/ErrorReport.h"
#include "js/Exception.h"
#include "js/friend/ErrorMessages.h"
#include "js/HeapAPI.h"
#include "js/TypeDecls.h"
#include "js/Utility.h"

#ifndef JS_STACK_GROWTH_DIRECTION
#  ifdef __hppa
#    define JS_STACK_GROWTH_DIRECTION (1)
#  else
#    define JS_STACK_GROWTH_DIRECTION (-1)
#  endif
#endif

#if JS_STACK_GROWTH_DIRECTION > 0
#  define JS_CHECK_STACK_SIZE(limit, sp) (MOZ_LIKELY((uintptr_t)(sp) < (limit)))
#else
#  define JS_CHECK_STACK_SIZE(limit, sp) (MOZ_LIKELY((uintptr_t)(sp) > (limit)))
#endif

struct JSErrorFormatString;
struct JSJitInfo;

namespace JS {
template <class T>
class Heap;

class ExceptionStack;
} /* namespace JS */

namespace js {
class JS_FRIEND_API BaseProxyHandler;
class InterpreterFrame;
} /* namespace js */

extern JS_FRIEND_API void JS_SetGrayGCRootsTracer(JSContext* cx,
                                                  JSTraceDataOp traceOp,
                                                  void* data);

extern JS_FRIEND_API JSObject* JS_FindCompilationScope(JSContext* cx,
                                                       JS::HandleObject obj);

extern JS_FRIEND_API JSFunction* JS_GetObjectFunction(JSObject* obj);

extern JS_FRIEND_API bool JS_SplicePrototype(JSContext* cx,
                                             JS::HandleObject obj,
                                             JS::HandleObject proto);

extern JS_FRIEND_API JSObject* JS_NewObjectWithUniqueType(
    JSContext* cx, const JSClass* clasp, JS::HandleObject proto);

/**
 * Allocate an object in exactly the same way as JS_NewObjectWithGivenProto, but
 * without invoking the metadata callback on it.  This allows creation of
 * internal bookkeeping objects that are guaranteed to not have metadata
 * attached to them.
 */
extern JS_FRIEND_API JSObject* JS_NewObjectWithoutMetadata(
    JSContext* cx, const JSClass* clasp, JS::Handle<JSObject*> proto);

extern JS_FRIEND_API bool JS_NondeterministicGetWeakMapKeys(
    JSContext* cx, JS::HandleObject obj, JS::MutableHandleObject ret);

extern JS_FRIEND_API bool JS_NondeterministicGetWeakSetKeys(
    JSContext* cx, JS::HandleObject obj, JS::MutableHandleObject ret);

// Raw JSScript* because this needs to be callable from a signal handler.
extern JS_FRIEND_API unsigned JS_PCToLineNumber(JSScript* script,
                                                jsbytecode* pc,
                                                unsigned* columnp = nullptr);

/**
 * Determine whether the given object is backed by a DeadObjectProxy.
 *
 * Such objects hold no other objects (they have no outgoing reference edges)
 * and will throw if you touch them (e.g. by reading/writing a property).
 */
extern JS_FRIEND_API bool JS_IsDeadWrapper(JSObject* obj);

/**
 * Creates a new dead wrapper object in the given scope. To be used when
 * attempting to wrap objects from scopes which are already dead.
 *
 * If origObject is passed, it must be an proxy object, and will be
 * used to determine the characteristics of the new dead wrapper.
 */
extern JS_FRIEND_API JSObject* JS_NewDeadWrapper(
    JSContext* cx, JSObject* origObject = nullptr);

namespace js {

/**
 * Get the script private value associated with an object, if any.
 *
 * The private value is set with SetScriptPrivate() or SetModulePrivate() and is
 * internally stored on the relevant ScriptSourceObject.
 *
 * This is used by the cycle collector to trace through
 * ScriptSourceObjects. This allows private values to contain an nsISupports
 * pointer and hence support references to cycle collected C++ objects.
 */
JS_FRIEND_API JS::Value MaybeGetScriptPrivate(JSObject* object);

}  // namespace js

/*
 * Used by the cycle collector to trace through a shape or object group and
 * all cycle-participating data it reaches, using bounded stack space.
 */
extern JS_FRIEND_API void JS_TraceShapeCycleCollectorChildren(
    JS::CallbackTracer* trc, JS::GCCellPtr shape);
extern JS_FRIEND_API void JS_TraceObjectGroupCycleCollectorChildren(
    JS::CallbackTracer* trc, JS::GCCellPtr group);

extern JS_FRIEND_API JSPrincipals* JS_GetScriptPrincipals(JSScript* script);

namespace js {

// Release-assert the compartment contains exactly one realm.
extern JS_FRIEND_API void AssertCompartmentHasSingleRealm(
    JS::Compartment* comp);

extern JS_FRIEND_API JS::Realm* GetScriptRealm(JSScript* script);
} /* namespace js */

extern JS_FRIEND_API bool JS_ScriptHasMutedErrors(JSScript* script);

extern JS_FRIEND_API JSObject* JS_CloneObject(JSContext* cx,
                                              JS::HandleObject obj,
                                              JS::HandleObject proto);

/**
 * Copy the own properties of src to dst in a fast way.  src and dst must both
 * be native and must be in the compartment of cx.  They must have the same
 * class, the same parent, and the same prototype.  Class reserved slots will
 * NOT be copied.
 *
 * dst must not have any properties on it before this function is called.
 *
 * src must have been allocated via JS_NewObjectWithoutMetadata so that we can
 * be sure it has no metadata that needs copying to dst.  This also means that
 * dst needs to have the compartment global as its parent.  This function will
 * preserve the existing metadata on dst, if any.
 */
extern JS_FRIEND_API bool JS_InitializePropertiesFromCompatibleNativeObject(
    JSContext* cx, JS::HandleObject dst, JS::HandleObject src);

namespace js {

JS_FRIEND_API bool GetBuiltinClass(JSContext* cx, JS::HandleObject obj,
                                   ESClass* cls);

JS_FRIEND_API bool IsArgumentsObject(JS::HandleObject obj);

JS_FRIEND_API const char* ObjectClassName(JSContext* cx, JS::HandleObject obj);

JS_FRIEND_API bool AddRawValueRoot(JSContext* cx, JS::Value* vp,
                                   const char* name);

JS_FRIEND_API void RemoveRawValueRoot(JSContext* cx, JS::Value* vp);

JS_FRIEND_API JSAtom* GetPropertyNameFromPC(JSScript* script, jsbytecode* pc);

}  // namespace js

namespace JS {

/**
 * Set all of the uninitialized lexicals on an object to undefined. Return
 * true if any lexicals were initialized and false otherwise.
 * */
extern JS_FRIEND_API bool ForceLexicalInitialization(JSContext* cx,
                                                     HandleObject obj);

/**
 * Whether we are poisoning unused/released data for error detection. Governed
 * by the JS_GC_POISONING #ifdef as well as the $JSGC_DISABLE_POISONING
 * environment variable.
 */
extern JS_FRIEND_API int IsGCPoisoning();

extern JS_FRIEND_API JSPrincipals* GetRealmPrincipals(JS::Realm* realm);

extern JS_FRIEND_API void SetRealmPrincipals(JS::Realm* realm,
                                             JSPrincipals* principals);

extern JS_FRIEND_API bool GetIsSecureContext(JS::Realm* realm);

}  // namespace JS

/**
 * Copies all own properties from |obj| to |target|. Both |obj| and |target|
 * must not be cross-compartment wrappers because we have to enter their realms.
 *
 * This function immediately enters a realm, and does not impose any
 * restrictions on the realm of |cx|.
 */
extern JS_FRIEND_API bool JS_CopyPropertiesFrom(JSContext* cx,
                                                JS::HandleObject target,
                                                JS::HandleObject obj);

/*
 * Single-property version of the above. This function asserts that an |own|
 * property of the given name exists on |obj|.
 *
 * On entry, |cx| must be same-compartment with |obj|. |target| must not be a
 * cross-compartment wrapper because we have to enter its realm.
 *
 * The copyBehavior argument controls what happens with
 * non-configurable properties.
 */
typedef enum {
  MakeNonConfigurableIntoConfigurable,
  CopyNonConfigurableAsIs
} PropertyCopyBehavior;

extern JS_FRIEND_API bool JS_CopyPropertyFrom(
    JSContext* cx, JS::HandleId id, JS::HandleObject target,
    JS::HandleObject obj,
    PropertyCopyBehavior copyBehavior = CopyNonConfigurableAsIs);

extern JS_FRIEND_API bool JS_WrapPropertyDescriptor(
    JSContext* cx, JS::MutableHandle<JS::PropertyDescriptor> desc);

struct JSFunctionSpecWithHelp {
  const char* name;
  JSNative call;
  uint16_t nargs;
  uint16_t flags;
  const JSJitInfo* jitInfo;
  const char* usage;
  const char* help;
};

#define JS_FN_HELP(name, call, nargs, flags, usage, help) \
  { name, call, nargs, (flags) | JSPROP_ENUMERATE, nullptr, usage, help }
#define JS_INLINABLE_FN_HELP(name, call, nargs, flags, native, usage, help)    \
  {                                                                            \
    name, call, nargs, (flags) | JSPROP_ENUMERATE, &js::jit::JitInfo_##native, \
        usage, help                                                            \
  }
#define JS_FS_HELP_END \
  { nullptr, nullptr, 0, 0, nullptr, nullptr }

extern JS_FRIEND_API bool JS_DefineFunctionsWithHelp(
    JSContext* cx, JS::HandleObject obj, const JSFunctionSpecWithHelp* fs);

namespace js {

/**
 * Use the runtime's internal handling of job queues for Promise jobs.
 *
 * Most embeddings, notably web browsers, will have their own task scheduling
 * systems and need to integrate handling of Promise jobs into that, so they
 * will want to manage job queues themselves. For basic embeddings such as the
 * JS shell that don't have an event loop of their own, it's easier to have
 * SpiderMonkey handle job queues internally.
 *
 * Note that the embedding still has to trigger processing of job queues at
 * right time(s), such as after evaluation of a script has run to completion.
 */
extern JS_FRIEND_API bool UseInternalJobQueues(JSContext* cx);

/**
 * Enqueue |job| on the internal job queue.
 *
 * This is useful in tests for creating situations where a call occurs with no
 * other JavaScript on the stack.
 */
extern JS_FRIEND_API bool EnqueueJob(JSContext* cx, JS::HandleObject job);

/**
 * Instruct the runtime to stop draining the internal job queue.
 *
 * Useful if the embedding is in the process of quitting in reaction to a
 * builtin being called, or if it wants to resume executing jobs later on.
 */
extern JS_FRIEND_API void StopDrainingJobQueue(JSContext* cx);

extern JS_FRIEND_API void RunJobs(JSContext* cx);

extern JS_FRIEND_API JS::Zone* GetRealmZone(JS::Realm* realm);

using PreserveWrapperCallback = bool (*)(JSContext*, JS::HandleObject);
using HasReleasedWrapperCallback = bool (*)(JS::HandleObject);

extern JS_FRIEND_API bool IsSystemRealm(JS::Realm* realm);

extern JS_FRIEND_API bool IsSystemCompartment(JS::Compartment* comp);

extern JS_FRIEND_API bool IsSystemZone(JS::Zone* zone);

extern JS_FRIEND_API bool IsAtomsZone(JS::Zone* zone);

struct WeakMapTracer {
  JSRuntime* runtime;

  explicit WeakMapTracer(JSRuntime* rt) : runtime(rt) {}

  // Weak map tracer callback, called once for every binding of every
  // weak map that was live at the time of the last garbage collection.
  //
  // m will be nullptr if the weak map is not contained in a JS Object.
  //
  // The callback should not GC (and will assert in a debug build if it does
  // so.)
  virtual void trace(JSObject* m, JS::GCCellPtr key, JS::GCCellPtr value) = 0;
};

extern JS_FRIEND_API void TraceWeakMaps(WeakMapTracer* trc);

extern JS_FRIEND_API bool AreGCGrayBitsValid(JSRuntime* rt);

extern JS_FRIEND_API bool ZoneGlobalsAreAllGray(JS::Zone* zone);

extern JS_FRIEND_API bool IsCompartmentZoneSweepingOrCompacting(
    JS::Compartment* comp);

using IterateGCThingCallback = void (*)(void*, JS::GCCellPtr,
                                        const JS::AutoRequireNoGC&);

extern JS_FRIEND_API void VisitGrayWrapperTargets(
    JS::Zone* zone, IterateGCThingCallback callback, void* closure);

/**
 * Invoke cellCallback on every gray JSObject in the given zone.
 */
extern JS_FRIEND_API void IterateGrayObjects(
    JS::Zone* zone, IterateGCThingCallback cellCallback, void* data);

#if defined(JS_GC_ZEAL) || defined(DEBUG)
// Trace the heap and check there are no black to gray edges. These are
// not allowed since the cycle collector could throw away the gray thing and
// leave a dangling pointer.
//
// This doesn't trace weak maps as these are handled separately.
extern JS_FRIEND_API bool CheckGrayMarkingState(JSRuntime* rt);
#endif

#ifdef JS_HAS_CTYPES
extern JS_FRIEND_API size_t
SizeOfDataIfCDataObject(mozilla::MallocSizeOf mallocSizeOf, JSObject* obj);
#endif

// Note: this returns nullptr iff |zone| is the atoms zone.
extern JS_FRIEND_API JS::Realm* GetAnyRealmInZone(JS::Zone* zone);

// Returns the first realm's global in a compartment. Note: this is not
// guaranteed to always be the same realm because individual realms can be
// collected by the GC.
extern JS_FRIEND_API JSObject* GetFirstGlobalInCompartment(
    JS::Compartment* comp);

// Returns true if the compartment contains a global object and this global is
// not being collected.
extern JS_FRIEND_API bool CompartmentHasLiveGlobal(JS::Compartment* comp);

// Returns true if this compartment can be shared across multiple Realms.  Used
// when we're looking for an existing compartment to place a new Realm in.
extern JS_FRIEND_API bool IsSharableCompartment(JS::Compartment* comp);

/*
 * Shadow declarations of JS internal structures, for access by inline access
 * functions below. Do not use these structures in any other way. When adding
 * new fields for access by inline methods, make sure to add static asserts to
 * the original header file to ensure that offsets are consistent.
 */
namespace shadow {

struct ObjectGroup {
  const JSClass* clasp;
  JSObject* proto;
  JS::Realm* realm;
};

struct BaseShape {
  const JSClass* clasp_;
  JSObject* parent;
};

class Shape {
 public:
  shadow::BaseShape* base;
  jsid _1;
  uint32_t immutableFlags;

  static const uint32_t FIXED_SLOTS_SHIFT = 24;
  static const uint32_t FIXED_SLOTS_MASK = 0x1f << FIXED_SLOTS_SHIFT;
};

/**
 * This layout is shared by all native objects. For non-native objects, the
 * group may always be accessed safely, and other members may be as well,
 * depending on the object's specific layout.
 */
struct Object {
  shadow::ObjectGroup* group;
  shadow::Shape* shape;
  JS::Value* slots;
  void* _1;

  static constexpr size_t MAX_FIXED_SLOTS = 16;

  size_t numFixedSlots() const {
    return (shape->immutableFlags & Shape::FIXED_SLOTS_MASK) >>
           Shape::FIXED_SLOTS_SHIFT;
  }

  JS::Value* fixedSlots() const {
    return (JS::Value*)(uintptr_t(this) + sizeof(shadow::Object));
  }

  JS::Value& slotRef(size_t slot) const {
    size_t nfixed = numFixedSlots();
    if (slot < nfixed) {
      return fixedSlots()[slot];
    }
    return slots[slot - nfixed];
  }
};

struct Function {
  Object base;
  uint16_t nargs;
  uint16_t flags;
  /* Used only for natives */
  JSNative native;
  const JSJitInfo* jitinfo;
  void* _1;
};

} /* namespace shadow */

// This is equal to |&JSObject::class_|.  Use it in places where you don't want
// to #include vm/JSObject.h.
extern JS_FRIEND_DATA const JSClass* const ObjectClassPtr;

inline const JSClass* GetObjectClass(const JSObject* obj) {
  return reinterpret_cast<const shadow::Object*>(obj)->group->clasp;
}

JS_FRIEND_API const JSClass* ProtoKeyToClass(JSProtoKey key);

// Returns the key for the class inherited by a given standard class (that
// is to say, the prototype of this standard class's prototype).
//
// You must be sure that this corresponds to a standard class with a cached
// JSProtoKey before calling this function. In general |key| will match the
// cached proto key, except in cases where multiple JSProtoKeys share a
// JSClass.
inline JSProtoKey InheritanceProtoKeyForStandardClass(JSProtoKey key) {
  // [Object] has nothing to inherit from.
  if (key == JSProto_Object) {
    return JSProto_Null;
  }

  // If we're ClassSpec defined return the proto key from that
  if (ProtoKeyToClass(key)->specDefined()) {
    return ProtoKeyToClass(key)->specInheritanceProtoKey();
  }

  // Otherwise, we inherit [Object].
  return JSProto_Object;
}

JS_FRIEND_API bool ShouldIgnorePropertyDefinition(JSContext* cx, JSProtoKey key,
                                                  jsid id);

JS_FRIEND_API bool IsFunctionObject(JSObject* obj);

JS_FRIEND_API bool UninlinedIsCrossCompartmentWrapper(const JSObject* obj);

static MOZ_ALWAYS_INLINE JS::Compartment* GetObjectCompartment(JSObject* obj) {
  JS::Realm* realm = reinterpret_cast<shadow::Object*>(obj)->group->realm;
  return JS::GetCompartmentForRealm(realm);
}

// CrossCompartmentWrappers are shared by all realms within the compartment, so
// getting a wrapper's realm usually doesn't make sense.
static MOZ_ALWAYS_INLINE JS::Realm* GetNonCCWObjectRealm(JSObject* obj) {
  MOZ_ASSERT(!js::UninlinedIsCrossCompartmentWrapper(obj));
  return reinterpret_cast<shadow::Object*>(obj)->group->realm;
}

JS_FRIEND_API JSObject* GetPrototypeNoProxy(JSObject* obj);

JS_FRIEND_API void AssertSameCompartment(JSContext* cx, JSObject* obj);

JS_FRIEND_API void AssertSameCompartment(JSContext* cx, JS::HandleValue v);

#ifdef JS_DEBUG
JS_FRIEND_API void AssertSameCompartment(JSObject* objA, JSObject* objB);
#else
inline void AssertSameCompartment(JSObject* objA, JSObject* objB) {}
#endif

JS_FRIEND_API void NotifyAnimationActivity(JSObject* obj);

JS_FRIEND_API JSFunction* DefineFunctionWithReserved(
    JSContext* cx, JSObject* obj, const char* name, JSNative call,
    unsigned nargs, unsigned attrs);

JS_FRIEND_API JSFunction* NewFunctionWithReserved(JSContext* cx, JSNative call,
                                                  unsigned nargs,
                                                  unsigned flags,
                                                  const char* name);

JS_FRIEND_API JSFunction* NewFunctionByIdWithReserved(JSContext* cx,
                                                      JSNative native,
                                                      unsigned nargs,
                                                      unsigned flags, jsid id);

JS_FRIEND_API const JS::Value& GetFunctionNativeReserved(JSObject* fun,
                                                         size_t which);

JS_FRIEND_API void SetFunctionNativeReserved(JSObject* fun, size_t which,
                                             const JS::Value& val);

JS_FRIEND_API bool FunctionHasNativeReserved(JSObject* fun);

JS_FRIEND_API bool GetObjectProto(JSContext* cx, JS::HandleObject obj,
                                  JS::MutableHandleObject proto);

extern JS_FRIEND_API JSObject* GetStaticPrototype(JSObject* obj);

JS_FRIEND_API bool GetRealmOriginalEval(JSContext* cx,
                                        JS::MutableHandleObject eval);

inline void* GetObjectPrivate(JSObject* obj) {
  MOZ_ASSERT(GetObjectClass(obj)->flags & JSCLASS_HAS_PRIVATE);
  const shadow::Object* nobj = reinterpret_cast<const shadow::Object*>(obj);
  void** addr =
      reinterpret_cast<void**>(&nobj->fixedSlots()[nobj->numFixedSlots()]);
  return *addr;
}

/**
 * Get the value stored in an object's reserved slot. This can be used with
 * both native objects and proxies, but if |obj| is known to be a proxy
 * GetProxyReservedSlot is a bit more efficient.
 */
inline const JS::Value& GetReservedSlot(JSObject* obj, size_t slot) {
  MOZ_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
  return reinterpret_cast<const shadow::Object*>(obj)->slotRef(slot);
}

JS_FRIEND_API void SetReservedSlotWithBarrier(JSObject* obj, size_t slot,
                                              const JS::Value& value);

/**
 * Store a value in an object's reserved slot. This can be used with
 * both native objects and proxies, but if |obj| is known to be a proxy
 * SetProxyReservedSlot is a bit more efficient.
 */
inline void SetReservedSlot(JSObject* obj, size_t slot,
                            const JS::Value& value) {
  MOZ_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
  shadow::Object* sobj = reinterpret_cast<shadow::Object*>(obj);
  if (sobj->slotRef(slot).isGCThing() || value.isGCThing()) {
    SetReservedSlotWithBarrier(obj, slot, value);
  } else {
    sobj->slotRef(slot) = value;
  }
}

JS_FRIEND_API uint32_t GetObjectSlotSpan(JSObject* obj);

inline const JS::Value& GetObjectSlot(JSObject* obj, size_t slot) {
  MOZ_ASSERT(slot < GetObjectSlotSpan(obj));
  return reinterpret_cast<const shadow::Object*>(obj)->slotRef(slot);
}

MOZ_ALWAYS_INLINE size_t GetAtomLength(JSAtom* atom) {
  return reinterpret_cast<JS::shadow::String*>(atom)->length();
}

// Maximum length of a JS string. This is chosen so that the number of bytes
// allocated for a null-terminated TwoByte string still fits in int32_t.
static const uint32_t MaxStringLength = (1 << 30) - 2;

static_assert((uint64_t(MaxStringLength) + 1) * sizeof(char16_t) <= INT32_MAX,
              "size of null-terminated JSString char buffer must fit in "
              "INT32_MAX");

MOZ_ALWAYS_INLINE size_t GetLinearStringLength(JSLinearString* s) {
  return reinterpret_cast<JS::shadow::String*>(s)->length();
}

MOZ_ALWAYS_INLINE bool LinearStringHasLatin1Chars(JSLinearString* s) {
  return reinterpret_cast<JS::shadow::String*>(s)->flags() &
         JS::shadow::String::LATIN1_CHARS_BIT;
}

MOZ_ALWAYS_INLINE bool AtomHasLatin1Chars(JSAtom* atom) {
  return reinterpret_cast<JS::shadow::String*>(atom)->flags() &
         JS::shadow::String::LATIN1_CHARS_BIT;
}

MOZ_ALWAYS_INLINE bool StringHasLatin1Chars(JSString* s) {
  return reinterpret_cast<JS::shadow::String*>(s)->flags() &
         JS::shadow::String::LATIN1_CHARS_BIT;
}

MOZ_ALWAYS_INLINE const JS::Latin1Char* GetLatin1LinearStringChars(
    const JS::AutoRequireNoGC& nogc, JSLinearString* linear) {
  MOZ_ASSERT(LinearStringHasLatin1Chars(linear));

  using JS::shadow::String;
  String* s = reinterpret_cast<String*>(linear);
  if (s->flags() & String::INLINE_CHARS_BIT) {
    return s->inlineStorageLatin1;
  }
  return s->nonInlineCharsLatin1;
}

MOZ_ALWAYS_INLINE const char16_t* GetTwoByteLinearStringChars(
    const JS::AutoRequireNoGC& nogc, JSLinearString* linear) {
  MOZ_ASSERT(!LinearStringHasLatin1Chars(linear));

  using JS::shadow::String;
  String* s = reinterpret_cast<String*>(linear);
  if (s->flags() & String::INLINE_CHARS_BIT) {
    return s->inlineStorageTwoByte;
  }
  return s->nonInlineCharsTwoByte;
}

MOZ_ALWAYS_INLINE char16_t GetLinearStringCharAt(JSLinearString* linear,
                                                 size_t index) {
  MOZ_ASSERT(index < GetLinearStringLength(linear));
  JS::AutoCheckCannotGC nogc;
  return LinearStringHasLatin1Chars(linear)
             ? GetLatin1LinearStringChars(nogc, linear)[index]
             : GetTwoByteLinearStringChars(nogc, linear)[index];
}

MOZ_ALWAYS_INLINE JSLinearString* AtomToLinearString(JSAtom* atom) {
  return reinterpret_cast<JSLinearString*>(atom);
}

MOZ_ALWAYS_INLINE const JS::Latin1Char* GetLatin1AtomChars(
    const JS::AutoRequireNoGC& nogc, JSAtom* atom) {
  return GetLatin1LinearStringChars(nogc, AtomToLinearString(atom));
}

MOZ_ALWAYS_INLINE const char16_t* GetTwoByteAtomChars(
    const JS::AutoRequireNoGC& nogc, JSAtom* atom) {
  return GetTwoByteLinearStringChars(nogc, AtomToLinearString(atom));
}

MOZ_ALWAYS_INLINE bool IsExternalString(
    JSString* str, const JSExternalStringCallbacks** callbacks,
    const char16_t** chars) {
  using JS::shadow::String;
  String* s = reinterpret_cast<String*>(str);

  if ((s->flags() & String::TYPE_FLAGS_MASK) != String::EXTERNAL_FLAGS) {
    return false;
  }

  MOZ_ASSERT(JS_IsExternalString(str));
  *callbacks = s->externalCallbacks;
  *chars = s->nonInlineCharsTwoByte;
  return true;
}

JS_FRIEND_API JSLinearString* StringToLinearStringSlow(JSContext* cx,
                                                       JSString* str);

MOZ_ALWAYS_INLINE JSLinearString* StringToLinearString(JSContext* cx,
                                                       JSString* str) {
  using JS::shadow::String;
  String* s = reinterpret_cast<String*>(str);
  if (MOZ_UNLIKELY(!(s->flags() & String::LINEAR_BIT))) {
    return StringToLinearStringSlow(cx, str);
  }
  return reinterpret_cast<JSLinearString*>(str);
}

template <typename CharType>
MOZ_ALWAYS_INLINE void CopyLinearStringChars(CharType* dest, JSLinearString* s,
                                             size_t len, size_t start = 0);

MOZ_ALWAYS_INLINE void CopyLinearStringChars(char16_t* dest, JSLinearString* s,
                                             size_t len, size_t start = 0) {
  MOZ_ASSERT(start + len <= GetLinearStringLength(s));
  JS::AutoCheckCannotGC nogc;
  if (LinearStringHasLatin1Chars(s)) {
    const JS::Latin1Char* src = GetLatin1LinearStringChars(nogc, s);
    for (size_t i = 0; i < len; i++) {
      dest[i] = src[start + i];
    }
  } else {
    const char16_t* src = GetTwoByteLinearStringChars(nogc, s);
    mozilla::PodCopy(dest, src + start, len);
  }
}

MOZ_ALWAYS_INLINE void CopyLinearStringChars(char* dest, JSLinearString* s,
                                             size_t len, size_t start = 0) {
  MOZ_ASSERT(start + len <= GetLinearStringLength(s));
  JS::AutoCheckCannotGC nogc;
  if (LinearStringHasLatin1Chars(s)) {
    const JS::Latin1Char* src = GetLatin1LinearStringChars(nogc, s);
    for (size_t i = 0; i < len; i++) {
      dest[i] = char(src[start + i]);
    }
  } else {
    const char16_t* src = GetTwoByteLinearStringChars(nogc, s);
    for (size_t i = 0; i < len; i++) {
      dest[i] = char(src[start + i]);
    }
  }
}

template <typename CharType>
inline bool CopyStringChars(JSContext* cx, CharType* dest, JSString* s,
                            size_t len, size_t start = 0) {
  JSLinearString* linear = StringToLinearString(cx, s);
  if (!linear) {
    return false;
  }

  CopyLinearStringChars(dest, linear, len, start);
  return true;
}

/**
 * Add some or all property keys of obj to the id vector *props.
 *
 * The flags parameter controls which property keys are added. Pass a
 * combination of the following bits:
 *
 *     JSITER_OWNONLY - Don't also search the prototype chain; only consider
 *       obj's own properties.
 *
 *     JSITER_HIDDEN - Include nonenumerable properties.
 *
 *     JSITER_SYMBOLS - Include property keys that are symbols. The default
 *       behavior is to filter out symbols.
 *
 *     JSITER_SYMBOLSONLY - Exclude non-symbol property keys.
 *
 * This is the closest C++ API we have to `Reflect.ownKeys(obj)`, or
 * equivalently, the ES6 [[OwnPropertyKeys]] internal method. Pass
 * `JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS` as flags to get
 * results that match the output of Reflect.ownKeys.
 */
JS_FRIEND_API bool GetPropertyKeys(JSContext* cx, JS::HandleObject obj,
                                   unsigned flags,
                                   JS::MutableHandleIdVector props);

JS_FRIEND_API bool AppendUnique(JSContext* cx, JS::MutableHandleIdVector base,
                                JS::HandleIdVector others);

/**
 * Determine whether the given string is an array index in the sense of
 * <https://tc39.github.io/ecma262/#array-index>.
 *
 * If it isn't, returns false.
 *
 * If it is, returns true and outputs the index in *indexp.
 */
JS_FRIEND_API bool StringIsArrayIndex(JSLinearString* str, uint32_t* indexp);

/**
 * Overloads of StringIsArrayIndex taking (char*,length) pairs.  These
 * behave the same as the JSLinearString version.
 */
JS_FRIEND_API bool StringIsArrayIndex(const char* str, uint32_t length,
                                      uint32_t* indexp);

JS_FRIEND_API bool StringIsArrayIndex(const char16_t* str, uint32_t length,
                                      uint32_t* indexp);

JS_FRIEND_API void SetPreserveWrapperCallbacks(
    JSContext* cx, PreserveWrapperCallback preserveWrapper,
    HasReleasedWrapperCallback hasReleasedWrapper);

JS_FRIEND_API bool IsObjectInContextCompartment(JSObject* obj,
                                                const JSContext* cx);

/*
 * NB: keep these in sync with the copy in builtin/SelfHostingDefines.h.
 */
/* 0x1 is no longer used */
/* 0x2 is no longer used */
/* 0x4 is no longer used */
#define JSITER_OWNONLY 0x8      /* iterate over obj's own properties only */
#define JSITER_HIDDEN 0x10      /* also enumerate non-enumerable properties */
#define JSITER_SYMBOLS 0x20     /* also include symbol property keys */
#define JSITER_SYMBOLSONLY 0x40 /* exclude string property keys */
#define JSITER_FORAWAITOF 0x80  /* for-await-of */

JS_FRIEND_API void StartPCCountProfiling(JSContext* cx);

JS_FRIEND_API void StopPCCountProfiling(JSContext* cx);

JS_FRIEND_API void PurgePCCounts(JSContext* cx);

JS_FRIEND_API size_t GetPCCountScriptCount(JSContext* cx);

JS_FRIEND_API JSString* GetPCCountScriptSummary(JSContext* cx, size_t script);

JS_FRIEND_API JSString* GetPCCountScriptContents(JSContext* cx, size_t script);

/**
 * Generate lcov trace file content for the current realm, and allocate a new
 * buffer and return the content in it, the size of the newly allocated content
 * within the buffer would be set to the length out-param. The 'All' variant
 * will collect data for all realms in the runtime.
 *
 * In case of out-of-memory, this function returns nullptr. The length
 * out-param is undefined on failure.
 */
JS_FRIEND_API JS::UniqueChars GetCodeCoverageSummary(JSContext* cx,
                                                     size_t* length);
JS_FRIEND_API JS::UniqueChars GetCodeCoverageSummaryAll(JSContext* cx,
                                                        size_t* length);

using DOMInstanceClassHasProtoAtDepth = bool (*)(const JSClass*, uint32_t,
                                                 uint32_t);
struct JSDOMCallbacks {
  DOMInstanceClassHasProtoAtDepth instanceClassMatchesProto;
};
using DOMCallbacks = struct JSDOMCallbacks;

extern JS_FRIEND_API void SetDOMCallbacks(JSContext* cx,
                                          const DOMCallbacks* callbacks);

extern JS_FRIEND_API const DOMCallbacks* GetDOMCallbacks(JSContext* cx);

extern JS_FRIEND_API JSObject* GetTestingFunctions(JSContext* cx);

/* Implemented in jsexn.cpp. */

/**
 * Get an error type name from a JSExnType constant.
 * Returns nullptr for invalid arguments and JSEXN_INTERNALERR
 */
extern JS_FRIEND_API JSLinearString* GetErrorTypeName(JSContext* cx,
                                                      int16_t exnType);

extern JS_FRIEND_API RegExpShared* RegExpToSharedNonInline(
    JSContext* cx, JS::HandleObject regexp);

/* Implemented in CrossCompartmentWrapper.cpp. */
typedef enum NukeReferencesToWindow {
  NukeWindowReferences,
  DontNukeWindowReferences
} NukeReferencesToWindow;

typedef enum NukeReferencesFromTarget {
  NukeAllReferences,
  NukeIncomingReferences,
} NukeReferencesFromTarget;

/*
 * These filters are designed to be ephemeral stack classes, and thus don't
 * do any rooting or holding of their members.
 */
struct CompartmentFilter {
  virtual bool match(JS::Compartment* c) const = 0;
};

struct AllCompartments : public CompartmentFilter {
  virtual bool match(JS::Compartment* c) const override { return true; }
};

struct ContentCompartmentsOnly : public CompartmentFilter {
  virtual bool match(JS::Compartment* c) const override {
    return !IsSystemCompartment(c);
  }
};

struct ChromeCompartmentsOnly : public CompartmentFilter {
  virtual bool match(JS::Compartment* c) const override {
    return IsSystemCompartment(c);
  }
};

struct SingleCompartment : public CompartmentFilter {
  JS::Compartment* ours;
  explicit SingleCompartment(JS::Compartment* c) : ours(c) {}
  virtual bool match(JS::Compartment* c) const override { return c == ours; }
};

extern JS_FRIEND_API bool NukeCrossCompartmentWrappers(
    JSContext* cx, const CompartmentFilter& sourceFilter, JS::Realm* target,
    NukeReferencesToWindow nukeReferencesToWindow,
    NukeReferencesFromTarget nukeReferencesFromTarget);

extern JS_FRIEND_API bool AllowNewWrapper(JS::Compartment* target,
                                          JSObject* obj);

extern JS_FRIEND_API bool NukedObjectRealm(JSObject* obj);

/* Specify information about DOMProxy proxies in the DOM, for use by ICs. */

/*
 * The DOMProxyShadowsCheck function will be called to check if the property for
 * id should be gotten from the prototype, or if there is an own property that
 * shadows it.
 * * If ShadowsViaDirectExpando is returned, then the slot at
 *   listBaseExpandoSlot contains an expando object which has the property in
 *   question.
 * * If ShadowsViaIndirectExpando is returned, then the slot at
 *   listBaseExpandoSlot contains a private pointer to an ExpandoAndGeneration
 *   and the expando object in the ExpandoAndGeneration has the property in
 *   question.
 * * If DoesntShadow is returned then the slot at listBaseExpandoSlot should
 *   either be undefined or point to an expando object that would contain the
 *   own property.
 * * If DoesntShadowUnique is returned then the slot at listBaseExpandoSlot
 *   should contain a private pointer to a ExpandoAndGeneration, which contains
 *   a JS::Value that should either be undefined or point to an expando object,
 *   and a uint64 value. If that value changes then the IC for getting a
 *   property will be invalidated.
 * * If Shadows is returned, that means the property is an own property of the
 *   proxy but doesn't live on the expando object.
 */

struct ExpandoAndGeneration {
  ExpandoAndGeneration() : expando(JS::UndefinedValue()), generation(0) {}

  void OwnerUnlinked() { ++generation; }

  static size_t offsetOfExpando() {
    return offsetof(ExpandoAndGeneration, expando);
  }

  static size_t offsetOfGeneration() {
    return offsetof(ExpandoAndGeneration, generation);
  }

  JS::Heap<JS::Value> expando;
  uint64_t generation;
};

typedef enum DOMProxyShadowsResult {
  ShadowCheckFailed,
  Shadows,
  DoesntShadow,
  DoesntShadowUnique,
  ShadowsViaDirectExpando,
  ShadowsViaIndirectExpando
} DOMProxyShadowsResult;
using DOMProxyShadowsCheck = DOMProxyShadowsResult (*)(JSContext*,
                                                       JS::HandleObject,
                                                       JS::HandleId);
JS_FRIEND_API void SetDOMProxyInformation(
    const void* domProxyHandlerFamily,
    DOMProxyShadowsCheck domProxyShadowsCheck,
    const void* domRemoteProxyHandlerFamily);

const void* GetDOMProxyHandlerFamily();
DOMProxyShadowsCheck GetDOMProxyShadowsCheck();
inline bool DOMProxyIsShadowing(DOMProxyShadowsResult result) {
  return result == Shadows || result == ShadowsViaDirectExpando ||
         result == ShadowsViaIndirectExpando;
}

const void* GetDOMRemoteProxyHandlerFamily();

extern JS_FRIEND_API bool IsDOMRemoteProxyObject(JSObject* object);

// Callbacks and other information for use by the JITs when optimizing accesses
// on xray wrappers.
struct XrayJitInfo {
  // Test whether a proxy handler is a cross compartment xray with no
  // security checks.
  bool (*isCrossCompartmentXray)(const BaseProxyHandler* handler);

  // Test whether xrays in |obj|'s compartment have expandos of their own,
  // instead of sharing them with Xrays from other compartments.
  bool (*compartmentHasExclusiveExpandos)(JSObject* obj);

  // Proxy reserved slot used by xrays in sandboxes to store their holder
  // object.
  size_t xrayHolderSlot;

  // Reserved slot used by xray holders to store the xray's expando object.
  size_t holderExpandoSlot;

  // Reserved slot used by xray expandos to store a custom prototype.
  size_t expandoProtoSlot;
};

JS_FRIEND_API void SetXrayJitInfo(XrayJitInfo* info);

XrayJitInfo* GetXrayJitInfo();

/* Implemented in jsdate.cpp. */

/** Detect whether the internal date value is NaN. */
extern JS_FRIEND_API bool DateIsValid(JSContext* cx, JS::HandleObject obj,
                                      bool* isValid);

extern JS_FRIEND_API bool DateGetMsecSinceEpoch(JSContext* cx,
                                                JS::HandleObject obj,
                                                double* msecSinceEpoch);

} /* namespace js */

namespace js {

/* Implemented in vm/StructuredClone.cpp. */
extern JS_FRIEND_API uint64_t GetSCOffset(JSStructuredCloneWriter* writer);

namespace jit {

enum class InlinableNative : uint16_t;

}  // namespace jit

}  // namespace js

/**
 * A class, expected to be passed by value, which represents the CallArgs for a
 * JSJitGetterOp.
 */
class JSJitGetterCallArgs : protected JS::MutableHandleValue {
 public:
  explicit JSJitGetterCallArgs(const JS::CallArgs& args)
      : JS::MutableHandleValue(args.rval()) {}

  explicit JSJitGetterCallArgs(JS::RootedValue* rooted)
      : JS::MutableHandleValue(rooted) {}

  JS::MutableHandleValue rval() { return *this; }
};

/**
 * A class, expected to be passed by value, which represents the CallArgs for a
 * JSJitSetterOp.
 */
class JSJitSetterCallArgs : protected JS::MutableHandleValue {
 public:
  explicit JSJitSetterCallArgs(const JS::CallArgs& args)
      : JS::MutableHandleValue(args[0]) {}

  JS::MutableHandleValue operator[](unsigned i) {
    MOZ_ASSERT(i == 0);
    return *this;
  }

  unsigned length() const { return 1; }

  // Add get() or maybe hasDefined() as needed
};

struct JSJitMethodCallArgsTraits;

/**
 * A class, expected to be passed by reference, which represents the CallArgs
 * for a JSJitMethodOp.
 */
class JSJitMethodCallArgs
    : protected JS::detail::CallArgsBase<JS::detail::NoUsedRval> {
 private:
  using Base = JS::detail::CallArgsBase<JS::detail::NoUsedRval>;
  friend struct JSJitMethodCallArgsTraits;

 public:
  explicit JSJitMethodCallArgs(const JS::CallArgs& args) {
    argv_ = args.array();
    argc_ = args.length();
  }

  JS::MutableHandleValue rval() const { return Base::rval(); }

  unsigned length() const { return Base::length(); }

  JS::MutableHandleValue operator[](unsigned i) const {
    return Base::operator[](i);
  }

  bool hasDefined(unsigned i) const { return Base::hasDefined(i); }

  JSObject& callee() const {
    // We can't use Base::callee() because that will try to poke at
    // this->usedRval_, which we don't have.
    return argv_[-2].toObject();
  }

  JS::HandleValue get(unsigned i) const { return Base::get(i); }

  bool requireAtLeast(JSContext* cx, const char* fnname,
                      unsigned required) const {
    // Can just forward to Base, since it only needs the length and we
    // forward that already.
    return Base::requireAtLeast(cx, fnname, required);
  }
};

struct JSJitMethodCallArgsTraits {
  static const size_t offsetOfArgv = offsetof(JSJitMethodCallArgs, argv_);
  static const size_t offsetOfArgc = offsetof(JSJitMethodCallArgs, argc_);
};

using JSJitGetterOp = bool (*)(JSContext*, JS::HandleObject, void*,
                               JSJitGetterCallArgs);
using JSJitSetterOp = bool (*)(JSContext*, JS::HandleObject, void*,
                               JSJitSetterCallArgs);
using JSJitMethodOp = bool (*)(JSContext*, JS::HandleObject, void*,
                               const JSJitMethodCallArgs&);

/**
 * This struct contains metadata passed from the DOM to the JS Engine for JIT
 * optimizations on DOM property accessors. Eventually, this should be made
 * available to general JSAPI users, but we are not currently ready to do so.
 */
struct JSJitInfo {
  enum OpType {
    Getter,
    Setter,
    Method,
    StaticMethod,
    InlinableNative,
    IgnoresReturnValueNative,
    // Must be last
    OpTypeCount
  };

  enum ArgType {
    // Basic types
    String = (1 << 0),
    Integer = (1 << 1),  // Only 32-bit or less
    Double = (1 << 2),   // Maybe we want to add Float sometime too
    Boolean = (1 << 3),
    Object = (1 << 4),
    Null = (1 << 5),

    // And derived types
    Numeric = Integer | Double,
    // Should "Primitive" use the WebIDL definition, which
    // excludes string and null, or the typical JS one that includes them?
    Primitive = Numeric | Boolean | Null | String,
    ObjectOrNull = Object | Null,
    Any = ObjectOrNull | Primitive,

    // Our sentinel value.
    ArgTypeListEnd = (1 << 31)
  };

  static_assert(Any & String, "Any must include String.");
  static_assert(Any & Integer, "Any must include Integer.");
  static_assert(Any & Double, "Any must include Double.");
  static_assert(Any & Boolean, "Any must include Boolean.");
  static_assert(Any & Object, "Any must include Object.");
  static_assert(Any & Null, "Any must include Null.");

  /**
   * An enum that describes what this getter/setter/method aliases.  This
   * determines what things can be hoisted past this call, and if this
   * call is movable what it can be hoisted past.
   */
  enum AliasSet {
    /**
     * Alias nothing: a constant value, getting it can't affect any other
     * values, nothing can affect it.
     */
    AliasNone,

    /**
     * Alias things that can modify the DOM but nothing else.  Doing the
     * call can't affect the behavior of any other function.
     */
    AliasDOMSets,

    /**
     * Alias the world.  Calling this can change arbitrary values anywhere
     * in the system.  Most things fall in this bucket.
     */
    AliasEverything,

    /** Must be last. */
    AliasSetCount
  };

  bool needsOuterizedThisObject() const {
    return type() != Getter && type() != Setter;
  }

  bool isTypedMethodJitInfo() const { return isTypedMethod; }

  OpType type() const { return OpType(type_); }

  AliasSet aliasSet() const { return AliasSet(aliasSet_); }

  JSValueType returnType() const { return JSValueType(returnType_); }

  union {
    JSJitGetterOp getter;
    JSJitSetterOp setter;
    JSJitMethodOp method;
    /** A DOM static method, used for Promise wrappers */
    JSNative staticMethod;
    JSNative ignoresReturnValueMethod;
  };

  static unsigned offsetOfIgnoresReturnValueNative() {
    return offsetof(JSJitInfo, ignoresReturnValueMethod);
  }

  union {
    uint16_t protoID;
    js::jit::InlinableNative inlinableNative;
  };

  union {
    uint16_t depth;

    // Additional opcode for some InlinableNative functions.
    uint16_t nativeOp;
  };

  // These fields are carefully packed to take up 4 bytes.  If you need more
  // bits for whatever reason, please see if you can steal bits from existing
  // fields before adding more members to this structure.

#define JITINFO_OP_TYPE_BITS 4
#define JITINFO_ALIAS_SET_BITS 4
#define JITINFO_RETURN_TYPE_BITS 8
#define JITINFO_SLOT_INDEX_BITS 10

  /** The OpType that says what sort of function we are. */
  uint32_t type_ : JITINFO_OP_TYPE_BITS;

  /**
   * The alias set for this op.  This is a _minimal_ alias set; in
   * particular for a method it does not include whatever argument
   * conversions might do.  That's covered by argTypes and runtime
   * analysis of the actual argument types being passed in.
   */
  uint32_t aliasSet_ : JITINFO_ALIAS_SET_BITS;

  /** The return type tag.  Might be JSVAL_TYPE_UNKNOWN. */
  uint32_t returnType_ : JITINFO_RETURN_TYPE_BITS;

  static_assert(OpTypeCount <= (1 << JITINFO_OP_TYPE_BITS),
                "Not enough space for OpType");
  static_assert(AliasSetCount <= (1 << JITINFO_ALIAS_SET_BITS),
                "Not enough space for AliasSet");
  static_assert((sizeof(JSValueType) * 8) <= JITINFO_RETURN_TYPE_BITS,
                "Not enough space for JSValueType");

#undef JITINFO_RETURN_TYPE_BITS
#undef JITINFO_ALIAS_SET_BITS
#undef JITINFO_OP_TYPE_BITS

  /** Is op fallible? False in setters. */
  uint32_t isInfallible : 1;

  /**
   * Is op movable?  To be movable the op must
   * not AliasEverything, but even that might
   * not be enough (e.g. in cases when it can
   * throw or is explicitly not movable).
   */
  uint32_t isMovable : 1;

  /**
   * Can op be dead-code eliminated? Again, this
   * depends on whether the op can throw, in
   * addition to the alias set.
   */
  uint32_t isEliminatable : 1;

  // XXXbz should we have a JSValueType for the type of the member?
  /**
   * True if this is a getter that can always
   * get the value from a slot of the "this" object.
   */
  uint32_t isAlwaysInSlot : 1;

  /**
   * True if this is a getter that can sometimes (if the slot doesn't contain
   * UndefinedValue()) get the value from a slot of the "this" object.
   */
  uint32_t isLazilyCachedInSlot : 1;

  /** True if this is an instance of JSTypedMethodJitInfo. */
  uint32_t isTypedMethod : 1;

  /**
   * If isAlwaysInSlot or isSometimesInSlot is true,
   * the index of the slot to get the value from.
   * Otherwise 0.
   */
  uint32_t slotIndex : JITINFO_SLOT_INDEX_BITS;

  static const size_t maxSlotIndex = (1 << JITINFO_SLOT_INDEX_BITS) - 1;

#undef JITINFO_SLOT_INDEX_BITS
};

static_assert(sizeof(JSJitInfo) == (sizeof(void*) + 2 * sizeof(uint32_t)),
              "There are several thousand instances of JSJitInfo stored in "
              "a binary. Please don't increase its space requirements without "
              "verifying that there is no other way forward (better packing, "
              "smaller datatypes for fields, subclassing, etc.).");

struct JSTypedMethodJitInfo {
  // We use C-style inheritance here, rather than C++ style inheritance
  // because not all compilers support brace-initialization for non-aggregate
  // classes. Using C++ style inheritance and constructors instead of
  // brace-initialization would also force the creation of static
  // constructors (on some compilers) when JSJitInfo and JSTypedMethodJitInfo
  // structures are declared. Since there can be several thousand of these
  // structures present and we want to have roughly equivalent performance
  // across a range of compilers, we do things manually.
  JSJitInfo base;

  const JSJitInfo::ArgType* const argTypes; /* For a method, a list of sets of
                                               types that the function
                                               expects.  This can be used,
                                               for example, to figure out
                                               when argument coercions can
                                               have side-effects. */
};

namespace js {

static MOZ_ALWAYS_INLINE shadow::Function* FunctionObjectToShadowFunction(
    JSObject* fun) {
  MOZ_ASSERT(GetObjectClass(fun) == FunctionClassPtr);
  return reinterpret_cast<shadow::Function*>(fun);
}

/* Statically asserted in FunctionFlags.cpp. */
static const unsigned JS_FUNCTION_INTERPRETED_BITS = 0x0060;

// Return whether the given function object is native.
static MOZ_ALWAYS_INLINE bool FunctionObjectIsNative(JSObject* fun) {
  return !(FunctionObjectToShadowFunction(fun)->flags &
           JS_FUNCTION_INTERPRETED_BITS);
}

static MOZ_ALWAYS_INLINE JSNative GetFunctionObjectNative(JSObject* fun) {
  MOZ_ASSERT(FunctionObjectIsNative(fun));
  return FunctionObjectToShadowFunction(fun)->native;
}

}  // namespace js

static MOZ_ALWAYS_INLINE const JSJitInfo* FUNCTION_VALUE_TO_JITINFO(
    const JS::Value& v) {
  MOZ_ASSERT(js::FunctionObjectIsNative(&v.toObject()));
  return js::FunctionObjectToShadowFunction(&v.toObject())->jitinfo;
}

static MOZ_ALWAYS_INLINE void SET_JITINFO(JSFunction* func,
                                          const JSJitInfo* info) {
  js::shadow::Function* fun = reinterpret_cast<js::shadow::Function*>(func);
  MOZ_ASSERT(!(fun->flags & js::JS_FUNCTION_INTERPRETED_BITS));
  fun->jitinfo = info;
}

// All strings stored in jsids are atomized, but are not necessarily property
// names.
static MOZ_ALWAYS_INLINE bool JSID_IS_ATOM(jsid id) {
  return JSID_IS_STRING(id);
}

static MOZ_ALWAYS_INLINE bool JSID_IS_ATOM(jsid id, JSAtom* atom) {
  return id == JS::PropertyKey::fromNonIntAtom(atom);
}

static MOZ_ALWAYS_INLINE JSAtom* JSID_TO_ATOM(jsid id) {
  return (JSAtom*)JSID_TO_STRING(id);
}

static_assert(sizeof(jsid) == sizeof(void*));

namespace js {

static MOZ_ALWAYS_INLINE JS::Value IdToValue(jsid id) {
  if (JSID_IS_STRING(id)) {
    return JS::StringValue(JSID_TO_STRING(id));
  }
  if (JSID_IS_INT(id)) {
    return JS::Int32Value(JSID_TO_INT(id));
  }
  if (JSID_IS_SYMBOL(id)) {
    return JS::SymbolValue(JSID_TO_SYMBOL(id));
  }
  MOZ_ASSERT(JSID_IS_VOID(id));
  return JS::UndefinedValue();
}

/**
 * PrepareScriptEnvironmentAndInvoke asserts the embedder has registered a
 * ScriptEnvironmentPreparer and then it calls the preparer's 'invoke' method
 * with the given |closure|, with the assumption that the preparer will set up
 * any state necessary to run script in |global|, invoke |closure| with a valid
 * JSContext*, report any exceptions thrown from the closure, and return.
 *
 * PrepareScriptEnvironmentAndInvoke will report any exceptions that are thrown
 * by the closure.  Consumers who want to propagate back whether the closure
 * succeeded should do so via members of the closure itself.
 */

struct ScriptEnvironmentPreparer {
  struct Closure {
    virtual bool operator()(JSContext* cx) = 0;
  };

  virtual void invoke(JS::HandleObject global, Closure& closure) = 0;
};

extern JS_FRIEND_API void PrepareScriptEnvironmentAndInvoke(
    JSContext* cx, JS::HandleObject global,
    ScriptEnvironmentPreparer::Closure& closure);

JS_FRIEND_API void SetScriptEnvironmentPreparer(
    JSContext* cx, ScriptEnvironmentPreparer* preparer);

enum CTypesActivityType {
  CTYPES_CALL_BEGIN,
  CTYPES_CALL_END,
  CTYPES_CALLBACK_BEGIN,
  CTYPES_CALLBACK_END
};

using CTypesActivityCallback = void (*)(JSContext*, CTypesActivityType);

/**
 * Sets a callback that is run whenever js-ctypes is about to be used when
 * calling into C.
 */
JS_FRIEND_API void SetCTypesActivityCallback(JSContext* cx,
                                             CTypesActivityCallback cb);

class MOZ_RAII JS_FRIEND_API AutoCTypesActivityCallback {
 private:
  JSContext* cx;
  CTypesActivityCallback callback;
  CTypesActivityType endType;

 public:
  AutoCTypesActivityCallback(JSContext* cx, CTypesActivityType beginType,
                             CTypesActivityType endType);
  ~AutoCTypesActivityCallback() { DoEndCallback(); }
  void DoEndCallback() {
    if (callback) {
      callback(cx, endType);
      callback = nullptr;
    }
  }
};

// Abstract base class for objects that build allocation metadata for JavaScript
// values.
struct AllocationMetadataBuilder {
  AllocationMetadataBuilder() = default;

  // Return a metadata object for the newly constructed object |obj|, or
  // nullptr if there's no metadata to attach.
  //
  // Implementations should treat all errors as fatal; there is no way to
  // report errors from this callback. In particular, the caller provides an
  // oomUnsafe for overriding implementations to use.
  virtual JSObject* build(JSContext* cx, JS::HandleObject obj,
                          AutoEnterOOMUnsafeRegion& oomUnsafe) const {
    return nullptr;
  }
};

/**
 * Specify a callback to invoke when creating each JS object in the current
 * compartment, which may return a metadata object to associate with the
 * object.
 */
JS_FRIEND_API void SetAllocationMetadataBuilder(
    JSContext* cx, const AllocationMetadataBuilder* callback);

/** Get the metadata associated with an object. */
JS_FRIEND_API JSObject* GetAllocationMetadata(JSObject* obj);

JS_FRIEND_API bool GetElementsWithAdder(JSContext* cx, JS::HandleObject obj,
                                        JS::HandleObject receiver,
                                        uint32_t begin, uint32_t end,
                                        js::ElementAdder* adder);

JS_FRIEND_API bool ForwardToNative(JSContext* cx, JSNative native,
                                   const JS::CallArgs& args);

/**
 * Helper function for HTMLDocument and HTMLFormElement.
 *
 * These are the only two interfaces that have [OverrideBuiltins], a named
 * getter, and no named setter. They're implemented as proxies with a custom
 * getOwnPropertyDescriptor() method. Unfortunately, overriding
 * getOwnPropertyDescriptor() automatically affects the behavior of set(),
 * which normally is just common sense but is *not* desired for these two
 * interfaces.
 *
 * The fix is for these two interfaces to override set() to ignore the
 * getOwnPropertyDescriptor() override.
 *
 * SetPropertyIgnoringNamedGetter is exposed to make it easier to override
 * set() in this way.  It carries out all the steps of BaseProxyHandler::set()
 * except the initial getOwnPropertyDescriptor() call.  The caller must supply
 * that descriptor as the 'ownDesc' parameter.
 *
 * Implemented in proxy/BaseProxyHandler.cpp.
 */
JS_FRIEND_API bool SetPropertyIgnoringNamedGetter(
    JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v,
    JS::HandleValue receiver, JS::Handle<JS::PropertyDescriptor> ownDesc,
    JS::ObjectOpResult& result);

// This function is for one specific use case, please don't use this for
// anything else!
extern JS_FRIEND_API bool ExecuteInFrameScriptEnvironment(
    JSContext* cx, JS::HandleObject obj, JS::HandleScript script,
    JS::MutableHandleObject scope);

// These functions are provided for the JSM component loader in Gecko.
//
// A 'JSMEnvironment' refers to an environment chain constructed for JSM loading
// in a shared global. Internally it is a NonSyntacticVariablesObject with a
// corresponding extensible LexicalEnvironmentObject that is accessible by
// JS_ExtensibleLexicalEnvironment. The |this| value of that lexical environment
// is the NSVO itself.
//
// Normal global environment (ES6):     JSM "global" environment:
//
//                                      * - extensible lexical environment
//                                      |   (code runs in this environment;
//                                      |    `let/const` bindings go here)
//                                      |
//                                      * - JSMEnvironment (=== `this`)
//                                      |   (`var` bindings go here)
//                                      |
// * - extensible lexical environment   * - extensible lexical environment
// |   (code runs in this environment;  |   (empty)
// |    `let/const` bindings go here)   |
// |                                    |
// * - actual global (=== `this`)       * - shared JSM global
//     (var bindings go here; and           (Object, Math, etc. live here)
//      Object, Math, etc. live here)

// Allocate a new environment in current compartment that is compatible with JSM
// shared loading.
extern JS_FRIEND_API JSObject* NewJSMEnvironment(JSContext* cx);

// Execute the given script (copied into compartment if necessary) in the given
// JSMEnvironment. The script must have been compiled for hasNonSyntacticScope.
// The |jsmEnv| must have been previously allocated by NewJSMEnvironment.
//
// NOTE: The associated extensible lexical environment is reused.
extern JS_FRIEND_API bool ExecuteInJSMEnvironment(JSContext* cx,
                                                  JS::HandleScript script,
                                                  JS::HandleObject jsmEnv);

// Additionally, target objects may be specified as required by the Gecko
// subscript loader. These are wrapped in non-syntactic WithEnvironments and
// temporarily placed on environment chain.
//
// See also: JS::CloneAndExecuteScript(...)
extern JS_FRIEND_API bool ExecuteInJSMEnvironment(
    JSContext* cx, JS::HandleScript script, JS::HandleObject jsmEnv,
    JS::HandleObjectVector targetObj);

// Used by native methods to determine the JSMEnvironment of caller if possible
// by looking at stack frames. Returns nullptr if top frame isn't a scripted
// caller in a JSM.
//
// NOTE: This may find NonSyntacticVariablesObject generated by other embedding
// such as a Gecko FrameScript. Caller can check the compartment if needed.
extern JS_FRIEND_API JSObject* GetJSMEnvironmentOfScriptedCaller(JSContext* cx);

// Determine if obj is a JSMEnvironment
//
// NOTE: This may return true for an NonSyntacticVariablesObject generated by
// other embedding such as a Gecko FrameScript. Caller can check compartment.
extern JS_FRIEND_API bool IsJSMEnvironment(JSObject* obj);

extern JS_FRIEND_API bool IsSavedFrame(JSObject* obj);

// Matches the condition in js/src/jit/ProcessExecutableMemory.cpp
#if defined(XP_WIN)
// Parameters use void* types to avoid #including windows.h. The return value of
// this function is returned from the exception handler.
typedef long (*JitExceptionHandler)(void* exceptionRecord,  // PEXECTION_RECORD
                                    void* context);         // PCONTEXT

/**
 * Windows uses "structured exception handling" to handle faults. When a fault
 * occurs, the stack is searched for a handler (similar to C++ exception
 * handling). If the search does not find a handler, the "unhandled exception
 * filter" is called. Breakpad uses the unhandled exception filter to do crash
 * reporting. Unfortunately, on Win64, JIT code on the stack completely throws
 * off this unwinding process and prevents the unhandled exception filter from
 * being called. The reason is that Win64 requires unwind information be
 * registered for all code regions and JIT code has none. While it is possible
 * to register full unwind information for JIT code, this is a lot of work (one
 * has to be able to recover the frame pointer at any PC) so instead we register
 * a handler for all JIT code that simply calls breakpad's unhandled exception
 * filter (which will perform crash reporting and then terminate the process).
 * This would be wrong if there was an outer __try block that expected to handle
 * the fault, but this is not generally allowed.
 *
 * Gecko must call SetJitExceptionFilter before any JIT code is compiled and
 * only once per process.
 */
extern JS_FRIEND_API void SetJitExceptionHandler(JitExceptionHandler handler);
#endif

extern JS_FRIEND_API bool ReportIsNotFunction(JSContext* cx, JS::HandleValue v);

extern JS_FRIEND_API JSObject* ConvertArgsToArray(JSContext* cx,
                                                  const JS::CallArgs& args);

// Create and add the Intl.MozDateTimeFormat constructor function to the
// provided object.
// If JS was built without JS_HAS_INTL_API, this function will throw an
// exception.
//
// This custom date/time formatter constructor gives users the ability
// to specify a custom format pattern. This pattern is passed *directly*
// to ICU with NO SYNTAX PARSING OR VALIDATION WHATSOEVER. ICU appears to
// have a a modicum of testing of this, and it won't fall over completely
// if passed bad input. But the current behavior is entirely under-specified
// and emphatically not shippable on the web, and it *must* be fixed before
// this functionality can be exposed in the real world. (There are also some
// questions about whether the format exposed here is the *right* one to
// standardize, that will also need to be resolved to ship this.)
extern bool AddMozDateTimeFormatConstructor(JSContext* cx,
                                            JS::Handle<JSObject*> intl);

// Create and add the Intl.MozDisplayNames constructor function to the
// provided object.
// If JS was built without JS_HAS_INTL_API, this function will throw an
// exception.
extern bool AddMozDisplayNamesConstructor(JSContext* cx,
                                          JS::Handle<JSObject*> intl);

// Create and add the Intl.DisplayNames constructor function to the provided
// object.
// If JS was built without JS_HAS_INTL_API, this function will throw an
// exception.
extern bool AddDisplayNamesConstructor(JSContext* cx,
                                       JS::Handle<JSObject*> intl);

class MOZ_STACK_CLASS JS_FRIEND_API AutoAssertNoContentJS {
 public:
  explicit AutoAssertNoContentJS(JSContext* cx);
  ~AutoAssertNoContentJS();

 private:
  JSContext* context_;
  bool prevAllowContentJS_;
};

// Turn on assertions so that we assert that
//     !realm->validAccessPtr || *realm->validAccessPtr
// is true for every |realm| that we run JS code in. The realm's validAccessPtr
// is set via SetRealmValidAccessPtr.
extern JS_FRIEND_API void EnableAccessValidation(JSContext* cx, bool enabled);

// See EnableAccessValidation above. The caller must guarantee that accessp will
// live at least as long as |global| is alive. The JS engine reads accessp from
// threads that are allowed to run code on |global|, so all changes to *accessp
// should be made from whichever thread owns |global| at a given time.
extern JS_FRIEND_API void SetRealmValidAccessPtr(JSContext* cx,
                                                 JS::HandleObject global,
                                                 bool* accessp);

// Returns true if the system zone is available (i.e., if no cooperative
// contexts are using it now).
extern JS_FRIEND_API bool SystemZoneAvailable(JSContext* cx);

using LogCtorDtor = void (*)(void*, const char*, uint32_t);

/**
 * Set global function used to monitor a few internal classes to highlight
 * leaks, and to hint at the origin of the leaks.
 */
extern JS_FRIEND_API void SetLogCtorDtorFunctions(LogCtorDtor ctor,
                                                  LogCtorDtor dtor);

extern JS_FRIEND_API void LogCtor(void* self, const char* type, uint32_t sz);

extern JS_FRIEND_API void LogDtor(void* self, const char* type, uint32_t sz);

#define JS_COUNT_CTOR(Class) LogCtor((void*)this, #Class, sizeof(Class))

#define JS_COUNT_DTOR(Class) LogDtor((void*)this, #Class, sizeof(Class))

/**
 * This function only reports GC heap memory,
 * and not malloc allocated memory associated with GC things.
 */
extern JS_FRIEND_API uint64_t GetGCHeapUsageForObjectZone(JSObject* obj);

/**
 * Return whether a global object's realm has had instrumentation enabled by a
 * Debugger.
 */
extern JS_FRIEND_API bool GlobalHasInstrumentation(JSObject* global);

class JS_FRIEND_API CompartmentTransplantCallback {
 public:
  virtual JSObject* getObjectToTransplant(JS::Compartment* compartment) = 0;
};

// Gather a set of remote window proxies by calling the callback on every
// compartment, then transform them into cross-compartment wrappers to newTarget
// via brain transplants. If there's a proxy in newTarget's compartment, it will
// get swapped with newTarget, and the value of newTarget will be updated. If
// the callback returns null for a compartment, no cross-compartment wrapper
// will be created for that compartment. Any non-null values it returns must be
// DOM remote proxies from the compartment that was passed in.
extern JS_FRIEND_API void RemapRemoteWindowProxies(
    JSContext* cx, CompartmentTransplantCallback* callback,
    JS::MutableHandleObject newTarget);

namespace gc {

// API to let the DOM tell us whether we're currently in pageload, so we can
// change the GC triggers to discourage collection of the atoms zone.
//
// This is a temporary measure; bug 1544117 will make this unnecessary.

enum class PerformanceHint { Normal, InPageLoad };

extern JS_FRIEND_API void SetPerformanceHint(JSContext* cx,
                                             PerformanceHint hint);

} /* namespace gc */

extern JS_FRIEND_API JS::Zone* GetObjectZoneFromAnyThread(const JSObject* obj);

} /* namespace js */

#endif /* jsfriendapi_h */
