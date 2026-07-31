#pragma once
#include <string>
#include <map>

using FString = std::string;

template<typename K, typename V>
struct TMap
{
    std::map<K, V> Map;
    V* Find(const K& Key)
    {
        auto It = Map.find(Key);
        return It != Map.end() ? &It->second : nullptr;
    }
    const V* Find(const K& Key) const
    {
        auto It = Map.find(Key);
        return It != Map.end() ? &It->second : nullptr;
    }
    V& Add(const K& Key, const V& Value)
    {
        Map[Key] = Value;
        return Map[Key];
    }
};

class UObject {};
class UStaticMesh {};
class USkeletalMesh {};
class UAnimInstance {};
class UAnimSequence {};
class UMaterialInterface {};
class USoundBase {};

struct FName
{
    std::string Name;
    FName() = default;
    FName(const char* s) : Name(s ? s : "") {}
    bool operator==(const FName& Other) const { return Name == Other.Name; }
};

struct FLinearColor
{
    float R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;
    FLinearColor() = default;
    FLinearColor(float r, float g, float b, float a = 1.0f) : R(r), G(g), B(b), A(a) {}
};

struct FVector
{
    double X = 0.0, Y = 0.0, Z = 0.0;
    FVector() = default;
    FVector(double x, double y, double z) : X(x), Y(y), Z(z) {}
};

struct FRotator
{
    double Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
    FRotator() = default;
    FRotator(double p, double y, double r) : Pitch(p), Yaw(y), Roll(r) {}
};

template<typename T>
struct TSoftObjectPtr
{
    std::string Path;
    bool IsNull() const { return Path.empty(); }
};

template<typename T>
struct TSoftClassPtr
{
    std::string Path;
    bool IsNull() const { return Path.empty(); }
};

#ifndef USTRUCT
#define USTRUCT(...)
#endif
#ifndef UCLASS
#define UCLASS(...)
#endif
#ifndef UPROPERTY
#define UPROPERTY(...)
#endif
#ifndef UFUNCTION
#define UFUNCTION(...)
#endif
#ifndef GENERATED_BODY
#define GENERATED_BODY(...)
#endif
#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif
#ifndef BlueprintType
#define BlueprintType
#endif
