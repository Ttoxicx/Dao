#pragma once

#include "runtime/core/meta/json.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Dao {

#if defined(__REFLECTION_PARSER__)
#define META(...) __attribute__((annotate(#__VA_ARGS__)))
#define CLASS(class_name, ...) class __attribute__((annotate(#__VA_ARGS__))) class_name
#define STRUCT(struct_name, ...) struct __attribute__((annotate(#__VA_ARGS__))) struct_name
#else
#define META(...)
#define CLASS(class_name, ...) class class_name
#define STRUCT(struct_name, ...) struct struct_name
#endif // __REFLECTION_PARSER__

#define REFLECTION_BODY(class_name) \
    friend class Reflection::TypeFieldReflectionOparator::Type##class_name##Operator; \
    friend class Serializer;

#define REFLECTION_TYPE(class_name) \
    namespace Reflection \
    { \
        namespace TypeFieldReflectionOparator \
        { \
            class Type##class_name##Operator; \
        } \
    };

#define REGISTER_FIELD_TO_MAP(name, value) TypeMetaRegisterInterface::registerToFieldMap(name, value);
#define REGISTER_METHOD_TO_MAP(name, value) TypeMetaRegisterInterface::registerToMethodMap(name, value);
#define REGISTER_BASE_CLASS_TO_MAP(name, value) TypeMetaRegisterInterface::registerToClassMap(name, value);
#define REGISTER_ARRAY_TO_MAP(name, value) TypeMetaRegisterInterface::registerToArrayMap(name, value);
#define UNREGISTER_ALL TypeMetaRegisterInterface::unregisterAll();

#define DAO_REFLECTION_NEW(name, ...) Reflection::ReflectionPtr(#name, new name(__VA_ARGS__));
#define DAO_REFLECTION_DELETE(value) \
    if (value) \
    { \
        delete value.operator->(); \
        value.getPtrReference() = nullptr; \
    }
#define DAO_REFLECTION_DEEP_COPY(type, dst_ptr, src_ptr) \
    *static_cast<type*>(dst_ptr) = *static_cast<type*>(src_ptr.getPtr());

