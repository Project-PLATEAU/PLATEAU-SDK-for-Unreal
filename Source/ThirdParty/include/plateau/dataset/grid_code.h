#pragma once

#include <string>
#include <memory>
#include <libplateau_api.h>
#include "plateau/geometry/geo_coordinate.h"

namespace plateau::dataset {
    class StandardMapGrid;
    class MeshCode;

    /**
     * \brief 地図の区画を表すコードの基底クラスです。
     * 
     * メッシュコードや国土基本図図郭など、地図の区画を表現するコードシステムの共通機能を提供します。
     */
    class LIBPLATEAU_EXPORT GridCode {
    public:
        virtual ~GridCode() = default;

        /**
         * \brief コードを文字列として取得します。
         */
        virtual std::string get() const = 0;

        /**
         * \brief コードが表す緯度経度範囲を取得します。
         */
        virtual geometry::Extent getExtent() const = 0;

        /**
         * \brief コードが適切な値かどうかを返します。
         */
        virtual bool isValid() const = 0;

        /**
         * \brief １段階上のレベルのグリッドコードに変換します。
         */
        virtual std::shared_ptr<GridCode> upper() const = 0;

        /**
         * \brief upper()のP/Invokeから呼び出す版です。newして返すので、利用者が適切に廃棄する必要があります。
         */
        virtual GridCode* upperRaw() const = 0;

        /**
         * \brief コードのレベル（詳細度）を取得します。
         */
        virtual int getLevel() const = 0;

        /**
         * \brief コードのレベル（詳細度）が、PLATEAUの仕様上考えられる中でもっとも広域であるときにtrueを返します。
         */
        virtual bool isLargestLevel() const = 0;

        /**
         * \brief コードのレベル（詳細度）が、PLATEAUの典型的な建物のGMLファイルのレベルよりも詳細である場合にtrueを返します。
         */
        virtual bool isSmallerThanNormalGml() const = 0;

        /**
         * \brief コードのレベル（詳細度）が、PLATEAUの典型的な建物のGMLファイルのレベルである場合にtrueを返します。
         */
        virtual bool isNormalGmlLevel() const = 0;


        /**
         * \brief 与えられたコードから適切なGridCodeの派生クラスのインスタンスを作成します。
         * \param code コード文字列
         * \return コードの形式に応じてMeshCodeまたはStandardMapGridのインスタンスを返します。
         * \throw std::invalid_argument コードの形式が不正な場合
         */
        static std::shared_ptr<GridCode> create(const std::string& code);

        /**
         * \brief create()をUnityのP/Invokeから呼び出す版です。newして返すので、利用者が適切に廃棄する必要があります。
         * \param code コード文字列
         * \return コードの形式に応じてMeshCodeまたはStandardMapGridのインスタンスを返します。生ポインタで返されます。
         * \throw std::invalid_argument コードの形式が不正な場合
         */
        static GridCode* createRaw(const std::string& code);

    };

    struct GridCodeComparator {
        bool operator()(const std::shared_ptr<GridCode>& lhs, const std::shared_ptr<GridCode>& rhs) const {
            if(lhs == nullptr && rhs == nullptr) return false;
            if(lhs != nullptr && rhs == nullptr) return false;
            if(lhs == nullptr) return true;
            return lhs->get() < rhs->get();
        }
    };
} 
