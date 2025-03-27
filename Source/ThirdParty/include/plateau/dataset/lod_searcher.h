#pragma once

#include <filesystem>
#include <libplateau_api.h>

namespace plateau::dataset {

    struct LIBPLATEAU_EXPORT LodFlag;

    /**
     * \brief GMLファイルに含まれるLOD番号を検索します。
     * ファイルの中身を文字列検索し、":lod(番号)" にヒットした番号のうち高いものを返します。
     *
     * 用途:
     * PLATEAUデータのインポートにおける範囲選択画面で、GMLファイルについて利用可能なLODを検索します。
     * 範囲選択画面では多くのGMLファイルを検索対象とするので、高速である必要があります。
     *
     * 検証方法:
     * このクラスに変更を加えたとき、多くのGMLファイルに関して結果が変わらないかどうかを検証する方法を残しました。
     * 詳しくはtest_lod_searcher.cpp の DisplayLodsRecursive という名前のテストのコメントを参照してください。
     */
    class LIBPLATEAU_EXPORT LodSearcher {

    public:
        /// ファイル中に含まれる最大LODを返します。
        /// 引数 specification_max_lod は仕様上とりうるLODの最大値であり、そのLODが見つかった場合は探索を終了してその値を返します。
        static int searchMaxLodInFile(const std::filesystem::path& file_path, int specification_max_lod);
        static int searchMaxLodInIstream(std::istream& ifs, int specification_max_lod);

    private:

        /**
         * 高速化のための調整項目です。
         * GMLファイル内で最初にLOD表記が見つかった時点を0バイト目として、ファイルを指定バイト数読んだ時点で検索を打ち切ります。
         * サイズを制限しないと、範囲選択画面で重いGMLファイルのアイコンを1つ表示するのに長々と待たされることになります。
         * この制限であれば、2023年の東京、沼津、新潟の全データにおいて、全文を読むのと結果が変わらないことを検証済みです。
         * また、「ファイル先頭からのバイト数」で制限するよりも「最初のLODからのバイト数」で制限したほうがバイト数を小さくでき、結果としてより高速化できることも東京、沼津、新潟で検証済みです。
         * しかし、今後のあらゆるデータで大丈夫であるという保証はなく、今後に調整が必要になるかもしれません。
         */
        static constexpr std::streamsize max_gml_read_size_from_lod_0_found = 10 /*メガバイト*/* 1000000;
        static constexpr std::streamsize max_gml_read_size_from_lod_1_found = 50 /*メガバイト*/* 1000000;
        static constexpr std::streamsize max_gml_read_size_from_lod_2_found = 120/*メガバイト*/* 1000000; // LOD2→3を検索するときは多めに検索しないと見逃す
        static constexpr std::streamsize max_gml_read_size_from_lod_3_found = 240/*メガバイト*/* 1000000; // LOD3→4を検索するときはさらに多めに検索しないと見逃す
//        static constexpr std::streamsize max_gml_read_size_from_lod_0_found = LONG_MAX;
    };
}
