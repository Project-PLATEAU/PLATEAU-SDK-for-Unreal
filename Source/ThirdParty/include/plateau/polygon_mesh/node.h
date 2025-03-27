#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include "mesh.h"
#include "transform.h"

namespace plateau::polygonMesh {
    /**
     * Model 以下の階層構造を構成するノードです。
     * Node は 0個以上の 子Node を持つため階層構造になります。
     *
     * 詳しくは Model クラスのコメントをご覧ください。
     *
     * Node::name_ はゲームエンジン側ではゲームオブジェクトの名前として解釈されることが想定されます。
     * Node::mesh_ はそのゲームオブジェクトの持つメッシュとして解釈されることが想定されます。
     */
    class LIBPLATEAU_EXPORT Node {
    public:
        explicit Node(std::string name);

        Node(std::string name, std::unique_ptr<Mesh>&& mesh);

        /// コピーを禁止します。
        Node(const Node& node) = delete;
        Node& operator=(const Node& node) = delete;
        Node(Node&& node) = default;
        Node& operator=(Node&& node) = default;

        const std::string& getName() const;
        void setName(const std::string& name);
        Mesh* getMesh() const;
        void setMesh(std::unique_ptr<Mesh>&& mesh);
        TVec3d getLocalPosition() const;
        void setLocalPosition(TVec3d pos);
        TVec3d getLocalScale() const;
        void setLocalScale(TVec3d scale);
        Quaternion getLocalRotation() const;
        void setLocalRotation(Quaternion rotation);
        Transform getLocalTransform() const;

        /// Meshが存在し、かつそのMeshに頂点が1つ以上あるときにtrueを返します。
        bool hasVertices() const;

        /// Nodeをmoveで追加し、追加後のNodeを返します。
        Node& addChildNode(Node&& node);
        void setChildNodes(std::vector<Node>&& child_nodes);

        Node& addEmptyChildNode(const std::string& name);
        size_t getChildCount() const;

        Node& getChildAt(unsigned int index);
        const Node& getChildAt(unsigned int index) const;

        /// Parent Node設定
        void setParentNode(Node* node);  
        const Node& getParentNode() const; //Model.assignNodeHierarchyを呼ぶことで取得可能になります。
        const bool hasParentNode() const;

        /// Root Node設定
        void setRootNode(Node* node);
        const Node& getRootNode() const; //Model.assignNodeHierarchyを呼ぶことで取得可能になります。
        const bool hasRootNode() const;

        /**
         * 子のうち、子もなくメッシュもないノードを削除します。再帰的に行われます。
         */
        void eraseEmptyChildren();

        /// このノードがメッシュを持ち、かつそのメッシュがポリゴンを持つときに true を返します。
        bool polygonExists() const;

        /// Node 以下の階層構造を stringstream に書き込みます。
        void debugString(std::stringstream& ss, int indent) const;

        void setGranularityConvertInfo(bool is_primary, bool is_active); // GranularityConverterの内部でのみ利用する情報を付与します。
        bool isPrimary() const; // GranularityConverterでのみ利用します。
        void setIsActive(bool is_active); // GranularityConverterでのみ利用します。
        bool isActive() const; // GranularityConverterでのみ利用します。
        void reserveChild(size_t reserve_count);

        /// 子なしで自身のコピーを作成します。
        Node copyWithoutChildren() const;
    private:
        std::string name_;
        std::vector<Node> child_nodes_;
        Node* parent_node_;
        Node* root_node_;
        std::unique_ptr<Mesh> mesh_;
        bool is_primary_; // GranularityConverterでのみ利用します。

        /// FBX,GLTFエクスポート時にローカルなトランスフォームとして利用されます。
        Transform local_transform_;

        /**
         * ゲームエンジン上でNodeに相当するゲームオブジェクトがアクティブかどうかです。
         * GranularityConverterでのみ利用します。それ以外の用途では常にtrueになります。
         */
        bool is_active_;
    };
}
