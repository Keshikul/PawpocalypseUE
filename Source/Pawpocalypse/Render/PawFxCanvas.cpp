#include "Render/PawFxCanvas.h"

void FPawFxCanvas::Reset()
{
	Verts.Reset();
	Colors.Reset();
	Indices.Reset();
	CurrentZ = 0.0;
}

FColor FPawFxCanvas::Bytes(const FLinearColor& C)
{
	auto B = [](float V) { return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(V * 255.0f), 0, 255)); };
	return FColor(B(C.R), B(C.G), B(C.B), B(C.A));
}

int32 FPawFxCanvas::Add(const FVector2D& P, const FColor& C)
{
	Verts.Add(FVector(P.X, P.Y, CurrentZ));
	Colors.Add(C);
	return Verts.Num() - 1;
}

void FPawFxCanvas::Circle(const FVector2D& Centre, double Radius, const FLinearColor& Color)
{
	if (Radius <= 0.0 || Color.A <= 0.0f)
	{
		return;
	}
	const FColor C = Bytes(Color);
	const int32 Mid = Add(Centre, C);
	const int32 First = Verts.Num();
	for (int32 I = 0; I < CircleSegments; ++I)
	{
		const double A = UE_DOUBLE_TWO_PI * I / CircleSegments;
		Add(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius, C);
	}
	for (int32 I = 0; I < CircleSegments; ++I)
	{
		Indices.Append({ Mid, First + I, First + (I + 1) % CircleSegments });
	}
}

void FPawFxCanvas::Arc(const FVector2D& Centre, double Radius, double Start, double End, int32 PointCount, const FLinearColor& Color,
	double Width)
{
	if (PointCount < 2 || Radius <= 0.0)
	{
		return;
	}
	TArray<FVector2D, TInlineAllocator<128>> Points;
	for (int32 I = 0; I < PointCount; ++I)
	{
		const double A = Start + (End - Start) * I / (PointCount - 1);
		Points.Add(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius);
	}
	Polyline(Points, Color, Width);
}

void FPawFxCanvas::Line(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, double Width)
{
	const FVector2D Pts[2] = { A, B };
	Polyline(Pts, Color, Width);
}

void FPawFxCanvas::Polyline(TConstArrayView<FVector2D> Points, const FLinearColor& Color, double Width)
{
	const int32 N = Points.Num();
	if (N < 2 || Width <= 0.0 || Color.A <= 0.0f)
	{
		return;
	}
	const FColor C = Bytes(Color);
	// A closed outline (its last point on its first) joins round the seam too.
	const bool bClosed = N > 2 && Points[0].Equals(Points[N - 1], 1e-6);
	const double Half = Width * 0.5;
	const int32 Base = Verts.Num();
	for (int32 I = 0; I < N; ++I)
	{
		const int32 Prev = I > 0 ? I - 1 : (bClosed ? N - 2 : -1);
		const int32 Next = I < N - 1 ? I + 1 : (bClosed ? 1 : -1);
		const FVector2D In = Prev >= 0 ? (Points[I] - Points[Prev]).GetSafeNormal() : FVector2D::ZeroVector;
		const FVector2D Out = Next >= 0 ? (Points[Next] - Points[I]).GetSafeNormal() : FVector2D::ZeroVector;
		const FVector2D NIn(-In.Y, In.X), NOut(-Out.Y, Out.X);
		FVector2D Normal = (NIn + NOut).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = !NOut.IsNearlyZero() ? NOut : NIn;
		}
		// Keep the width across the joint: scale by 1 / cos(half the turn), capped so a hairpin does not spike.
		const double Cos = FVector2D::DotProduct(Normal, !NOut.IsNearlyZero() ? NOut : NIn);
		const double Scale = 1.0 / FMath::Max(Cos, 0.5);
		Add(Points[I] - Normal * Half * Scale, C);
		Add(Points[I] + Normal * Half * Scale, C);
	}
	for (int32 I = 0; I < N - 1; ++I)
	{
		const int32 A0 = Base + 2 * I, A1 = A0 + 1, B0 = A0 + 2, B1 = A0 + 3;
		Indices.Append({ A0, B0, A1, A1, B0, B1 });
	}
}

void FPawFxCanvas::Polygon(TConstArrayView<FVector2D> Points, const FLinearColor& Color)
{
	if (Points.Num() < 3 || Color.A <= 0.0f)
	{
		return;
	}
	const FColor C = Bytes(Color);
	const int32 Base = Verts.Num();
	for (const FVector2D& P : Points)
	{
		Add(P, C);
	}
	for (int32 I = 1; I < Points.Num() - 1; ++I)
	{
		Indices.Append({ Base, Base + I, Base + I + 1 });
	}
}

