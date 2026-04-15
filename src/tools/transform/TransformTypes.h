#pragma once

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

enum class TransformMode
{
    Translate,
    Scale,
    Rotate
};