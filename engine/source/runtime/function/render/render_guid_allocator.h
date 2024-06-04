#pragma once

#include <unordered_map>

namespace Dao {
	static const size_t s_invalid_guid = 0;

	template<typename T>
	class GuidAllocator {
	public:
		static bool isValidGuid(size_t guid) { 
			return guid != s_invalid_guid;
		}

		size_t allocGuid(const T& t) {
			auto find_it = _elements_guid_map.find(t);
			if (find_it != _elements_guid_map.end()) {
				return find_it->second;
			}
			for (size_t i = 0; i <= _guid_elements_map.size(); ++i) {
				size_t guid = i + 1;
				if (_guid_elements_map.find(guid) == _guid_elements_map.end()) {
					_guid_elements_map.insert(std::make_pair(guid, t));
					_elements_guid_map.insert(std::make_pair(t, guid));
					return guid;
				}
			}
			return s_invalid_guid;
		}


		bool getGuidRelatedElement(size_t guid, T& t) {
			auto find_it = _guid_elements_map.find(guid);
			if (find_it != _guid_elements_map.end()) {
				t = find_it->second;
				return true;
			}
			return false;
		}

		bool getElementGuid(const T& t, size_t& guid) {
			auto find_it = _elements_guid_map.find(t);
			if (find_it != _elements_guid_map.end()) {
				guid = find_it->second;
				return true;
			}
			return false;
		}

		bool hasElement(const T& t) {
			return _elements_guid_map.find(t) != _elements_guid_map.end();
		}

		void freeGuid(size_t guid) {
			auto find_it = _guid_elements_map.find(guid);
			if (find_it != _guid_elements_map.end()) {
				const auto& element = find_it->second;
				_elements_guid_map.erase(element);
				_guid_elements_map.erase(guid);
			}
		}

		void freeElement(const T& t) {
			auto find_it = _elements_guid_map.find(t);
			if (find_it != _elements_guid_map.end()) {
				const auto& guid = find_it->second;
				_guid_elements_map.erase(guid);
				_elements_guid_map.erase(t);
			}
		}

		std::vector<size_t> getAllocatedGuids() const {
			std::vector<size_t> allocated_guids;
			for (const auto& entry : _guid_elements_map) {
				allocated_guids.push_back(entry.first);
			}
			return allocated_guids;
		}

		void clear() {
			_elements_guid_map.clear();
			_guid_elements_map.clear();
		}

	private:
		std::unordered_map<T, size_t> _elements_guid_map;
		std::unordered_map<size_t, T> _guid_elements_map;
	};
}