#pragma once

struct FVector;

struct FQuat
{
	float X;
	float Y;
	float Z;
	float W;

	FQuat() : X(0), Y(0), Z(0), W(1) {}
	FQuat(float InX, float InY, float InZ, float InW) : X(InX), Y(InY), Z(InZ), W(InW) {}

	static FQuat Identity() { return {0, 0, 0, 1}; }
	static FQuat FromAxisAngle(const FVector& Axis, float AngleRad);

	static FQuat FromEuler(const FVector& EulerDeg);
	static FQuat FromRotationMatrix(const FMatrix& M);
	FVector ToEuler() const;
	FRotator ToRotator() const;

	FMatrix ToRotationMatrix() const;

	FQuat operator*(const FQuat& Q) const;

	void Normalize();

	FQuat Conjugate() const { return {-X, -Y, -Z, W}; }
	FQuat Inverse() const { FQuat c = Conjugate(); float n = X * X + Y * Y + Z * Z + W * W; return (n > 0) ? FQuat(c.X / n, c.Y / n, c.Z / n, c.W / n) : FQuat(); }
	static FQuat MakeFromDirection(const FVector& Direction);
	static FVector RotateVector(const FQuat& q, const FVector& v);
	FVector RotateVector(const FVector& V) const;

	/**
	 * Spherical Linear Interpolation
	 * Smoothly interpolates between two quaternions with constant angular velocity
	 * @param A Starting quaternion
	 * @param B Ending quaternion
	 * @param Alpha Interpolation factor (0 to 1)
	 * @return Interpolated quaternion
	 */
	static FQuat Slerp(const FQuat& A, const FQuat& B, float Alpha);

	/**
	 * Spherical Linear Interpolation with shortest path
	 * Automatically chooses the shorter rotation path
	 * @param A Starting quaternion
	 * @param B Ending quaternion
	 * @param Alpha Interpolation factor (0 to 1)
	 * @return Interpolated quaternion
	 */
	static FQuat SlerpShortestPath(const FQuat& A, const FQuat& B, float Alpha);
};