#define TypeMetaDef(class_name, ptr) \
    Dao::Reflection::ReflectionInstance(Dao::Reflection::TypeMeta::newMetaFromName(#class_name), \
                                            (class_name*)ptr)

#define TypeMetaDefPtr(class_name, ptr) \
    new Dao::Reflection::ReflectionInstance(Dao::Reflection::TypeMeta::newMetaFromName(#class_name), \
                                                (class_name*)ptr)

    template<typename T,typename U,typename = void>
    struct is_safely_castable :std::false_type {};

    template<typename T,typename U>
    struct is_safely_castable<T, U, std::void_t<decltype(static_cast<U>(std::declval<T>()))>> :std::true_type {};

    namespace Reflection {
        class TypeMeta;
        class FieldAccessor;
        class MethodAccessor;
        class ArrayAccessor;
        class ReflectionInstance;
    }

    typedef std::function<void(void*, void*)>       SetFunction;
    typedef std::function<void* (void*)>            GetFunction;
    typedef std::function<const char* ()>           GetNameFunction;
    typedef std::function<void(int, void*, void*)>  SetArrayFunction;
    typedef std::function<void* (int, void*)>       GetArrayFunction;
    typedef std::function<int(void*)>               GetSizeFunction;
    typedef std::function<bool()>                   GetBoolFunction;
    typedef std::function<void(void*)>              InvokeFunction;

    typedef std::function<void* (const Json&)>                          ConstructorWithJson;
    typedef std::function<Json(void*)>                                  WriteJsonByName;
    typedef std::function<int(Reflection::ReflectionInstance*&, void*)> GetBaseClassReflectionInstanceListFunction;

    typedef std::tuple<SetFunction, GetFunction, GetNameFunction, GetNameFunction, GetNameFunction, GetBoolFunction>    FieldFunctionTuple;
    typedef std::tuple<GetNameFunction, InvokeFunction>                                                                 MethodFunctionTuple;
    typedef std::tuple<GetBaseClassReflectionInstanceListFunction, ConstructorWithJson, WriteJsonByName>                ClassFunctionTuple;
    typedef std::tuple<SetArrayFunction, GetArrayFunction, GetSizeFunction, GetNameFunction, GetNameFunction>           ArrayFunctionTuple;

    namespace Reflection {

        class TypeMetaRegisterInterface {
        public:
            static void registerToClassMap(const char* name, ClassFunctionTuple* value);
            static void registerToFieldMap(const char* name, FieldFunctionTuple* value);
            static void registerToMethodMap(const char* name, MethodFunctionTuple* value);
            static void registerToArrayMap(const char* name, ArrayFunctionTuple* value);
            static void unregisterAll();
        };

        class TypeMeta {
            friend class FieldAccessor;
            friend class ArrayAccessor;
            friend class TypeMetaRegisterInterface;
        public:
            TypeMeta();

            static TypeMeta newMetaFromName(std::string type_name);
            static bool newArrayAccessorFromName(std::string array_type_name, ArrayAccessor& accessor);
            static ReflectionInstance newFromNameAndJson(std::string type_name, const Json& json_context);
            static Json writeByName(std::string type_name, void* instance);

            std::string getTypeName();
            int getFieldsList(FieldAccessor*& out_list);
            int getMethodsList(MethodAccessor*& out_list);
            int getBaseClassReflectionInstanceList(ReflectionInstance*& out_list, void* instance);
            
            FieldAccessor getFieldByName(const char* name);
            MethodAccessor getMethodByName(const char* name);

            bool isValid() { return _is_valid; }

            TypeMeta& operator=(const TypeMeta& dest);

        private:
            TypeMeta(std::string type_name);

        private:
            std::vector<FieldAccessor, std::allocator<FieldAccessor>>   _fields;
            std::vector<MethodAccessor, std::allocator<MethodAccessor>> _methods;
            std::string                                                 _type_name;
            bool                                                        _is_valid;
        };

        class FieldAccessor {
            friend class TypeMeta;
        public:
            FieldAccessor();

            void* get(void* instance);
            void set(void* instance, void* value);
            
            TypeMeta getOwnerTypeMeta();

            bool getTypeMeta(TypeMeta& field_type);
            const char* getFieldName() const;
            const char* getFieldTypeName();
            bool isArrayType();

            FieldAccessor& operator=(const FieldAccessor& dest);

        private:
            FieldAccessor(FieldFunctionTuple* functions);

        private:
            FieldFunctionTuple*     _functions;
            const char*             _field_name;
            const char*             _field_type_name;
        };

        class MethodAccessor {
            friend class TypeMeta;

        public:
            MethodAccessor();

            void invoke(void* instance);

            const char* getMethodName() const;

            MethodAccessor& operator=(const MethodAccessor& dest);

        private:
            MethodAccessor(MethodFunctionTuple* functions);

        private:
            MethodFunctionTuple*    _functions;
            const char*             _method_name;
        };

        class ArrayAccessor {
            friend class TypeMeta;

        public:
            ArrayAccessor();
            const char* getArrayTypeName();
            const char* getElementTypeName();

            void set(int index, void* instance, void* element_value);
            void* get(int index, void* instance);
            int getSize(void* instance);

            ArrayAccessor& operator=(ArrayAccessor& dest);

        private:
            ArrayAccessor(ArrayFunctionTuple* functions);

        private:
            ArrayFunctionTuple* _functions;
            const char*         _array_type_name;
            const char*         _element_type_name;
        };

        class ReflectionInstance {
        public:
            ReflectionInstance(TypeMeta meta, void* instance) :_meta(meta), _instance(instance) {};
            ReflectionInstance() :_meta(), _instance(nullptr) {}

            ReflectionInstance& operator=(ReflectionInstance& dest);
            ReflectionInstance& operator=(ReflectionInstance&& dest);

        private:
            TypeMeta    _meta;
            void*       _instance;
        };

        template<typename T>
        class ReflectionPtr {
            template<typename U>
            friend class ReflectionPtr;

        public:
            ReflectionPtr(std::string type_name, T* instance) :_type_name(type_name), _instance(instance) {}
            ReflectionPtr() :_type_name(), _instance(nullptr) {};
            ReflectionPtr(const ReflectionPtr& dest) :_type_name(dest._type_name), _instance(dest._instance) {}

            template<typename U>
            ReflectionPtr<T>& operator=(const ReflectionPtr<U>& dest) {
                if (this == static_cast<void*>(&dest)) {
                    return *this;
                }
                _type_name = dest._type_name;
                _instance = static_cast<T*>(dest._instance);
                return *this;
            }

            template<typename U>
            ReflectionPtr<T>& operator=(ReflectionPtr<U>&& dest) {
                if (this == static_cast<void*>(&dest)) {
                    return *this;
                }
                _type_name = dest._type_name;
                _instance = static_cast<T*>(dest._instance);
                return *this;
            }

            ReflectionPtr<T>& operator=(const ReflectionPtr<T>& dest) {
                if (this == &dest) {
                    return *this;
                }
                _type_name = dest._type_name;
                _instance = dest._instance;
                return *this;
            }

            ReflectionPtr<T>& operator=(const ReflectionPtr<T>&& dest) {
                if (this == &dest) {
                    return *this;
                }
                _type_name = dest._type_name;
                _instance = dest._instance;
                return *this;
            }

            std::string getTypeName() const { return _type_name; }
            void setTypeName(std::string name) { _type_name = name; }

            bool operator==(const T* ptr) const { return _instance == ptr; }
            bool operator!=(const T* ptr) const { return _instance != ptr; }
            bool operator==(const ReflectionPtr<T>& rhs_ptr) const { return _instance == rhs_ptr.ptr; }
            bool operator!=(const ReflectionPtr<T>& rhs_ptr) const { return _instance != rhs_ptr.ptr; }

            template<typename T1>
            explicit operator T1* () {
                return static_cast<T1*>(_instance);
            }

            template<typename T1>
            explicit operator ReflectionPtr<T1>() {
                return ReflectionPtr<T1>(_type_name, (T1*)(_instance));
            }

            template<typename T1>
            explicit operator const T1* () const {
                return static_cast<T1*>(_instance);
            }

            template<typename T1>
            explicit operator const ReflectionPtr<T1>() const {
                return ReflectionPtr<T1>(_type_name, (T1*)(_instance));
            }

            T* operator->() { return _instance; }
            T* operator->() const { return _instance; }
            T& operator*() { return *(_instance); }

            T* getPtr() { return _instance; }
            T* getPtr() const { return _instance; }

            const T& operator*() const { return *(static_cast<const T*>(_instance)); }
            T*& getPtrReference() { return _instance; }

            operator bool() const { return _instance != nullptr; }

        private:
            std::string _type_name{ "" };
            typedef T   _type;
            T*          _instance{ nullptr };
        };
    }
}