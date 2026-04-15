#pragma once

enum class TransformMode
{
    None,
    Translate,
    Rotate,
    Scale
};

enum class TransformAxis
{
    None,
    X,
    Y,
    Z
};

enum class TransformSpace
{
    Global,
    Local
};