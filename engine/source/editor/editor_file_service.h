#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Dao{

    class EditorFileNode;
    using EditorFileNodeArray = std::vector<std::shared_ptr<EditorFileNode>>;

    struct EditorFileNode {
        std::string         m_file_name;
        std::string         m_file_type;
        std::string         m_file_path;
        int                 m_node_depth;
        EditorFileNodeArray m_child_nodes;
        EditorFileNode() = default;
        EditorFileNode(const std::string& name, const std::string& type, const std::string& path, int depth) {
            m_file_name = name;
            m_file_type = type;
            m_file_path = path;
            m_node_depth = depth;
        }
    };

    class EditorFileService {

    public:
        EditorFileNode* getEditorRootNode() { return _file_node_array.empty() ? nullptr : _file_node_array[0].get(); }
        void buildEngineFileTree();

    private:
        EditorFileNode* getParentNodePtr(EditorFileNode* file_node);
        bool            checkFileArray(EditorFileNode* file_node);

    private:
        EditorFileNodeArray _file_node_array;
        EditorFileNode      _root_node{ "asset", "Folder", "asset", -1 };

    };
}