void FPawFxCanvas::Fan(const FVector2D& Centre, TConstArrayView<FVector2D> Points, const FLinearColor& Color)
{
	const int32 N = Points.Num();
	if (N < 3 || Color.A <= 0.0f)
	{
		return;
	}
	const FColor C = Bytes(Color);
	const int32 Mid = Add(Centre, C);
	const int32 First = Verts.Num();
	for (const FVector2D& P : Points)
	{
		Add(P, C);
	}
	for (int32 I = 0; I < N; ++I)
	{
		Indices.Append({ Mid, First + I, First + (I + 1) % N });
	}
}

void FPawFxCanvas::Polyline(TConstArrayView<FVector2D> Points, TConstArrayView<FLinearColor> PointColors, double Width, double Feather)
{
	const int32 N = Points.Num();
	if (N < 2 || Width <= 0.0 || PointColors.Num() != N)
	{
		return;
	}
	const bool bClosed = N > 2 && Points[0].Equals(Points[N - 1], 1e-6);
	const double Half = Width * 0.5;
	// Per point: the inner and outer edge of the stroke, and (feathered) a clear vertex Feather beyond each.
	const int32 Stride = Feather > 0.0 ? 4 : 2;
	const int32 Base = Verts.Num();
	for (int32 I = 0; I < N; ++I)
	{
		const int32 Prev = I > 0 ? I - 1 : (bClosed ? N - 2 : -1);
		const int32 Next = I < N - 1 ? I + 1 : (bClosed ? 1 : -1);
		const FVector2D In = Prev >= 0 ? (Points[I] - Points[Prev]).GetSafeNormal() : FVector2D::ZeroVector;
		const FVector2D Out = Next >= 0 ? (Points[Next] - Points[I]).GetSafeNormal() : FVector2D::ZeroVector;
		const FVector2D NIn(-In.Y, In.X), NOut(-Out.Y, Out.X);
		FVector2D Normal = (NIn + NOut).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = !NOut.IsNearlyZero() ? NOut : NIn;
		}
		const double Cos = FVector2D::DotProduct(Normal, !NOut.IsNearlyZero() ? NOut : NIn);
		const double Scale = 1.0 / FMath::Max(Cos, 0.5);
		const FColor C = Bytes(PointColors[I]);
		Add(Points[I] - Normal * Half * Scale, C);
		Add(Points[I] + Normal * Half * Scale, C);
		if (Stride == 4)
		{
			const FColor Clear(C.R, C.G, C.B, 0);
			Add(Points[I] - Normal * (Half + Feather) * Scale, Clear);
			Add(Points[I] + Normal * (Half + Feather) * Scale, Clear);
		}
	}
	for (int32 I = 0; I < N - 1; ++I)
	{
		const int32 A = Base + Stride * I, B = A + Stride;
		Indices.Append({ A, B, A + 1, A + 1, B, B + 1 }); // the stroke
		if (Stride == 4)
		{
			Indices.Append({ A + 2, B + 2, A, A, B + 2, B });         // the feather inside
			Indices.Append({ A + 1, B + 1, A + 3, A + 3, B + 1, B + 3 }); // and outside
		}
	}
}

void FPawFxCanvas::Polygon(TConstArrayView<FVector2D> Points, TConstArrayView<FLinearColor> PointColors)
{
	if (Points.Num() < 3 || PointColors.Num() != Points.Num())
	{
		return;
	}
	const int32 Base = Verts.Num();
	for (int32 I = 0; I < Points.Num(); ++I)
	{
		Add(Points[I], Bytes(PointColors[I]));
	}
	for (int32 I = 1; I < Points.Num() - 1; ++I)
	{
		Indices.Append({ Base, Base + I, Base + I + 1 });
	}
}

FLinearColor FPawFxCanvas::FromHsv(double H, double S, double V)
{
	// Color::set_hsv: h in turns, the sextant and its fraction.
	H = FMath::Fmod(H, 1.0);
	if (H < 0.0)
	{
		H += 1.0;
	}
	if (S <= 0.0)
	{
		return FLinearColor(static_cast<float>(V), static_cast<float>(V), static_cast<float>(V), 1.0f);
	}
	H *= 6.0;
	const int32 I = static_cast<int32>(FMath::FloorToDouble(H)) % 6;
	const double F = H - FMath::FloorToDouble(H);
	const double P = V * (1.0 - S), Q = V * (1.0 - S * F), T = V * (1.0 - S * (1.0 - F));
	double R = V, G = T, B = P;
	switch (I)
	{
	case 0: R = V; G = T; B = P; break;
	case 1: R = Q; G = V; B = P; break;
	case 2: R = P; G = V; B = T; break;
	case 3: R = P; G = Q; B = V; break;
	case 4: R = T; G = P; B = V; break;
	default: R = V; G = P; B = Q; break;
	}
	return FLinearColor(static_cast<float>(R), static_cast<float>(G), static_cast<float>(B), 1.0f);
}
