#include "reflection.h"

#include <cstring>
#include <map>

namespace Dao {
	
	namespace Reflection {
		const char* k_unknown_type = "UnknownType";
		const char* k_unknown = "Unknown";
		static std::map<std::string, ClassFunctionTuple*>	m_class_map;
		static std::map<std::string, FieldFunctionTuple*>	m_field_map;
		static std::map<std::string, MethodFunctionTuple*>	m_method_map;
		static std::map<std::string, ArrayFunctionTuple*>	m_array_map;

		void TypeMetaRegisterInterface::registerToClassMap(const char* name, ClassFunctionTuple* value) {
			if (m_class_map.find(name) == m_class_map.end()) {
				m_class_map.insert(std::make_pair(name, value));
			}
			else {
				delete(value);
			}
		}

		void TypeMetaRegisterInterface::registerToFieldMap(const char* name, FieldFunctionTuple* value) {
			m_field_map.insert(std::make_pair(name, value));
		}

		void TypeMetaRegisterInterface::registerToMethodMap(const char* name, MethodFunctionTuple* value) {
			m_method_map.insert(std::make_pair(name, value));
		}

		void TypeMetaRegisterInterface::registerToArrayMap(const char* name, ArrayFunctionTuple* value) {
			if (m_array_map.find(name) == m_array_map.end()) {
				m_array_map.insert(std::make_pair(name, value));
			}
			else {
				delete(value);
			}
		}

		void TypeMetaRegisterInterface::unregisterAll() {
			for (const auto& itr : m_class_map) {
				delete(itr.second);
			}
			m_class_map.clear();
			for (const auto& itr : m_field_map) {
				delete(itr.second);
			}
			m_field_map.clear();
			for (const auto& itr : m_method_map) {
				delete(itr.second);
			}
			m_method_map.clear();
			for (const auto& itr : m_array_map) {
				delete(itr.second);
			}
			m_array_map.clear();
		}

		TypeMeta::TypeMeta(std::string type_name) :_type_name(type_name) {
			_is_valid = false;
			auto fields_itr = m_field_map.equal_range(type_name);
			while (fields_itr.first != fields_itr.second) {
				FieldAccessor f_field(fields_itr.first->second);
				_fields.emplace_back(f_field);
				_is_valid = true;
				++fields_itr.first;
			}
			auto method_itr = m_method_map.equal_range(type_name);
			while (method_itr.first != method_itr.second)
			{
				MethodAccessor f_method(method_itr.first->second);
				_methods.emplace_back(f_method);
				_is_valid = true;
				++method_itr.first;
			}
		}
		TypeMeta::TypeMeta() :_type_name(k_unknown_type), _is_valid(false) {
			_fields.clear();
			_methods.clear();
		}

		TypeMeta TypeMeta::newMetaFromName(std::string type_name) {
			return TypeMeta{ type_name };
		}

		bool TypeMeta::newArrayAccessorFromName(std::string array_type_name, ArrayAccessor& accessor) {
			auto itr = m_array_map.find(array_type_name);
			if (itr != m_array_map.end()) {
				accessor = ArrayAccessor{ itr->second };
				return true;
			}
			return false;
		}

		ReflectionInstance TypeMeta::newFromNameAndJson(std::string type_name, const Json& json_context) {
			auto itr = m_class_map.find(type_name);
			if (itr != m_class_map.end()) {
				return ReflectionInstance(TypeMeta{ type_name }, (std::get<1>(*itr->second)(json_context)));
			}
			return ReflectionInstance();
		}

		Json TypeMeta::writeByName(std::string type_name, void* instance) {
			auto itr = m_class_map.find(type_name);
			if (itr != m_class_map.end()) {
				return std::get<2>(*itr->second)(instance);
			}
			return Json{};
		}

		std::string TypeMeta::getTypeName() {
			return _type_name;
		}

		int TypeMeta::getFieldsList(FieldAccessor*& out_list) {
			int count = _fields.size();
			out_list = new FieldAccessor[count];
			for (int i = 0; i < count; ++i) {
				out_list[i] = _fields[i];
			}
			return count;
		}

		int TypeMeta::getMethodsList(MethodAccessor*& out_list) {
			int count = _methods.size();
			out_list = new MethodAccessor[count];
			for (int i = 0; i < count; ++i) {
				out_list[i] = _methods[i];
			}
			return count;
		}

		int TypeMeta::getBaseClassReflectionInstanceList(ReflectionInstance*& out_list, void* instance) {
			auto itr = m_class_map.find(_type_name);
			if (itr != m_class_map.end()) {
				return (std::get<0>(*itr->second))(out_list, instance);
			}
			return 0;
		}

		FieldAccessor TypeMeta::getFieldByName(const char* name) {
			auto condition = [&](const FieldAccessor& i) {
				return std::strcmp(i.getFieldName(), name) == 0;
			};
			const auto itr = std::find_if(_fields.begin(), _fields.end(), condition);
			if (itr != _fields.end()) {
				return *itr;
			}
			return FieldAccessor{ nullptr };
		}

