# インストール
## 対応Unrealバージョンのインストール
- Unreal Engineの対応バージョンについて、[リリースページ](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unreal/releases) に記載のバージョンを利用してください。
  - [Epic Games Launcher](https://www.unrealengine.com/ja/download) から指定のバージョンをインストールできます。


## Unrealプロジェクトの作成
- 先ほどインストールしたバージョンを起動します。
- Project Defaultsで`C++`を選択します。
- 適宜設定を行い`作成`を押します。
- Unreal Engineの仕様上、プロジェクトパスに日本語が含まれるとビルド時にエラーが発生する場合があります。フォルダ名に日本語を含めないようにしてください。
![](../resources/manual/installation/unrealEngineNewProjectCategory.png)

<a id="blueprint_project"></a>

## 既存のBlueprintプロジェクトを利用する場合
> [!Note]
> C++プロジェクトへのSDKを導入される場合はこの章の対応は必要ございません。  

BlueprintプロジェクトにSDKを導入し、パッケージを作成した場合はアプリケーション起動時にエラーダイアログが表示されます。パッケージを利用される場合は下記の手順を追加で行って頂く必要がございます。  

### ダミーのC++クラスを追加
1. メニューからC++クラスを追加します
   ![](../resources/manual/installation/addCppCodes.png)

2. 親クラスにNoneを選択します
   ![](../resources/manual/installation/parentClass.png)

3. 新しいクラス名をつけます
   ![](../resources/manual/installation/newClass.png)

4. ダイアログを閉じた後、エディタを閉じます
   ![](../resources/manual/installation/includeDialog.png)

5. YES選択後に作成されたC++クラスがIDEで開かれます
   ![](../resources/manual/installation/openCodes.png)

6. コードをビルドします（エディタを閉じていないとビルドエラーが出ます）
   ![](../resources/manual/installation/buildCodes.png)

7. エディターを起動してプロジェクトを開いた後、パッケージを作成してアプリケーションを起動できます

## PLATEAU SDK for Unrealの導入
 - [リリースページ](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unreal/releases)から提供されている`PLATEAU-SDK-for-Unreal-{バージョン名}.zip`をダウンロードします。
 - 作成したプロジェクトのフォルダ（先ほどのプロジェクト作成で指定した場所）に`Plugins`という名前のフォルダを作成し、先程ダウンロードしたSDKを解凍しコピーします。
(`Plugins`フォルダの作成は、`Unreal Engine` のコンテンツブラウザからではなく、エクスプローラで作成してください。)
 - `Plugins/PLATEAU-SDK-for-Unreal/PLATEAU-SDK-for-Unreal.uplugin`が存在することを確認します。
 - プロジェクトを開きます。既にプロジェクトを開いている場合は一度閉じてから再度プロジェクトを開いてください。
 - プロジェクトを開く際に以下の画面が表示される場合は`はい`を押して完了です。

![](../resources/manual/installation/pluginBuild.png)

# エディタ設定の変更
## 自動保存の無効化
3D都市モデルのインポート中にマップの自動保存が実行された際エディタがクラッシュしてしまう場合があるため、以下の手順で自動保存を無効化してください。
1. `エディタの環境設定`から`ロード＆保存中`を選択します。
2. `自動保存`内の`マップを保存`のチェックを外します。
![](../resources/manual/installation/disableAutoMapSave.png)


## 自動再インポート対象の除外
1. `エディタの環境設定`から`ロード＆保存中`を選択します。
2. `自動再インポート`内の`監視するディレクトリ`→`インデックス[0]`を選択します。
3. `Include/Exclude Wildcards`に除外対象として`PLATEAU/Datasets/*`を追加します。
![](../resources/manual/installation/excludeFromReimportTarget.png)
この操作を行わない場合、取得されたgmlファイル等の生データを手動でインポート対象から除外する必要があります。

## 距離スケーリング済みのカメラ速度を使用
1. `エディタの環境設定`から`距離スケーリング`で検索します。
2. `距離スケーリング済みのカメラ速度を使用`にチェックを入れます。
![](../resources/manual/installation/distanceScaled.png)
この操作を行うことで広域なマップでの操作性が向上します。

# プラグインをビルドする場合
- Windowsの場合、 Visual Studio の利用を想定しています。
  - Visual Studio についても、[リリースページ](https://github.com/Project-PLATEAU/PLATEAU-SDK-for-Unreal/releases) に記載のバージョンを利用してください。  
    バージョンが違うとビルドに失敗する場合があります。
  - Visual Studio のインストール時は、[こちらの手順に従って](https://docs.unrealengine.com/5.0/ja/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine/) 追加のコンポーネントをインストールしてください。
- MacOSの場合、Xcode がインストールされていることが前提となります。

# トラブルシューティング
うまく導入できない場合、次のことをご確認ください。
- お使いのウイルス対策ソフトによっては、SDKに含まれるバイナリファイルが削除されることがあります。ウィルス対策ソフトによってSDKのファイルが削除されないよう設定をお願いします。
- 既存のBlueprintプロジェクトにSDKを導入してパッケージしたアプリケーションを起動した際に下図のエラーが出ます。[こちら](#blueprint_project) を参照して下さい。
  ![](../resources/manual/installation/moduleError.png)
