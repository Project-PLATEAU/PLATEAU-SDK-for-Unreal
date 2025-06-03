#pragma once

#include <string>
#include <vector>

#include <libplateau_api.h>
#include "plateau/geometry/geo_coordinate.h"
#include "plateau/dataset/grid_code.h"

namespace plateau::dataset {
    /**
     * \brief 地域メッシュコードを表します。
     * 
     * 2~5次メッシュの緯度経度範囲の取得、緯度経度範囲を内包する3次メッシュの取得を行う機能を提供しています。
     */
class LIBPLATEAU_EXPORT MeshCode : public plateau::dataset::GridCode {
    public:
        explicit MeshCode(const std::string& code);
        MeshCode() = default;

        /**
         * \brief メッシュコードを文字列として取得します。
         */
        std::string get() const override;

        /**
         * \brief メッシュコードの次数を取得します。
         */
        int getLevel() const override;

        /**
         * \brief メッシュコードの緯度経度範囲を取得します。
         */
        geometry::Extent getExtent() const override;

        /**
         * \brief 座標点を含む3次メッシュを取得します。
         */
        static MeshCode getThirdMesh(const geometry::GeoCoordinate& coordinate);

        /**
         * \brief 座標範囲を含む3次メッシュを全て取得します。
         */
        static void getThirdMeshes(const geometry::Extent& extent, std::vector<MeshCode>& mesh_codes);

        /**
         * \brief 座標範囲を含む3次メッシュを全て取得します。
         */
        static std::shared_ptr<std::vector<MeshCode>> getThirdMeshes(const geometry::Extent& extent);

        /**
         * \brief 地域メッシュを2次メッシュとして取得します。
        */
        MeshCode asSecond() const;

        /**
         * \brief レベル2以上の範囲で１段階上のレベルの地域メッシュに変換します。
        */
        std::shared_ptr<GridCode> upper() const override;
        GridCode* upperRaw() const override;

        /**
         * \brief メッシュコードが適切な値かどうかを返します。
        */
        bool isValid() const override;

        /**
         * \brief コードのレベル（詳細度）が、PLATEAUの仕様上考えられる中でもっとも大きいものであるときにtrueを返します。
        */
        bool isLargestLevel() const override;
        bool isSmallerThanNormalGml() const override;
        bool isNormalGmlLevel() const override;

        bool operator==(const MeshCode& other) const;



    private:
        int first_row_ = 0;
        int first_col_ = 0;
        int second_row_ = 0;
        int second_col_ = 0;
        int third_row_ = 0;
        int third_col_ = 0;
        int fourth_row_ = 0;
        int fourth_col_ = 0;
        int fifth_row_ = 0;
        int fifth_col_ = 0;
        int level_ = 0;
        bool is_valid_ = false;

        static void nextRow(MeshCode& mesh_code);
        static void nextCol(MeshCode& mesh_code);
        static int compareCol(const MeshCode& lhs, const MeshCode& rhs);
        static int compareRow(const MeshCode& lhs, const MeshCode& rhs);
    };
}
