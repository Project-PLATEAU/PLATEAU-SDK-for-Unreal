#pragma once
#include <plateau/polygon_mesh/model.h>
#include <string>

namespace plateau::materialAdjust {
    using namespace plateau::polygonMesh;

    /// マテリアル分けの基底クラスです。
    class MaterialAdjusterBase {
    public:
        virtual bool shouldOverrideMaterial(const std::string& gml_id, int& out_next_override_game_mat_id) const = 0;
    };

    /// マテリアル分けの共通処理を提供します。
    /// 個別処理が必要な箇所は、コンストラクタで渡される MaterialAdjusterBase に以上します。
    class MaterialAdjusterCommon {
    public:
        MaterialAdjusterCommon(MaterialAdjusterBase* base) : base(base) {}
        void exec(plateau::polygonMesh::Model& model);

    private:
        MaterialAdjusterBase* base;
    };
}
