#pragma once

#include <string>

#include <libplateau_api.h>
#include "plateau/geometry/geo_coordinate.h"
#include "plateau/dataset/grid_code.h"

namespace plateau::dataset {

    enum class StandardMapGridLevel
    {
        Invalid = -1,
        Level50000 = 0,
        Level5000 = 1,
        Level2500 = 2,
        Level1000 = 3,
        Level500 = 4,
    };

    /**
     * \brief 国土基本図図郭を表します。
     * 
     * 国土基本図の図郭コードを扱い、緯度経度範囲の取得などの機能を提供します。
     */
    class LIBPLATEAU_EXPORT StandardMapGrid : public GridCode {
    public:
        explicit StandardMapGrid(std::string code);
        StandardMapGrid() = default;

        /**
         * \brief 図郭コードを文字列として取得します。
         */
        std::string get() const override;

        /**
         * \brief 図郭の緯度経度範囲を取得します。
         */
        geometry::Extent getExtent() const override;

        /**
         * \brief 図郭コードが適切な値かどうかを返します。
         */
        bool isValid() const override;

        /**
         * \brief １段階上のレベルのグリッドコードに変換します。
         */
        std::shared_ptr<GridCode> upper() const override;
        GridCode* upperRaw() const override;

        /**
         * \brief コードのレベル（詳細度）を取得します。
         */
        int getLevel() const override;

        /**
         * \brief コードのレベル（詳細度）が、PLATEAUの仕様上考えられる中でもっとも大きいものであるときにtrueを返します。
         */
        bool isLargestLevel() const override;
        bool isSmallerThanNormalGml() const override;
        bool isNormalGmlLevel() const override;

        bool operator==(const StandardMapGrid& other) const;
        bool operator<(const StandardMapGrid& other) const;

        /**
         * \brief 図郭コードから平面直角座標系での範囲を計算します。
         * \return 平面直角座標系での範囲（min, max）
         */
        std::pair<TVec3d, TVec3d> calculateGridExtent() const;

    private:
        std::string code_;  // 図郭コード
        bool is_valid_ = false;     // コードが有効かどうか
        StandardMapGridLevel level_;

        int coordinate_origin_; // 原点
        char first_row_;       // 1文字目はrow座標（東西, A-H）
        char first_column_;    // 2文字目はcolumn座標（南北, A-T）
        int second_row_;
        int second_column_;
        int third_row_;
        int third_column_;
    };
}
