#pragma once

#include <string>
#include <vector>

#include <libplateau_api.h>
#include "plateau/geometry/geo_coordinate.h"

namespace plateau::udx {
    class LIBPLATEAU_EXPORT MeshCode {
    public:
        explicit MeshCode(const std::string& code);
        MeshCode() = default;

        static int getLevel(const std::string& code);
        std::string get() const;
        geometry::Extent getExtent() const;
        static MeshCode getThirdMesh(const geometry::GeoCoordinate& coordinate);
        static void getThirdMeshes(const geometry::Extent& extent, std::vector<MeshCode>& mesh_codes);
        bool isWithin(const MeshCode& other) const;

        bool operator==(const MeshCode& other) const;

        //! setに入れるために演算子オーバーロードします。
        bool operator<(MeshCode& other) const;
        bool operator<(const MeshCode& other) const;

    private:
        int first_row_;
        int first_col_;
        int second_row_;
        int second_col_;
        int third_row_;
        int third_col_;
        int level_;

        static void nextRow(MeshCode& mesh_code);
        static void nextCol(MeshCode& mesh_code);
        static int compareCol(const MeshCode& lhs, const MeshCode& rhs);
        static int compareRow(const MeshCode& lhs, const MeshCode& rhs);
    };
}