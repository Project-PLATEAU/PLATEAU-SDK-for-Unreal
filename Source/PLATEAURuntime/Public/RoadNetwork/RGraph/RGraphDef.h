// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once
// ☀
#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "RGraphDef.generated.h"

/**
 * 道路タイプ
 */
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERRoadTypeMask : uint8 {
    /** 何もなし */
    Empty = 0 UMETA(DisplayName = "Empty"),

    /** 不正な値(今後要素が増えると後ろに追加する必要があるので最初に定義する) */
    Undefined = 1 << 0 UMETA(DisplayName = "Undefined"),

    /** 車道 */
    Road = 1 << 1 UMETA(DisplayName = "Road"),

    /** 歩道 */
    SideWalk = 1 << 2 UMETA(DisplayName = "SideWalk"),

    /** 中央分離帯 */
    Median = 1 << 3 UMETA(DisplayName = "Median"),

    /** 高速道路 */
    HighWay = 1 << 4 UMETA(DisplayName = "HighWay"),

    /** 車線. Lod3.1以上だとこれが車線を表す */
    Lane = 1 << 5 UMETA(DisplayName = "Lane"),

    /** 全ての値 */
    All = (Undefined | Road | SideWalk | Median | HighWay | Lane) UMETA(DisplayName = "All")
};
ENUM_CLASS_FLAGS(ERRoadTypeMask);

class FRRoadTypeMaskEx {
public:
    static ERRoadTypeMask All() {
        return
        ERRoadTypeMask::Undefined
        | ERRoadTypeMask::Road
        | ERRoadTypeMask::SideWalk
        | ERRoadTypeMask::Median
        | ERRoadTypeMask::HighWay
        | ERRoadTypeMask::Lane;
    }

    // 車道部分
    static bool IsRoad(ERRoadTypeMask Self) {
        return EnumHasAnyFlags(Self, ERRoadTypeMask::Road);
    }

    // 交通道路
    static bool IsHighWay(ERRoadTypeMask Self) {
        return EnumHasAnyFlags(Self, ERRoadTypeMask::HighWay);
    }

    // 歩道
    static bool IsSideWalk(ERRoadTypeMask Self) {
        return EnumHasAnyFlags(Self, ERRoadTypeMask::SideWalk);
    }

    // 中央分離帯
    static bool IsMedian(ERRoadTypeMask Self) {
        return EnumHasAnyFlags(Self, ERRoadTypeMask::Median);
    }

    // 中央分離帯
    static bool IsLane(ERRoadTypeMask Self) {
        return EnumHasAnyFlags(Self, ERRoadTypeMask::Lane);
    }

    // selfがflagのどれかを持っているかどうか
    static bool HasAnyFlag(ERRoadTypeMask Self, ERRoadTypeMask Flag) {
        return EnumHasAnyFlags(Self, Flag);
    }
};


template<typename T>
struct FRGraphRef {
    using Type = T*;

    template<class... Args>
    static T* New(Args&&... args) {
        auto Ret = NewObject<T>();
        Ret->Init(Forward<Args>(args)...);
        return Ret;
    }
};
template<typename T>
using RGraphRef_t = typename FRGraphRef<T>::Type;

template<typename T, class... Args>
inline RGraphRef_t<T> RGraphNew(Args&&... args) {
    return FRGraphRef<T>::New(Forward<Args>(args)...);
}