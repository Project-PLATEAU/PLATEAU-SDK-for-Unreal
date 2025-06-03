#pragma once

#include <libplateau_api.h>
#include <set>
#include <optional>
#include "plateau/network/client.h"
#include "city_model_package.h"
#include "plateau/dataset/grid_code.h"

namespace plateau::dataset {

    /**
     * \brief GMLファイルに関する情報を保持するクラスです。
     */
    class LIBPLATEAU_EXPORT GmlFile {
    public:
        explicit GmlFile(const std::string& path);
        GmlFile(const std::string& path, const network::Client& client, int max_lod = -1);


        const std::string& getPath() const;
        void setPath(const std::string& path);
        std::shared_ptr<GridCode> getGridCode() const;
        /**
         * getGridCode関数をP/Invokeで利用するための生ポインタ版です。
         * 新しいGridCodeインスタンスを生成して返すので、DLL利用者が廃棄する必要があります。
         */
        GridCode* getGridCodeRaw() const;
        double getEpsg() const;
        const std::string& getFeatureType() const;
        PredefinedCityModelPackage getPackage() const;
        std::string getAppearanceDirectoryPath() const;
        bool isValid() const;

        bool isMaxLodCalculated() const;

        /**
         * GMLファイルの中身を文字列検索し、最大LODを求めます。
         * どのLODにも未対応であれば -1 を返します。
         */
        int getMaxLod();

        /**
         * \brief CityGMLファイルとその関連ファイル(テクスチャ、コードリスト)をコピーします。コピー先にすでにファイルが存在する場合はスキップします。
         * \param destination_root_path コピー先のフォルダへのパス。このパスの配下に3D都市モデルデータ製品のルートフォルダが配置されます。
         * \returns コピー先されたCityGMLファイルの情報を返します。
         */
        std::shared_ptr<GmlFile> fetch(const std::string& destination_root_path) const;

        /**
         * \brief GmlFileのパスがローカルマシンを指す場合、CityGMLファイルとその関連ファイル(テクスチャ、コードリスト)をコピーします。コピー先にすでにファイルが存在する場合はスキップします。
         * パスが http で始まる場合、GMLファイルとその関連ファイルをダウンロードします。
         * \param destination_root_path コピー先のフォルダへのパス。このパスの配下に3D都市モデルデータ製品のルートフォルダが配置されます。
         * \param copied_gml_file コピーされたCityGMLファイル
         */
        void fetch(const std::string& destination_root_path, GmlFile& copied_gml_file) const;

        /**
         * \brief GMLファイルの全文を検索し、見つかったコードリストパスの一覧を返します。
         */
        std::set<std::string> searchAllCodelistPathsInGML() const;

        /**
         * \brief GMLファイルの全文を検索し、見つかったテクスチャパスの一覧を返します。
         */
        std::set<std::string> searchAllImagePathsInGML() const;

    private:
        std::string path_;
        std::shared_ptr<GridCode> grid_code_;
        std::string feature_type_;
        std::string epsg_;
        bool is_valid_;
        bool is_local_;
        int max_lod_;

        /// サーバーモード(is_local_ が falseのとき)のみ利用します。ダウンロードに使用するクライアントです。
        std::optional<network::Client> client_;

        void applyPath();
    };
}
