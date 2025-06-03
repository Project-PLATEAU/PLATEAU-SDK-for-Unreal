#pragma once

#include <citygml/vecs.hpp>
#include "geo_coordinate.h"
#include <libplateau_api.h>

namespace plateau::geometry {

    /**
     * 座標変換の基準を保持し、座標変換します。
     *
     * zoneID についてはこちらを参照してください :
     * https://www.gsi.go.jp/sokuchikijun/jpc.html
     */
    class LIBPLATEAU_EXPORT GeoReference {
    public:
        explicit GeoReference(int coordinate_zone_id, const TVec3d& reference_point = TVec3d(0, 0, 0),
                              float unit_scale = 1.0, CoordinateSystem coordinate_system = CoordinateSystem::ENU);

        /**
         * 緯度・経度・高さで表現される座標を平面直角座標系に変換します。
         * 座標軸変換を含みます。
         */    
        TVec3d project(const GeoCoordinate& point) const;
        TVec3d project(const TVec3d& lat_lon) const;
        /// project の座標軸変換をしない版です。座標軸は ENU → ENU であるとします。 reference_point_ は ENUに変換されます。
        TVec3d projectWithoutAxisConvert(const TVec3d& lat_lon) const;
        TVec3d convertAxisToENU(const TVec3d& vertex) const;
        TVec3d convert(const TVec3d& lat_lon, const bool convert_axis = true, const int epsg = CoordinateReferenceFactory::default_epsg) const;
        static TVec3d convertAxisFromENUTo(CoordinateSystem axis, const TVec3d& vertex);
        static TVec3d convertAxisToENU(CoordinateSystem axis, const TVec3d& vertex); 

        static TVec3d reverseXY(const TVec3d& vertex);
        static GeoCoordinate planeToPolar(const TVec3d& vertex, const int coordinate_zone_id);

        GeoCoordinate unproject(const TVec3d& point) const;

        void setReferencePoint(TVec3d point);
        TVec3d getReferencePoint() const;
        int getZoneID() const;
        void setZoneID(int value);
        float getUnitScale() const;
        CoordinateSystem getCoordinateSystem() const;

    private:
        TVec3d reference_point_;
        int zone_id_;
        CoordinateSystem coordinate_system_;
        float unit_scale_;
    };
}
