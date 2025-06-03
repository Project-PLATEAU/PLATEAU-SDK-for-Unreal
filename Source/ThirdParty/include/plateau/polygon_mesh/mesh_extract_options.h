#pragma once

#include <plateau/geometry/geo_coordinate.h>
#include <citygml/vecs.hpp>
#include <plateau/polygon_mesh/polygon_mesh_utils.h>

namespace plateau::polygonMesh {
    /**
    * @enum MeshGranularity
    *
    * メッシュの結合単位
    */
    enum class MeshGranularity {
        //! 最小地物単位(LOD2, LOD3の各部品)
        PerAtomicFeatureObject = 0,
        //! 主要地物単位(建築物、道路等)
        PerPrimaryFeatureObject = 1,
        //! 都市モデル地域単位(GMLファイル内のすべてを結合)
        PerCityModelArea = 2
    };

    struct MeshExtractOptions {
        /// 設定をデフォルト値にするコンストラクタです。
        MeshExtractOptions() :
                reference_point(TVec3d(0, 0, 0)),
                mesh_axes(geometry::CoordinateSystem::EUN),
                mesh_granularity(MeshGranularity::PerPrimaryFeatureObject),
                max_lod(PolygonMeshUtils::max_lod_in_specification_), // 仕様上ありえる最大LODをデフォルトとします。
                min_lod(0), // 仕様上ありえる最小LODをデフォルトとします。
                export_appearance(true),
                grid_count_of_side(10),
                unit_scale(1.0),
                coordinate_zone_id(9), // 東京で歪みの少ない直交座標系をデフォルトとします。
                exclude_city_object_outside_extent(true),
                exclude_polygons_outside_extent(false),
                enable_texture_packing(false),
                texture_packing_resolution(2048),
                attach_map_tile(true),
                map_tile_zoom_level(15),
                map_tile_url("https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/{z}/{x}/{y}.jpg"),
                epsg_code(plateau::geometry::CoordinateReferenceFactory::default_epsg)
                {}

    public:
        TVec3d reference_point;
        geometry::CoordinateSystem mesh_axes;
        MeshGranularity mesh_granularity;
        unsigned max_lod;
        unsigned min_lod;
        bool export_appearance;
        /**
         * グリッド分けする時の、1辺の分割数です。
         * この数の2乗がグリッドの数となり、実際にはそれより細かくグループ分けされます。
         */
        int grid_count_of_side;
        float unit_scale;

        /**
         * 国土交通省が規定する、日本における平面直角座標系の基準点のうちどれを採用するかを番号で指定します。
         * 基準点と番号は次のWebサイトに記載のとおりです。
         * https://www.gsi.go.jp/sokuchikijun/jpc.html
         */
        int coordinate_zone_id;

        /**
         * 範囲外の3Dモデルを出力から除外するための、2つの方法のうち1つを有効にするかどうかを bool で指定します。
         * その方法とは、都市オブジェクトの最初の頂点の位置が範囲外のとき、そのオブジェクトはすべて範囲外とみなして出力から除外します。
         * これはビル1棟程度の大きさのオブジェクトでは有効ですが、
         * 10km×10kmの地形のような巨大なオブジェクトでは、実際には範囲内なのに最初の頂点が遠いために除外されるということがおきます。
         * したがって、この値は建物では true, 地形では false となるべきです。
         */
        bool exclude_city_object_outside_extent;

        /**
         * 範囲外の3Dモデルを出力から除外するための、2つの方法のうち1つを有効にするかどうかを bool で指定します。
         * その方法とは、メッシュ操作によって、範囲外に存在するポリゴンを除外します。
         * この方法であれば 10km×10km の地形など巨大なオブジェクトにも対応できます。
         */
        bool exclude_polygons_outside_extent;

        /**
         * テクスチャ結合（複数のテクスチャ画像を結合する機能）を有効にするかどうかを bool で指定します。
         */
        bool enable_texture_packing;

        /**
         * テクスチャ結合時の結合先のテクスチャ画像の解像度（縦：texture_packing_resolution x 横:texture_packing_resolution）
         */
        unsigned texture_packing_resolution;

        /**
         * 土地でのみ利用します。
         * 地図タイルを貼り付けるかどうかです。
         */
        bool attach_map_tile;

        /**
         * 土地でのみ利用します。
         * URLで地図タイルをダウンロードする場合のズームレベルです。
         */
        int map_tile_zoom_level;

        /**
         * 土地でのみ利用します。
         * URLで地図タイルをダウンロードする場合のURLであり、文字列として"{x}","{y}","{z}"を含むものです。
         * C#とC++でマーシャリングする関係上、charの固定長配列である必要があります。
         * 配列長を変更する場合、C#の MeshExtractOptions.cs にも変更を加える必要があります。
         *
         * 1000文字の根拠:
         * Wikipediaで登録されている長い言葉のページのURLが収まれば常識的に十分だろうと考えました。
         * 下のブログによると、2008年時点で最も長いURLは705文字です。
         * https://ctrlshift.hatenadiary.org/entry/20080119/1200719590
         */
        char map_tile_url[1000];

        /**
         * 平面直角座標系は、EPSGコードに応じて基準点を取得します。
         */
        int epsg_code;
    };
}