		MethodAccessor TypeMeta::getMethodByName(const char* name) {
			auto condition = [&](const MethodAccessor& i) {
				return std::strcmp(i.getMethodName(), name) == 0;
			};
			const auto itr = std::find_if(_methods.begin(), _methods.end(), condition);
			if (itr != _methods.end()) {
				return *itr;
			}
			return MethodAccessor{ nullptr };
		}

		TypeMeta& TypeMeta::operator=(const TypeMeta& dest) {
			if (this == &dest) {
				return *this;
			}
			_fields.clear();
			_fields = dest._fields;
			_methods.clear();
			_methods = dest._methods;
			_type_name = dest._type_name;
			_is_valid = dest._is_valid;
			return *this;
		}

		FieldAccessor::FieldAccessor() {
			_field_type_name = k_unknown_type;
			_field_name = k_unknown;
			_functions = nullptr;
		}

		FieldAccessor::FieldAccessor(FieldFunctionTuple* functions) :_functions(functions) {
			_field_type_name = k_unknown_type;
			_field_name = k_unknown;
			if (_functions == nullptr) {
				return;
			}
			_field_type_name = (std::get<4>(*_functions))();
			_field_name = (std::get<3>)(*_functions)();
		}

		void* FieldAccessor::get(void* instance) {
			return static_cast<void*>((std::get<1>(*_functions))(instance));
		}

		void FieldAccessor::set(void* instance, void* value) {
			(std::get<0>(*_functions))(instance, value);
		}

		TypeMeta FieldAccessor::getOwnerTypeMeta() {
			return TypeMeta{ (std::get<2>(*_functions))() };
		}

		bool FieldAccessor::getTypeMeta(TypeMeta& field_type) {
			field_type = TypeMeta{ _field_type_name };
			return field_type._is_valid;
		}

		const char* FieldAccessor::getFieldName() const { 
			return _field_name; 
		}

		const char* FieldAccessor::getFieldTypeName() { 
			return _field_type_name; 
		}

		bool FieldAccessor::isArrayType() {
			return (std::get<5>(*_functions))();
		}

		FieldAccessor& FieldAccessor::operator=(const FieldAccessor& dest) {
			if (this == &dest) {
				return *this;
			}
			_functions = dest._functions;
			_field_name = dest._field_name;
			_field_type_name = dest._field_type_name;
			return *this;
		}

		MethodAccessor::MethodAccessor() {
			_method_name = k_unknown;
			_functions = nullptr;
		}

		MethodAccessor::MethodAccessor(MethodFunctionTuple* functions) :_functions(functions) {
			_method_name = k_unknown;
			if (_functions == nullptr) {
				return;
			}
			_method_name = (std::get<0>(*_functions))();
		}

		const char* MethodAccessor::getMethodName() const {
			return (std::get<0>(*_functions))();
		}

		MethodAccessor& MethodAccessor::operator=(const MethodAccessor& dest) {
			if (this == &dest) {
				return *this;
			}
			_functions = dest._functions;
			_method_name = dest._method_name;
			return *this;
		}

		void MethodAccessor::invoke(void* instance) {
			(std::get<1>(*_functions))(instance);
		}

		ArrayAccessor::ArrayAccessor() :_functions(nullptr), _array_type_name(k_unknown_type), _element_type_name(k_unknown_type) {}

		ArrayAccessor::ArrayAccessor(ArrayFunctionTuple* functions) : _functions(functions) {
			_array_type_name = k_unknown_type;
			_element_type_name = k_unknown_type;
			if (_functions == nullptr) {
				return;
			}
			_array_type_name = (std::get<3>(*_functions))();
			_element_type_name = (std::get<4>(*_functions))();
		}

		const char* ArrayAccessor::getArrayTypeName() {
			return _array_type_name;
		}

		const char* ArrayAccessor::getElementTypeName() {
			return _element_type_name;
		}

		void ArrayAccessor::set(int index, void* instance, void* element_value) {
			size_t count = getSize(instance);
			std::get<0>(*_functions)(index, instance, element_value);
		}

		void* ArrayAccessor::get(int index, void* instance) {
			size_t count = getSize(instance);
			return std::get<1>(*_functions)(index, instance);
		}

		int ArrayAccessor::getSize(void* instance) {
			return std::get<2>(*_functions)(instance);
		}

		ArrayAccessor& ArrayAccessor::operator=(ArrayAccessor& dest) {
			if (this == &dest) {
				return *this;
			}
			_functions = dest._functions;
			_array_type_name = dest._array_type_name;
			_element_type_name = dest._element_type_name;
			return *this;
		}

		ReflectionInstance& ReflectionInstance::operator=(const ReflectionInstance& dest) {
			if (this == &dest) {
				return *this;
			}
			m_instance = dest.m_instance;
			m_meta = dest.m_meta;
			return *this;
		}

		ReflectionInstance& ReflectionInstance::operator=(ReflectionInstance&& dest) {
			if (this == &dest) {
				return *this;
			}
			m_instance = dest.m_instance;
			m_meta = dest.m_meta;
			return *this;
		}
	}
}