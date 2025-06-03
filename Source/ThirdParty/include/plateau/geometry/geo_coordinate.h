#pragma once

#include "citygml/vecs.hpp"
#include "citygml/cityobject.h"
#include <array>
#include <unordered_map>
#include <utility>

namespace plateau::geometry {

    /**
     * 緯度・経度・高さ による位置表現です。
     * PLATEAU の gmlファイルでは 緯度・経度・高さ によって座標が表現されます。
     *
     * 厳密にどの基準に基づいた座標であるかは、
     * GMLファイル名 または CityModel の Envelope の srcName に記載された
     * EPSGコードによって判別できます。
     * EPSGコードが 6697 のとき、それは
     * 「日本測地系2011における経緯度座標系と東京湾平均海面を基準とする標高の複合座標参照系」
     * になります。
     * EPSGコードが 10162 ～ 10174　の場合は平面直角座標系となります。
     */
    struct GeoCoordinate {
        double latitude;
        double longitude;
        double height;

        GeoCoordinate() = default;

        GeoCoordinate(double lat, double lon, double height) :
                latitude(lat),
                longitude(lon),
                height(height) {
        }

        GeoCoordinate operator+(GeoCoordinate op) const;
        GeoCoordinate operator*(double op) const;
        GeoCoordinate operator-(GeoCoordinate op) const;
        GeoCoordinate operator/(GeoCoordinate op) const;
    };


    /**
     * @enum CoordinateSystem
     *
     * 各列挙子について、3つのアルファベットはXYZ軸がどの方角、方向になるかを表しています。<br/>
     * N,S,E,Wはそれぞれ北,南,東,西<br/>
     * U,Dはそれぞれ上,下<br/>
     * に対応します。<br/>
     */
    enum class CoordinateSystem {
        //! PLATEAUでの座標系
        ENU = 0,
        WUN = 1,
        //! Unreal Engineでの座標系
        ESU = 2,
        //! Unityでの座標系
        EUN = 3
    };


    /**
     * 緯度・経度・高さの最小・最大で表現される範囲です。
     */
    struct Extent {
        GeoCoordinate min;
        GeoCoordinate max;

        Extent(const GeoCoordinate& min, const GeoCoordinate& max) {
            this->min = min;  // NOLINT(cppcoreguidelines-prefer-member-initializer)
            this->max = max;  // NOLINT(cppcoreguidelines-prefer-member-initializer)
        }

        bool contains(GeoCoordinate point, bool ignore_height = true) const;
        bool contains(TVec3d point, bool ignore_height = true) const;

        /**
         * 引数 city_obj の位置を推定し、その位置が Extent の範囲内に含まれるかどうかを返します。
         * city_obj の位置が不明の場合は false を返します。
         */
        bool contains(const citygml::CityObject& city_obj, bool ignore_height = true) const;

        /**
         * 座標系のEPSGコードに応じて適切な座標変換を行い、点が範囲内に含まれるかを判定します。
         * 平面直角座標系（EPSG 10162～10174）の場合は、平面座標を緯度経度座標に変換してから判定します。
         * 極座標系（それ以外のEPSG）の場合は、通常のcontainsメソッドと同様に直接判定します。
         *
         * @param point 判定対象の点
         * @param epsg 座標系を識別するEPSGコード
         * @param ignore_height 高さを無視する場合はtrue
         * @return 点が範囲内に含まれる場合はtrue
         */
        bool containsInPolar(TVec3d point,const int epsg, bool ignore_height = true) const;
        bool containsInPolar(const citygml::CityObject& city_obj,const int epsg, bool ignore_height = true) const;

        /**
         * other と交わる箇所があるかどうかを返します。
         * ただし other の高さは無視して緯度と経度の2次元のみで判定します。
         */
        bool intersects2D(const Extent& other) const;

        /**
         * min と max の中点を GeoCoordinate で返します。
         */
        GeoCoordinate centerPoint() const;

        /// Extentの南西端のUVを(0,0),北東端のUVを(1,1)とするとき、指定位置のUVを求めます。
        TVec2f uvAt(GeoCoordinate coord) const;

        static Extent all() {
            return {
                    GeoCoordinate(-90, -180, -9999),
                    GeoCoordinate(90, 180, 9999)
            };
        }
    };

    /**
     * 平面直角座標系の判定、平面直角座標の基準点取得
     */
    struct CoordinateReferenceFactory {
        static constexpr int default_epsg = 6697;
        // EPSGとZone IDのマッピング
        static const std::unordered_map<int, int> epsg_to_zone;
        // Zone IDごとの座標データ
        static const std::unordered_map<int, std::array<double, 3>> zone_to_point;
        // EPSGごとのzone取得
        static int GetZoneId(int epsg) {
            auto it = epsg_to_zone.find(epsg);
            if (it != epsg_to_zone.end()) {
                return it->second;
            }
            return 0;
        }
        // EPSGごとの基準点取得
        static GeoCoordinate GetOriginPoint(int epsg) {
            const int zone = GetZoneId(epsg);
            if (zone != 0) {
                auto it = zone_to_point.find(zone);
                if (it != zone_to_point.end()) {
                    const auto& coords = it->second;
                    return GeoCoordinate(coords[0], coords[1], coords[2]);
                }
            }
            return GeoCoordinate();
        }
        // 極座標系・平面直角座標系判定
        static bool IsPolarCoordinateSystem(int epsg) {
            // 平面直角座標系（EPSG 10162～10174）かどうかを明示的に確認
            return epsg_to_zone.find(epsg) == epsg_to_zone.end();
        }
    };
    // EPSGとZone IDのマッピング
    inline const std::unordered_map<int, int> CoordinateReferenceFactory::epsg_to_zone = {
        {10162, 1}, {10163, 2}, {10164, 3}, {10165, 4}, {10166, 5},
        {10167, 6}, {10168, 7}, {10169, 8}, {10170, 9}, {10171, 10},
        {10172, 11}, {10173, 12}, {10174, 13}
    };
    // Zone IDごとの座標データ
    inline const std::unordered_map<int, std::array<double, 3>> CoordinateReferenceFactory::zone_to_point = {
        {1, {33.0, 129.5, 0.0}},   {2, {33.0, 131.0, 0.0}},   {3, {36.0, 132.166667, 0.0}},
        {4, {33.0, 133.5, 0.0}},   {5, {36.0, 134.333333, 0.0}}, {6, {36.0, 136.0, 0.0}},
        {7, {36.0, 137.166667, 0.0}}, {8, {36.0, 138.5, 0.0}},   {9, {35.0, 139.833333, 0.0}},
        {10, {40.0, 140.833333, 0.0}}, {11, {44.0, 140.25, 0.0}},  {12, {44.0, 142.0, 0.0}},
        {13, {43.0, 144.0, 0.0}}
    };
}
