/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSHTMLDetailsElement.h"

#include "HTMLDetailsElement.h"
#include "HTMLNames.h"
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSHTMLDetailsElement);

/* Hash table */
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const HashTableValue JSHTMLDetailsElementTableValues[3] =
{
    { "open", DontDelete, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsHTMLDetailsElementOpen), (intptr_t)setJSHTMLDetailsElementOpen THUNK_GENERATOR(0) },
    { "constructor", DontEnum | ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsHTMLDetailsElementConstructor), (intptr_t)0 THUNK_GENERATOR(0) },
    { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
static JSC_CONST_HASHTABLE HashTable JSHTMLDetailsElementTable = { 4, 3, JSHTMLDetailsElementTableValues, 0 };
/* Hash table for constructor */
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const HashTableValue JSHTMLDetailsElementConstructorTableValues[1] =
{
    { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
static JSC_CONST_HASHTABLE HashTable JSHTMLDetailsElementConstructorTable = { 1, 0, JSHTMLDetailsElementConstructorTableValues, 0 };
class JSHTMLDetailsElementConstructor : public DOMConstructorObject {
public:
    JSHTMLDetailsElementConstructor(JSC::ExecState*, JSC::Structure*, JSDOMGlobalObject*);

    virtual bool getOwnPropertySlot(JSC::ExecState*, const JSC::Identifier&, JSC::PropertySlot&);
    virtual bool getOwnPropertyDescriptor(JSC::ExecState*, const JSC::Identifier&, JSC::PropertyDescriptor&);
    static const JSC::ClassInfo s_info;
    static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype)
    {
        return JSC::Structure::create(globalData, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }
protected:
    static const unsigned StructureFlags = JSC::OverridesGetOwnPropertySlot | JSC::ImplementsHasInstance | DOMConstructorObject::StructureFlags;
};

const ClassInfo JSHTMLDetailsElementConstructor::s_info = { "HTMLDetailsElementConstructor", &DOMConstructorObject::s_info, &JSHTMLDetailsElementConstructorTable, 0 };

JSHTMLDetailsElementConstructor::JSHTMLDetailsElementConstructor(ExecState* exec, Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorObject(structure, globalObject)
{
    ASSERT(inherits(&s_info));
    putDirect(exec->globalData(), exec->propertyNames().prototype, JSHTMLDetailsElementPrototype::self(exec, globalObject), DontDelete | ReadOnly);
}

bool JSHTMLDetailsElementConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSHTMLDetailsElementConstructor, JSDOMWrapper>(exec, &JSHTMLDetailsElementConstructorTable, this, propertyName, slot);
}

bool JSHTMLDetailsElementConstructor::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<JSHTMLDetailsElementConstructor, JSDOMWrapper>(exec, &JSHTMLDetailsElementConstructorTable, this, propertyName, descriptor);
}

/* Hash table for prototype */
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const HashTableValue JSHTMLDetailsElementPrototypeTableValues[1] =
{
    { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
static JSC_CONST_HASHTABLE HashTable JSHTMLDetailsElementPrototypeTable = { 1, 0, JSHTMLDetailsElementPrototypeTableValues, 0 };
const ClassInfo JSHTMLDetailsElementPrototype::s_info = { "HTMLDetailsElementPrototype", &JSC::JSObjectWithGlobalObject::s_info, &JSHTMLDetailsElementPrototypeTable, 0 };

JSObject* JSHTMLDetailsElementPrototype::self(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSHTMLDetailsElement>(exec, globalObject);
}

const ClassInfo JSHTMLDetailsElement::s_info = { "HTMLDetailsElement", &JSHTMLElement::s_info, &JSHTMLDetailsElementTable, 0 };

JSHTMLDetailsElement::JSHTMLDetailsElement(Structure* structure, JSDOMGlobalObject* globalObject, PassRefPtr<HTMLDetailsElement> impl)
    : JSHTMLElement(structure, globalObject, impl)
{
    ASSERT(inherits(&s_info));
}

JSObject* JSHTMLDetailsElement::createPrototype(ExecState* exec, JSGlobalObject* globalObject)
{
    return new (exec) JSHTMLDetailsElementPrototype(exec->globalData(), globalObject, JSHTMLDetailsElementPrototype::createStructure(exec->globalData(), JSHTMLElementPrototype::self(exec, globalObject)));
}

bool JSHTMLDetailsElement::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSHTMLDetailsElement, Base>(exec, &JSHTMLDetailsElementTable, this, propertyName, slot);
}

bool JSHTMLDetailsElement::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<JSHTMLDetailsElement, Base>(exec, &JSHTMLDetailsElementTable, this, propertyName, descriptor);
}

JSValue jsHTMLDetailsElementOpen(ExecState* exec, JSValue slotBase, const Identifier&)
{
    JSHTMLDetailsElement* castedThis = static_cast<JSHTMLDetailsElement*>(asObject(slotBase));
    UNUSED_PARAM(exec);
    HTMLDetailsElement* imp = static_cast<HTMLDetailsElement*>(castedThis->impl());
    JSValue result = jsBoolean(imp->hasAttribute(WebCore::HTMLNames::openAttr));
    return result;
}


JSValue jsHTMLDetailsElementConstructor(ExecState* exec, JSValue slotBase, const Identifier&)
{
    JSHTMLDetailsElement* domObject = static_cast<JSHTMLDetailsElement*>(asObject(slotBase));
    return JSHTMLDetailsElement::getConstructor(exec, domObject->globalObject());
}

void JSHTMLDetailsElement::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    lookupPut<JSHTMLDetailsElement, Base>(exec, propertyName, value, &JSHTMLDetailsElementTable, this, slot);
}

void setJSHTMLDetailsElementOpen(ExecState* exec, JSObject* thisObject, JSValue value)
{
    JSHTMLDetailsElement* castedThis = static_cast<JSHTMLDetailsElement*>(thisObject);
    HTMLDetailsElement* imp = static_cast<HTMLDetailsElement*>(castedThis->impl());
    imp->setBooleanAttribute(WebCore::HTMLNames::openAttr, value.toBoolean(exec));
}


JSValue JSHTMLDetailsElement::getConstructor(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSHTMLDetailsElementConstructor>(exec, static_cast<JSDOMGlobalObject*>(globalObject));
}


}