#pragma once

#include "Util/G3DCore.h"

namespace RBX {

// Represents rotations in angles
class RotationAngle
{
public:
    RotationAngle()
		: value(0)
        , sin(0)
        , cos(1)
	{
	}

    explicit RotationAngle(float angle)
    {
        float angleRad = angle * (G3D::pi() / 180.f);

        value = angle;
        sin = sinf(angleRad);
        cos = cosf(angleRad);
    }

    bool empty() const
    {
        return value == 0.f;
    }

    float getValue() const
    {
        return value;
    }

    float getSin() const
    {
        return sin;
    }

    float getCos() const
    {
        return cos;
    }

    bool operator==(const RotationAngle& other) const
    {
        return value == other.value;
    }

    bool operator!=(const RotationAngle& other) const
    {
        return value != other.value;
    }

    RotationAngle inverse() const
    {
        RotationAngle result;

        result.value = -value;
        result.sin = -sin;
        result.cos = cos;

        return result;
    }

    RotationAngle combine(const RotationAngle& other) const
	{
        RotationAngle result;

        result.value = value + other.value;
        result.sin = sin * other.cos + cos * other.sin;
        result.cos = cos * other.cos - sin * other.sin;

        return result;
    }

private:
    float value;
    float sin;
    float cos;
};

class Rotation2D
{
public:
    Rotation2D()
	{
	}

    Rotation2D(const RotationAngle& angle, const Vector2& center)
		: angle(angle)
        , center(center)
	{
	}

    const RotationAngle& getAngle() const
    {
        return angle;
    }

    const Vector2& getCenter() const
    {
        return center;
    }

    bool empty() const
    {
        return angle.empty();
    }

    bool operator==(const Rotation2D& other) const
    {
        return angle == other.angle && center == other.center;
    }

    bool operator!=(const Rotation2D& other) const
    {
        return angle != other.angle || center != other.center;
    }

    Vector2 rotate(const Vector2& p) const
    {
        if (angle.empty())
            return p;

		Vector2 pl = p - center;

        return center + Vector2(
            pl.x * angle.getCos() - pl.y * angle.getSin(),
            pl.y * angle.getCos() + pl.x * angle.getSin());
    }

    Rotation2D inverse() const
    {
        return Rotation2D(angle.inverse(), center);
    }

private:
    RotationAngle angle;
    Vector2 center;

};

}