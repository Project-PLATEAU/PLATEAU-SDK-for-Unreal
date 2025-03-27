// Copyright 2023 Ministry of Land, Infrastructure and Transport
#pragma once
#include "CoreMinimal.h"
#include <plateau_dll_logger.h>

/**
 * @brief PLATEAU共通ライブラリ内で発生したログをUnreal Engineに渡すロガーです。
 */
class PLATEAUDllLoggerUnreal
{
public:
    explicit PLATEAUDllLoggerUnreal(citygml::CityGMLLogger::LOGLEVEL LogLevel);
    
    /**
     * @brief PLATEAU共通ライブラリに渡すべきロガーを返します。 
     */
    std::shared_ptr<PlateauDllLogger> GetLogger();
private:
    std::shared_ptr<PlateauDllLogger> DllLogger;

    /**
     * @brief 共通ライブラリでInfoレベルのログが発生したときにコールバックで呼ばれます。
     */
     static void LogInfo(const char* Char);

    /**
     * @brief 共通ライブラリでWarningレベルのログが発生したときにコールバックで呼ばれます 
     */
    static void LogWarning(const char* Char);
    
    /**
     * @brief 共通ライブラリでErrorレベルのログが発生したときにコールバックで呼ばれます 
     */
    static void LogError(const char* Char);
};
